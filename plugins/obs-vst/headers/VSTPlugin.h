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

#ifndef OBS_STUDIO_VSTPLUGIN_H
#define OBS_STUDIO_VSTPLUGIN_H

#define BLOCK_SIZE 512

#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <QThread>
#include <QDirIterator>
#include <obs-module.h>
#include <QProcess>
#include "aeffectx.h"
#include "EditorWidget.h"
#include "headers/vst-plugin-define.hpp"
#include "headers/vst-plugin-callbacks.hpp"
#include "pls/pls-obs-api.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#define do_log(level, format, ...) \
	blog(level, "[VST Plug-in : %p] " format, sourceContext, ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)

class EditorWidget;
class VSTPlugin;

//------------------------------------------------------------------
struct ScanResultCache {
	QString dllHash = "";
	enum obs_vst_verify_state state = VST_STATUS_CHECKING;
};

struct ScanVstInfo {
	std::string path = "";
	QString dllHash;
	std::atomic_bool cancel_flag = false;
	enum obs_vst_verify_state state = VST_STATUS_CHECKING;

	ScanVstInfo(std::string vst);
	virtual ~ScanVstInfo();

	void cancel() { cancel_flag = true; }
	bool isCanceled() { return cancel_flag; }
};

typedef std::shared_ptr<ScanVstInfo> SCAN_VST_INFO_PTR;

//------------------------------------------------------------------
class ScanVstThread : public QThread {
	Q_OBJECT

	VSTPlugin *plugin;
	void *sourceContext;
	QProcess *scanProcess;

public:
	ScanVstThread(VSTPlugin *p, void *s);
	~ScanVstThread();

protected:
	void run() override;

signals:
	void ScanStateChanged(QProcess::ProcessState state);

public slots:
	void startScanVST(SCAN_VST_INFO_PTR info);
	void stopScanVST();
};

//------------------------------------------------------------------
class VSTPlugin : public QObject {
	Q_OBJECT

	obs_source_t *sourceContext = nullptr;

	/* Because effect is always changed in UI thread, so lock is only necessary for these situations:
	1. access effect object outside of UI thread;
	2. close/delete effect object any where. */
	std::recursive_mutex lockEffect;
	AEffect *effect = nullptr;
	std::atomic_bool effectReady = false;
	bool openInterfaceWhenActive = false;

	std::recursive_mutex lockPlugin;
	std::string pluginPath;
	QString pluginHash;
	enum obs_vst_verify_state pluginState = VST_STATUS_CHECKING;

	float **inputs = nullptr;
	float **outputs = nullptr;
	float **channelrefs = nullptr;
	size_t numChannels = 0;

	EditorWidget *editorWidget = nullptr;
	bool editorOpenned = false;

	std::string sourceName;
	std::string filterName;
	char effectName[64];
	VstTimeInfo mTimeInfo;
	// Remove below... or comment out
	char vendorString[64];

#ifdef __APPLE__
	CFBundleRef bundle = NULL;
#elif WIN32
	HINSTANCE dllHandle = nullptr;
#elif __linux__
	void *soHandle = nullptr;
#endif

	ScanVstThread *scanThread = nullptr;
	SCAN_VST_INFO_PTR currentScanInfo =
		nullptr; // only be accessed in UI thread
	std::vector<ScanResultCache>
		scanResultList; // only be accessed in UI thread

private:
	void checkActionLog(const std::string &path);

	void realUpdateVst(const std::string &path,
			   enum obs_vst_verify_state &state,
			   const QString &hash);
	void loadEffectFromPath(const std::string &path,
				enum obs_vst_verify_state state,
				const QString &hash);
	AEffect *loadEffect(const std::string &path);
	void unloadEffect();
	void unloadLibrary();

	void createChannelBuffers(size_t count);
	void cleanupChannelBuffers();

	static intptr_t hostCallback_static(AEffect *effect, int32_t opcode,
					    int32_t index, intptr_t value,
					    void *ptr, float opt);
	VstTimeInfo *GetTimeInfo();
	float GetSampleRate();

public:
	VSTPlugin(obs_source_t *sourceContext);
	~VSTPlugin();

	void NotifyStop();

	std::string getUsingVstHash();
	void getVstState(std::string &vst, enum obs_vst_verify_state &state);
	void updateVst(std::string path, bool openUIWhenActive);
	void clearVst();
	void process(struct obs_audio_data *audio);

	void setChunk(std::string data);
	std::string getChunk();

	void getSourceNames();
	void onEditorClosed();

public slots:
	void openEditor();
	void closeEditor();
	void scanFinish(SCAN_VST_INFO_PTR info);
};

#if defined(__APPLE__)
std::string getVstLibraryPath();
#endif

#endif // OBS_STUDIO_VSTPLUGIN_H
