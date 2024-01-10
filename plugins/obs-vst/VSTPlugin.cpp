/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.
Additional Code Copyright (C) 2016-2017 by c3r1c3 <c3r1c3@nevermindonline.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "headers/VSTPlugin.h"
#include <util/platform.h>
#include <algorithm>
#include <QCryptographicHash>
#include <QApplication>

#define SCAN_VST_TIMEOUT 60000 // in milliseconds

QString getFileMD5(const char *file)
{
	QFile f(file);
	if (f.open(QFile::ReadOnly)) {
		QCryptographicHash hash(QCryptographicHash::Md5);
		if (hash.addData(&f))
			return QString(hash.result().toHex());
	}

	return QString();
}

ScanVstInfo::ScanVstInfo(std::string vst)
{
	path = vst;
	dllHash = getFileMD5(vst.c_str());
}

ScanVstInfo::~ScanVstInfo() {}

ScanVstThread::ScanVstThread(VSTPlugin *p, void *s)
	: plugin(p), sourceContext(s)
{
	qRegisterMetaType<SCAN_VST_INFO_PTR>("SCAN_VST_INFO_PTR");

	moveToThread(this);
}

ScanVstThread::~ScanVstThread()
{
	stopScanVST();
}

void ScanVstThread::run()
{
	scanProcess = new QProcess(this);
	connect(scanProcess, &QProcess::stateChanged, this,
		&ScanVstThread::ScanStateChanged);
	this->exec();
}

void ScanVstThread::stopScanVST()
{
	scanProcess->kill();
}

#if defined(_WIN32)
#define VST_CHECK "vst-check.exe"
#else
#define VST_CHECK "vst-check"
#endif

void ScanVstThread::startScanVST(SCAN_VST_INFO_PTR info)
{
	enum obs_vst_verify_state result = VST_STATUS_UNKNOWN_ERROR;

	char *exe_path = os_get_executable_path_ptr(VST_CHECK);

	QString vst_path = QStringLiteral("--vst-path=") +
			   QString::fromUtf8(info->path.c_str());
#if defined(_WIN32)
	char *m = pls_get_module_file_name_ptr("obs-vst.dll");
	QString module_file_name = QStringLiteral("--vst-module-path=") +
				   QString::fromUtf8(m ? m : "");
	if (m)
		bfree(m);
#elif defined(__APPLE__)
	auto pluginPath = getVstLibraryPath();
	QString module_file_name =
		QStringLiteral("--vst-module-path=") + QString::fromUtf8(pluginPath.c_str(), pluginPath.length());
#endif

    if(scanProcess->state() == QProcess::Running)
        scanProcess->kill();
    
	auto st = os_gettime_ns();
	scanProcess->start(QString::fromUtf8(exe_path),
			   {vst_path, module_file_name});
	if (exe_path)
		bfree(exe_path);

	if (!scanProcess->waitForStarted(SCAN_VST_TIMEOUT)) {
		warn("Failed to wait scan result for start process failed.");
		result = VST_STATUS_UNKNOWN_ERROR;

	} else if (scanProcess->waitForFinished(SCAN_VST_TIMEOUT)) {

		auto exitCode = scanProcess->exitCode();
		auto exitStatus = scanProcess->exitStatus();
		scanProcess->kill();

		auto tm = os_gettime_ns() - st;
		info("Scan process exit code: %u, take time: %u ms", exitCode,
		     tm / 1000000);

		if (exitStatus == QProcess::CrashExit) {
			warn("VST scan process crashed");
			result = VST_STATUS_CRASH;
		} else {
			if (exitCode < VST_STATUS_SCAN_RESULT_END) {
				result = (enum obs_vst_verify_state)exitCode;
			} else if (exitCode > VST_STATUS_PROCESS_DISAPPEAR) {
				warn("Scan process return unknown exit code: %u, it will be treated as CRASH",
				     exitCode);
				result = VST_STATUS_CRASH;
			} else {
				warn("Scan process return unknown exit code: %u",
				     exitCode);
				result = (enum obs_vst_verify_state)exitCode;
				assert(false);
			}
		}
	} else {
		auto error = scanProcess->error();
		scanProcess->kill();
		switch (error) {
		case QProcess::FailedToStart:
			warn("Failed to wait scan result for start process failed.");
			result = VST_STATUS_UNKNOWN_ERROR;
			break;
		case QProcess::Crashed:
			warn("Failed to wait scan result for Scan VST crashed.");
			result = VST_STATUS_CRASH;
			break;
		case QProcess::Timedout:
			warn("Timeout to wait scan result");
			result = VST_STATUS_TIMEOUT;
			break;
		case QProcess::ReadError:
			warn("Failed to wait scan result for read error.");
			result = VST_STATUS_UNKNOWN_ERROR;
			break;
		case QProcess::WriteError:
			warn("Failed to wait scan result for write error.");
			result = VST_STATUS_UNKNOWN_ERROR;
			break;
		case QProcess::UnknownError:
			warn("Failed to wait scan result. res:%u LastError:%u",
			     error);
			result = VST_STATUS_UNKNOWN_ERROR;
			assert(false);
			break;
		default:
			break;
		}
	}

	if (!info->isCanceled()) {
		info->state = result;
		QMetaObject::invokeMethod(plugin, "scanFinish",
					  Q_ARG(SCAN_VST_INFO_PTR, info));
	}
}

intptr_t VSTPlugin::hostCallback_static(AEffect *effect, int32_t opcode,
					int32_t index, intptr_t value,
					void *ptr, float opt)
{
	VSTPlugin *plugin = nullptr;
	if (effect && effect->user) {
		plugin = static_cast<VSTPlugin *>(effect->user);
	}

	switch (opcode) {
	case audioMasterVersion:
		return (intptr_t)2400;

	case audioMasterGetCurrentProcessLevel:
		return 1;

	// We always replace, never accumulate
	case audioMasterWillReplaceOrAccumulate:
		return 1;

	case audioMasterGetSampleRate:
		if (plugin) {
			return (intptr_t)plugin->GetSampleRate();
		}
		return 0;

	case audioMasterGetTime:
		if (plugin) {
			return (intptr_t)plugin->GetTimeInfo();
		}
		return 0;

	// index: width, value: height
	case audioMasterSizeWindow:
		if (plugin && plugin->editorWidget) {
			plugin->editorWidget->handleResizeRequest(index, value);
		}
		return 1;
	// add the ability of resizing window. Reference from Audacity.
	case audioMasterCanDo: {
		char *s = (char *)ptr;
		if (strcmp(s, "sizeWindow") == 0) {
			return 1;
		}
		return 0;
	}

	default:
		return 0;
	}

	UNUSED_PARAMETER(ptr);
	UNUSED_PARAMETER(opt);
}

VstTimeInfo *VSTPlugin::GetTimeInfo()
{
	mTimeInfo.nanoSeconds = os_gettime_ns() / 1000000;
	return &mTimeInfo;
}

float VSTPlugin::GetSampleRate()
{
	return mTimeInfo.sampleRate;
}

VSTPlugin::VSTPlugin(obs_source_t *sourceContext) : sourceContext{sourceContext}
{
	scanThread = new ScanVstThread(this, sourceContext);
	scanThread->start();
}

VSTPlugin::~VSTPlugin()
{
	NotifyStop();
	scanThread->quit();
	scanThread->wait();
	scanThread->deleteLater();

	unloadEffect();
	cleanupChannelBuffers();
}

void VSTPlugin::NotifyStop()
{
	QMetaObject::invokeMethod(scanThread, "stopScanVST",
				  Qt::QueuedConnection);
}

void VSTPlugin::createChannelBuffers(size_t count)
{
	cleanupChannelBuffers();

	int blocksize = BLOCK_SIZE;
	numChannels = (std::max)((size_t)0, count);

	if (numChannels > 0) {
		inputs = (float **)malloc(sizeof(float *) * numChannels);
		outputs = (float **)malloc(sizeof(float *) * numChannels);
		channelrefs = (float **)malloc(sizeof(float *) * numChannels);
		for (size_t channel = 0; channel < numChannels; channel++) {
			inputs[channel] =
				(float *)malloc(sizeof(float) * blocksize);
			outputs[channel] =
				(float *)malloc(sizeof(float) * blocksize);
		}
	}
}

void VSTPlugin::cleanupChannelBuffers()
{
	for (size_t channel = 0; channel < numChannels; channel++) {
		if (inputs && inputs[channel]) {
			free(inputs[channel]);
			inputs[channel] = NULL;
		}
		if (outputs && outputs[channel]) {
			free(outputs[channel]);
			outputs[channel] = NULL;
		}
	}
	if (inputs) {
		free(inputs);
		inputs = NULL;
	}
	if (outputs) {
		free(outputs);
		outputs = NULL;
	}
	if (channelrefs) {
		free(channelrefs);
		channelrefs = NULL;
	}
	numChannels = 0;
}

void VSTPlugin::checkActionLog(const std::string &path)
{
	bool pushActionLog = false;

	{
		std::lock_guard<std::recursive_mutex> lock(lockPlugin);
		pushActionLog = (!pluginPath.empty() && !path.empty() &&
				 pluginPath != path);
	}

	if (pushActionLog) {
		std::string target =
			QFileInfo(path.c_str()).fileName().toStdString();

		//obs_source_action_event_notify(
		//sourceContext, OBS_SOURCE_VST_CHANGED, "init", "plugin", "vstPlugin", target.c_str());

		//const char *fields[][2] = {{"plugin", "obs-vst"}, {"editEvent", target.c_str()}};
		//blogex(false, LOG_INFO, fields, 2, "Nelo action log. <obs-vst> [editEvent : %s]", target.c_str());
	}
}

std::string VSTPlugin::getUsingVstHash()
{
	std::lock_guard<std::recursive_mutex> lock(lockPlugin);
	return pluginHash.toStdString();
}

void VSTPlugin::getVstState(std::string &vst, enum obs_vst_verify_state &state)
{
	SCAN_VST_INFO_PTR temp = currentScanInfo;
	if (temp.get()) {
		vst = temp->path;
		state = VST_STATUS_CHECKING;
	} else {
		std::lock_guard<std::recursive_mutex> lock(lockPlugin);
		vst = pluginPath;
		state = pluginState;
	}
}

void VSTPlugin::updateVst(std::string path, bool openUIWhenActive)
{
	std::string currentVst;
	enum obs_vst_verify_state currentState;
	getVstState(currentVst, currentState);

	bool activeChanged = (openInterfaceWhenActive != openUIWhenActive);
	openInterfaceWhenActive = openUIWhenActive;

	auto fileName = vst::getFileName(QString::fromUtf8(path.c_str()));

	if (currentVst == path) {
		if (!activeChanged)
			return; /* nothing is changed, keep state */

		if (currentState == VST_STATUS_CHECKING)
			return;

		if (currentState == VST_STATUS_AVAILABLE) {
			QString dllHash = getFileMD5(currentVst.c_str());
			realUpdateVst(currentVst, currentState, dllHash);
		}

		/* Won't start new scan if VST is not changed */
		return;

	} else {
		info("User selected new VST plugin: '%s'",
		     qUtf8Printable(fileName));
	}

	unloadEffect();

	if (currentScanInfo.get()) {
		//SetEvent(currentScanInfo->cancel);
		currentScanInfo->cancel();
		scanThread->stopScanVST();
		currentScanInfo = SCAN_VST_INFO_PTR();
	}

	currentScanInfo = SCAN_VST_INFO_PTR(new ScanVstInfo(path));
	currentScanInfo->state = VST_STATUS_CHECKING;

	if (currentScanInfo->dllHash.length() > 0) {
		ScanResultCache cache;
		for (size_t i = 0; i < scanResultList.size(); i++) {
			ScanResultCache temp = scanResultList[i];
			if (temp.dllHash == currentScanInfo->dllHash) {
				cache = temp;
				break;
			}
		}

		if (cache.dllHash.length() > 0) {
			currentScanInfo->state = cache.state;
			info("Find cached scan result for: '%s'",
			     qUtf8Printable(fileName));
			scanFinish(currentScanInfo);
			return;
		}
	}

	info("Start scanning VST: '%s'", qUtf8Printable(fileName));

	pls_vst_state_changed(sourceContext, path.c_str(), VST_STATUS_CHECKING);

	QMetaObject::invokeMethod(scanThread, "startScanVST",
				  Q_ARG(SCAN_VST_INFO_PTR, currentScanInfo));
}

void VSTPlugin::clearVst()
{
	info("clear effect");

	unloadEffect();

	{
		std::lock_guard<std::recursive_mutex> lock(lockPlugin);
		pluginPath = "";
		pluginHash = "";
		pluginState = VST_STATUS_AVAILABLE;
	}

	if (currentScanInfo.get()) {
		//SetEvent(currentScanInfo->cancel);
		currentScanInfo->cancel();
		scanThread->stopScanVST();
		currentScanInfo = SCAN_VST_INFO_PTR();
	}
}

void VSTPlugin::realUpdateVst(const std::string &path,
			      enum obs_vst_verify_state &state,
			      const QString &hash)
{
	loadEffectFromPath(path, state, hash);

	if (!effectReady && state == VST_STATUS_AVAILABLE) {
		std::lock_guard<std::recursive_mutex> lock(lockPlugin);
		pluginState = state = VST_STATUS_UNKNOWN_ERROR;
	}

	obs_data_t *settings = obs_source_get_settings(sourceContext);
	const char *chunkData = obs_data_get_string(settings, "chunk_data");
	const char *chunkHash = obs_data_get_string(settings, "chunk_hash");
	if (chunkData && (*chunkData) && chunkHash && (*chunkHash) &&
	    (hash == QString(chunkHash)))
		setChunk(std::string(chunkData));
	obs_data_release(settings);
}

void VSTPlugin::loadEffectFromPath(const std::string &path,
				   enum obs_vst_verify_state state,
				   const QString &hash)
{
	checkActionLog(path);

	bool uninit = false;
	{
		std::lock_guard<std::recursive_mutex> lock(lockPlugin);
		uninit = (pluginPath != path);
		pluginPath = path;
		pluginHash = hash;
		pluginState = state;
	}

	if (uninit) {
		unloadEffect();
	}

	if (state != VST_STATUS_AVAILABLE) {
		warn("VST is not available and won't use it!");
		unloadEffect();
		return;
	}

	if (effect)
		return;

	AEffect *effectTemp = loadEffect(path);
	if (!effectTemp) {
		warn("Can't load effect!");
		return;
	}

	{
		std::lock_guard<std::recursive_mutex> lock(lockEffect);
		effect = effectTemp;
	}

	// Check plug-in's magic number
	// If incorrect, then the file either was not loaded properly,
	// is not a real VST plug-in, or is otherwise corrupt.
	if (effect->magic != kEffectMagic) {
		warn("magic number is bad");
		return;
	}

	// it is better to call this code after checking magic number
	effect->dispatcher(effect, effGetEffectName, 0, 0, effectName, 0);
	effect->dispatcher(effect, effGetVendorString, 0, 0, vendorString, 0);

	// PRISM/WangShaohui/20211124/noissue/modify vst init code, refer to
	// https://github.com/audacity/audacity
	if ((effect->flags & effFlagsIsSynth) ||
	    !(effect->flags & effFlagsCanReplacing)) {
		warn("flag is bad");
		return;
	}

	int maxchans = (std::max)(effect->numInputs, effect->numOutputs);
	// sanity check
	if (maxchans < 0 || maxchans > 256) {
		warn("has invalid number of channels");
		return;
	}

	createChannelBuffers(maxchans);

	size_t sampleRate = audio_output_get_sample_rate(obs_get_audio());

	// Initialize time info
	memset(&mTimeInfo, 0, sizeof(mTimeInfo));
	mTimeInfo.sampleRate = sampleRate;
	mTimeInfo.nanoSeconds = os_gettime_ns() / 1000000;
	mTimeInfo.tempo = 120.0;
	mTimeInfo.timeSigNumerator = 4;
	mTimeInfo.timeSigDenominator = 4;
	mTimeInfo.flags = kVstTempoValid | kVstNanosValid |
			  kVstTransportPlaying;

	// PRISM/WangShaohui/20211124/noissue/modify vst init code, refer to
	// https://github.com/audacity/audacity
	effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, sampleRate);
	effect->dispatcher(effect, effSetBlockSize, 0, BLOCK_SIZE, nullptr,
			   0.0f);

	// Ask the plugin to identify itself...might be needed for older plugins
	effect->dispatcher(effect, effIdentify, 0, 0, nullptr, 0.0f);

	effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);

	// Set some default properties
	effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, sampleRate);
	effect->dispatcher(effect, effSetBlockSize, 0, BLOCK_SIZE, nullptr,
			   0.0f);

	effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0);

	effectReady = true;

	if (openInterfaceWhenActive) {
		openEditor();
	}
}

void silenceChannel(float **channelData, size_t numChannels, long numFrames)
{
	for (size_t channel = 0; channel < numChannels; ++channel) {
		for (long frame = 0; frame < numFrames; ++frame) {
			channelData[channel][frame] = 0.0f;
		}
	}
}

void VSTPlugin::process(struct obs_audio_data *audio)
{
	// Here we check the status firstly,
	// which help avoid waiting for lock while unloadEffect() is running.
	bool effectValid = (effect && effectReady && numChannels > 0);
	if (!effectValid)
		return;

	std::lock_guard<std::recursive_mutex> lock(lockEffect);

	if (effect && effectReady && numChannels > 0) {
		uint passes = (audio->frames + BLOCK_SIZE - 1) / BLOCK_SIZE;
		uint extra = audio->frames % BLOCK_SIZE;
		for (uint pass = 0; pass < passes; pass++) {
			uint frames = pass == passes - 1 && extra ? extra
								  : BLOCK_SIZE;
			silenceChannel(outputs, numChannels, BLOCK_SIZE);

			for (size_t d = 0; d < numChannels; d++) {
				if (d < MAX_AV_PLANES &&
				    audio->data[d] != nullptr) {
					channelrefs[d] =
						((float *)audio->data[d]) +
						(pass * BLOCK_SIZE);
				} else {
					channelrefs[d] = inputs[d];
				}
			};

			effect->processReplacing(effect, channelrefs, outputs,
						 frames);

			// only copy back the channels the plugin may have generated
			for (size_t c = 0; c < (size_t)effect->numOutputs &&
					   c < MAX_AV_PLANES;
			     c++) {
				if (audio->data[c]) {
					for (size_t i = 0; i < frames; i++) {
						channelrefs[c][i] =
							outputs[c][i];
					}
				}
			}
		}
	}
}

void VSTPlugin::unloadEffect()
{
	closeEditor();

	{
		std::lock_guard<std::recursive_mutex> lock(lockEffect);

		// Reset the status firstly to avoid VSTPlugin::process is blocked
		effectReady = false;

		if (effect) {
			effect->dispatcher(effect, effMainsChanged, 0, 0,
					   nullptr, 0);
			effect->dispatcher(effect, effClose, 0, 0, nullptr,
					   0.0f);
		}

		effect = nullptr;
	}

	unloadLibrary();
}

void VSTPlugin::onEditorClosed()
{
	if (!editorWidget)
		return;

	editorWidget->deleteLater();
	editorWidget = nullptr;

	if (effect && editorOpenned) {
		editorOpenned = false;
		effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0);
	}
}

std::string VSTPlugin::getChunk()
{
	if (!effect || !effectReady) {
		return "";
	}

	if (effect->flags & effFlagsProgramChunks) {
		void *buf = nullptr;

		intptr_t chunkSize = effect->dispatcher(effect, effGetChunk, 1,
							0, &buf, 0.0);

		QByteArray data = QByteArray((char *)buf, chunkSize);
		return QString(data.toBase64()).toStdString();
	} else {
		std::vector<float> params;
		for (int i = 0; i < effect->numParams; i++) {
			float parameter = effect->getParameter(effect, i);
			params.push_back(parameter);
		}

		const char *bytes = reinterpret_cast<const char *>(&params[0]);
		QByteArray data =
			QByteArray(bytes, (int)(sizeof(float) * params.size()));
		std::string encoded = QString(data.toBase64()).toStdString();
		return encoded;
	}
}

void VSTPlugin::setChunk(std::string data)
{
	if (!effect || !effectReady) {
		return;
	}

	if (effect->flags & effFlagsProgramChunks) {
		QByteArray base64Data =
			QByteArray(data.c_str(), (int)data.length());
		QByteArray chunkData = QByteArray::fromBase64(base64Data);
		void *buf = nullptr;
		buf = chunkData.data();
		effect->dispatcher(effect, effSetChunk, 1, chunkData.length(),
				   buf, 0);
	} else {
		QByteArray base64Data =
			QByteArray(data.c_str(), (int)data.length());
		QByteArray paramData = QByteArray::fromBase64(base64Data);

		const char *p_chars = paramData.data();
		const float *p_floats =
			reinterpret_cast<const float *>(p_chars);

		auto size = paramData.length() / sizeof(float);

		std::vector<float> params(p_floats, p_floats + size);

		if (params.size() != (size_t)effect->numParams) {
			return;
		}

		for (int i = 0; i < effect->numParams; i++) {
			effect->setParameter(effect, i, params[i]);
		}
	}
}

void VSTPlugin::getSourceNames()
{
	/* Only call inside the vst_filter_audio function! */
	sourceName = obs_source_get_name(obs_filter_get_parent(sourceContext));
	filterName = obs_source_get_name(sourceContext);
}

void VSTPlugin::openEditor()
{
	if (effect && effectReady) {
		if (!(effect->flags & effFlagsHasEditor)) {
			warn("Can't support edit feature");
			return;
		}

		if (editorWidget) {
			if (Qt::WindowMinimized & editorWidget->windowState()) {
				editorWidget->setWindowState(
					editorWidget->windowState() ^
						Qt::WindowMinimized |
					Qt::WindowActive);
			}
#if defined(_WIN32)
			::SetActiveWindow((HWND)editorWidget->winId());
#elif defined(_APPLE_)
			QApplication::setAcviveWindow(editorWidget);
#endif
			return;
		}

		editorOpenned = true;
		editorWidget = new EditorWidget(nullptr, this, sourceContext);
		editorWidget->buildEffectContainer(effect);

		if (sourceName.empty()) {
			sourceName = "VST 2.x";
		}

		if (filterName.empty()) {
			editorWidget->setWindowTitle(QString("%1 - %2").arg(
				sourceName.c_str(), effectName));
		} else {
			editorWidget->setWindowTitle(
				QString("%1: %2 - %3")
					.arg(sourceName.c_str(),
					     filterName.c_str(), effectName));
		}

		// if modal widget is showing, lower the vst interface.
		auto modalWidget = QApplication::activeModalWidget();
		if (modalWidget) {
			editorWidget->show();
			modalWidget->raise();
		} else {
			editorWidget->show();
		}
	}
}

void VSTPlugin::closeEditor()
{
	if (editorWidget)
		editorWidget->close();
}

std::string getStateString(enum obs_vst_verify_state state)
{
	switch (state) {
	case VST_STATUS_AVAILABLE:
		return "OK";
	case VST_STATUS_INVALID_ARCH:
		return "32Bit";
	case VST_STATUS_EFFECT_UNSUPPORT:
		return "Effect_Unsupported";
	case VST_STATUS_TIMEOUT:
		return "Scan_Timeout";
	case VST_STATUS_CRASH:
		return "VST_Crashed";
	case VST_STATUS_NOT_VST:
		return "Not_VST";
	case VST_STATUS_CHANNEL_UNSUPPORT:
		return "Channel_Unsupported";
	case VST_STATUS_DLL_LOAD_FAIL:
		return "Dll_Load_Failed";
	case VST_STATUS_UNKNOWN_ERROR:
		return "Unknown_Error";
	default: {
		assert(false);
		char buf[200];
		sprintf(buf, "Unknown_State_%u", state);
		return buf;
	}
	}
}

void VSTPlugin::scanFinish(SCAN_VST_INFO_PTR info)
{
	if (info != currentScanInfo)
		return;

	currentScanInfo = SCAN_VST_INFO_PTR();

	obs_source_t *s = obs_source_get_ref(sourceContext);
	if (!s)
		return;

	auto fileName = vst::getFileName(QString::fromUtf8(info->path.c_str()));

	info("Got scan result for '%s' : %s", qUtf8Printable(fileName),
	     getStateString(info->state).c_str());
	realUpdateVst(info->path, info->state, info->dllHash);

	pls_vst_state_changed(sourceContext, info->path.c_str(), info->state);
	obs_source_release(s);

	switch (info->state) {
	case VST_STATUS_INVALID_ARCH:
	case VST_STATUS_EFFECT_UNSUPPORT:
	case VST_STATUS_NOT_VST:
	case VST_STATUS_CHANNEL_UNSUPPORT:
	case VST_STATUS_DLL_LOAD_FAIL: {
		ScanResultCache cache;
		cache.dllHash = info->dllHash;
		cache.state = info->state;
		scanResultList.push_back(cache);
		info("Push scan result into cache list '%s' : %s",
		     qUtf8Printable(fileName),
		     getStateString(info->state).c_str());
		break;
	}
	default:
		break;
	}
}
