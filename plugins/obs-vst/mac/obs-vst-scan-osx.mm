#include "headers/VSTPlugin.h"
#include "headers/vst-plugin-define.hpp"
#include <util/platform.h>
#import <Foundation/Foundation.h>

#define TEST_SAMPLERATE 48000

void *process_handle = nullptr;
void *module_handle = nullptr;
CFBundleRef bundle    = NULL;
AEffect *   effect    = nullptr;
VstTimeInfo mTimeInfo;
char        effectName[64];
char        vendorString[64];

int     maxchans    = 0;
float **inputs      = nullptr;
float **outputs     = nullptr;
float **channelrefs = nullptr;
        
void utils_api_import(void *module, const char *func, bool *success)
{
    void *handle = os_dlsym(module, func);
    if(!handle){
        *success = false;
        blog(LOG_WARNING, "Could not load function '%s'", func);
    }
}

intptr_t VstHostCallback(AEffect *effect, int32_t opcode, int32_t index, intptr_t value, void *ptr, float opt)
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
			return TEST_SAMPLERATE;
		}
		return 0;

	case audioMasterGetTime:
		mTimeInfo.nanoSeconds = os_gettime_ns() / 1000000;
		return (intptr_t)&mTimeInfo;

	// index: width, value: height
	case audioMasterSizeWindow:
		return 0;

	default:
		return 0;
	}

	UNUSED_PARAMETER(ptr);
	UNUSED_PARAMETER(opt);
	UNUSED_PARAMETER(value);
	UNUSED_PARAMETER(index);
}

typedef AEffect *(*vstPluginMain)(audioMasterCallback audioMaster);
bool InitEffect(const char *path, int *result)
{
	// Create a path to the bundle
	CFStringRef pluginPathStringRef = CFStringCreateWithCString(
		NULL, path, kCFStringEncodingUTF8);
	CFURLRef bundleUrl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
							   pluginPathStringRef,
							   kCFURLPOSIXPathStyle,
							   true);

	if (bundleUrl == NULL) {
		blog(LOG_WARNING,
		     "Couldn't make URL reference for VST plug-in");
		*result = VST_STATUS_DLL_LOAD_FAIL;
		return false;
	}

	// Open the bundle
	bundle = CFBundleCreate(kCFAllocatorDefault, bundleUrl);
	if (bundle == NULL) {
		blog(LOG_WARNING, "Couldn't create VST bundle reference.");
		CFRelease(pluginPathStringRef);
		CFRelease(bundleUrl);
		*result = VST_STATUS_DLL_LOAD_FAIL;
		return false;
	}

	vstPluginMain mainEntryPoint = NULL;
	mainEntryPoint = (vstPluginMain)CFBundleGetFunctionPointerForName(bundle, CFSTR("VSTPluginMain"));
	
	// VST plugins previous to the 2.4 SDK used main_macho for the
	// entry point name.
	if (mainEntryPoint == NULL) {
		mainEntryPoint =
			(vstPluginMain)CFBundleGetFunctionPointerForName(
				bundle, CFSTR("main_macho"));
	}

	if (mainEntryPoint == NULL) {
		blog(LOG_WARNING, "Couldn't get a pointer to plug-in's main()");
		CFRelease(pluginPathStringRef);
		CFRelease(bundle);
		bundle = NULL;
		*result = VST_STATUS_NOT_VST;
		return false;
	}

	if (mainEntryPoint == nullptr) {
		CFRelease(pluginPathStringRef);
		CFRelease(bundle);
		*result = VST_STATUS_NOT_VST;
		return false;
	}

	@try {
		effect = mainEntryPoint(VstHostCallback);
	} 
	@catch (NSException *) {
		CFRelease(pluginPathStringRef);
		CFRelease(bundle);
		*result = VST_STATUS_CRASH;
		return false;
	}

	if (!effect) {
		CFRelease(pluginPathStringRef);
		CFRelease(bundle);
		*result = VST_STATUS_EFFECT_NULLPTR;
		return false;
	}

	// Successed clean up
	CFRelease(pluginPathStringRef);
	CFRelease(bundle);

	return true;
}

bool CheckEffect(int *result)
{
	if (effect->magic != kEffectMagic) {
		*result = VST_STATUS_NOT_VST;
		return false;
	}

	effect->dispatcher(effect, effGetEffectName, 0, 0, effectName, 0);
	effect->dispatcher(effect, effGetVendorString, 0, 0, vendorString, 0);

	if ((effect->flags & effFlagsIsSynth) || !(effect->flags & effFlagsCanReplacing)) {
		*result = VST_STATUS_EFFECT_UNSUPPORT;
		return false;
	}

	maxchans = (std::max)(effect->numInputs, effect->numOutputs);
	if (maxchans < 0 || maxchans > 256) {
		*result = VST_STATUS_CHANNEL_UNSUPPORT;
		return false;
	}

	int blocksize   = BLOCK_SIZE;
	int numChannels = maxchans;
	inputs          = (float **)malloc(sizeof(float *) * numChannels);
	outputs         = (float **)malloc(sizeof(float *) * numChannels);
	channelrefs     = (float **)malloc(sizeof(float *) * numChannels);
	for (int channel = 0; channel < numChannels; channel++) {
		inputs[channel]      = (float *)malloc(sizeof(float) * blocksize);
		outputs[channel]     = (float *)malloc(sizeof(float) * blocksize);
		channelrefs[channel] = (float *)malloc(sizeof(float) * blocksize);
	}

	size_t sampleRate = TEST_SAMPLERATE;

	memset(&mTimeInfo, 0, sizeof(mTimeInfo));
	mTimeInfo.sampleRate         = sampleRate;
	mTimeInfo.nanoSeconds        = os_gettime_ns() / 1000000;
	mTimeInfo.tempo              = 120.0;
	mTimeInfo.timeSigNumerator   = 4;
	mTimeInfo.timeSigDenominator = 4;
	mTimeInfo.flags              = kVstTempoValid | kVstNanosValid | kVstTransportPlaying;

	effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, sampleRate);
	effect->dispatcher(effect, effSetBlockSize, 0, BLOCK_SIZE, nullptr, 0.0f);

	// Ask the plugin to identify itself...might be needed for older plugins
	effect->dispatcher(effect, effIdentify, 0, 0, nullptr, 0.0f);

	effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);

	// Set some default properties
	effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, sampleRate);
	effect->dispatcher(effect, effSetBlockSize, 0, BLOCK_SIZE, nullptr, 0.0f);

	effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0);

	return true;
}

void TestChunk()
{
	std::string res = "";
	if (effect->flags & effFlagsProgramChunks) {
		void *buf = nullptr;

		intptr_t chunkSize = effect->dispatcher(effect, effGetChunk, 1, 0, &buf, 0.0);

		QByteArray data = QByteArray((char *)buf, chunkSize);
		QString    temp = QString(data.toBase64());
		res             = QString(data.toBase64()).toStdString();
	} else {
		std::vector<float> params;
		for (int i = 0; i < effect->numParams; i++) {
			float parameter = effect->getParameter(effect, i);
			params.push_back(parameter);
		}

		const char *bytes   = reinterpret_cast<const char *>(&params[0]);
		QByteArray  data    = QByteArray(bytes, (int)(sizeof(float) * params.size()));
		std::string encoded = QString(data.toBase64()).toStdString();
		res                 = encoded;
	}

	if (res.empty())
		return;

	std::string data = res;
	if (effect->flags & effFlagsProgramChunks) {
		QByteArray base64Data = QByteArray(data.c_str(), (int)data.length());
		QByteArray chunkData  = QByteArray::fromBase64(base64Data);
		void *     buf        = nullptr;
		buf                   = chunkData.data();
		effect->dispatcher(effect, effSetChunk, 1, chunkData.length(), buf, 0);
	} else {
		QByteArray base64Data = QByteArray(data.c_str(), (int)data.length());
		QByteArray paramData  = QByteArray::fromBase64(base64Data);

		const char * p_chars  = paramData.data();
		const float *p_floats = reinterpret_cast<const float *>(p_chars);

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

void TestProcess()
{
	effect->processReplacing(effect, channelrefs, outputs, BLOCK_SIZE);
	effect->processReplacing(effect, channelrefs, outputs, BLOCK_SIZE);
	effect->processReplacing(effect, channelrefs, outputs, BLOCK_SIZE);
}

void UninitEffect()
{
	effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0);
	effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);

	if (bundle) {
		CFRelease(bundle);
		bundle = NULL;
	}
    
    if(module_handle){
        os_dlclose(module_handle);
        module_handle = nullptr;
    }
}

extern "C" __attribute__((visibility("default"))) int ScanVstPlugin(const char *path)
{
	enum obs_vst_verify_state result = VST_STATUS_AVAILABLE;
	if(!InitEffect(path, (int *)&result)){
		return result;
	}

	if(!CheckEffect((int *)&result)){
		return result;
	}

	TestChunk();
	TestProcess();
	UninitEffect();

	blog(LOG_INFO, "All test cases passed, so this VST is available");
	return VST_STATUS_AVAILABLE;
}
