#include <objbase.h>

#include <obs-module.h>
#include <obs.hpp>
#include <util/dstr.hpp>
#include <util/platform.h>
#include <util/windows/WinHandle.hpp>
#include <util/threading.h>
#include "libdshowcapture/dshowcapture.hpp"
#include "ffmpeg-decode.h"
#include "encode-dstr.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <string>
#include <vector>
//PRISM/Xiewei/20230712/noissue/add log start
#include <atomic>
#include <chrono>
#include <optional>
//#define _DEBUG_AV_DATA
//PRISM/Xiewei/20230712/noissue/add log end

//PRISM/Liuying/20230808/new ux : add prism lens source
#include "pls/pls-source.h"
#include "pls/pls-properties.h"
#include "pls/pls-obs-api.h"
#include "tchar.h"

//PRISM/Xiewei/20240621/none/add more logs to trace video data
#include "pls/data-monitor/pls-data-monitor.h"
#include "pls/data-monitor/device-notification.h"

//PRISM/FanZirong/20241203/PRISM_PC-1675/add log fields
#include <pls/pls-base.h>

/*
 * TODO:
 *   - handle disconnections and reconnections
 *   - if device not present, wait for device to be plugged in
 */

#undef min
#undef max

using namespace std;
using namespace DShow;

/* clang-format off */

/* settings defines that will cause errors if there are typos */
#define VIDEO_DEVICE_ID   "video_device_id"
#define RES_TYPE          "res_type"
#define RESOLUTION        "resolution"
#define FRAME_INTERVAL    "frame_interval"
#define VIDEO_FORMAT      "video_format"
#define LAST_VIDEO_DEV_ID "last_video_device_id"
#define LAST_RESOLUTION   "last_resolution"
#define BUFFERING_VAL     "buffering"
#define FLIP_IMAGE        "flip_vertically"
#define AUDIO_OUTPUT_MODE "audio_output_mode"
#define USE_CUSTOM_AUDIO  "use_custom_audio_device"
#define AUDIO_DEVICE_ID   "audio_device_id"
#define COLOR_SPACE       "color_space"
#define COLOR_RANGE       "color_range"
#define DEACTIVATE_WNS    "deactivate_when_not_showing"
#define AUTOROTATION      "autorotation"
#define HW_DECODE         "hw_decode"

#define TEXT_INPUT_NAME     obs_module_text("VideoCaptureDevice")
#define TEXT_DEVICE         obs_module_text("Device")
#define TEXT_CONFIG_VIDEO   obs_module_text("ConfigureVideo")
#define TEXT_CONFIG_XBAR    obs_module_text("ConfigureCrossbar")
#define TEXT_RES_FPS_TYPE   obs_module_text("ResFPSType")
#define TEXT_CUSTOM_RES     obs_module_text("ResFPSType.Custom")
#define TEXT_PREFERRED_RES  obs_module_text("ResFPSType.DevPreferred")
#define TEXT_FPS_MATCHING   obs_module_text("FPS.Matching")
#define TEXT_FPS_HIGHEST    obs_module_text("FPS.Highest")
#define TEXT_RESOLUTION     obs_module_text("Resolution")
#define TEXT_VIDEO_FORMAT   obs_module_text("VideoFormat")
#define TEXT_FORMAT_UNKNOWN obs_module_text("VideoFormat.Unknown")
#define TEXT_BUFFERING      obs_module_text("Buffering")
#define TEXT_BUFFERING_AUTO obs_module_text("Buffering.AutoDetect")
#define TEXT_BUFFERING_ON   obs_module_text("Buffering.Enable")
#define TEXT_BUFFERING_OFF  obs_module_text("Buffering.Disable")
#define TEXT_FLIP_IMAGE     obs_module_text("FlipVertically")
#define TEXT_AUTOROTATION   obs_module_text("Autorotation")
#define TEXT_HW_DECODE      obs_module_text("HardwareDecode")
#define TEXT_AUDIO_MODE     obs_module_text("AudioOutputMode")
#define TEXT_MODE_CAPTURE   obs_module_text("AudioOutputMode.Capture")
#define TEXT_MODE_DSOUND    obs_module_text("AudioOutputMode.DirectSound")
#define TEXT_MODE_WAVEOUT   obs_module_text("AudioOutputMode.WaveOut")
#define TEXT_CUSTOM_AUDIO   obs_module_text("UseCustomAudioDevice")
#define TEXT_AUDIO_DEVICE   obs_module_text("AudioDevice")
#define TEXT_ACTIVATE       obs_module_text("Activate")
#define TEXT_DEACTIVATE     obs_module_text("Deactivate")
#define TEXT_COLOR_SPACE    obs_module_text("ColorSpace")
#define TEXT_COLOR_DEFAULT  obs_module_text("ColorSpace.Default")
#define TEXT_COLOR_709      obs_module_text("ColorSpace.709")
#define TEXT_COLOR_601      obs_module_text("ColorSpace.601")
#define TEXT_COLOR_2100PQ   obs_module_text("ColorSpace.2100PQ")
#define TEXT_COLOR_2100HLG  obs_module_text("ColorSpace.2100HLG")
#define TEXT_COLOR_RANGE    obs_module_text("ColorRange")
#define TEXT_RANGE_DEFAULT  obs_module_text("ColorRange.Default")
#define TEXT_RANGE_PARTIAL  obs_module_text("ColorRange.Partial")
#define TEXT_RANGE_FULL     obs_module_text("ColorRange.Full")
#define TEXT_DWNS           obs_module_text("DeactivateWhenNotShowing")

/* clang-format on */

//PRISM/Xiewei/20230712/noissue/add log
#define do_log(level, format, ...)                                         \
	{                                                                  \
		blog(level, "[obs_camera] %p['%s']['%s'] " format, source, \
		     obs_source_get_name(source), video_device,            \
		     ##__VA_ARGS__);                                       \
	}
//PRISM/Xiewei/20230712/noissue/add log

//PRISM/FanZirong/20241203/PRISM_PC-1675/add log
#define do_logex(kr, level, fields, field_count, format, ...)                                         \
	{                                                                   \
		blogex(kr, level, fields,                                   \
		       field_count , "[obs_camera] %p['%s']['%s'] " format, \
		       source, \
		     obs_source_get_name(source), video_device,             \
		     ##__VA_ARGS__);                                        \
	}
//PRISM/FanZirong/20241203/PRISM_PC-1675/add log

static DStr GetVideoFormatName(VideoFormat format);

//PRISM/Xiewei/20220119/#11238/resolve ui block, close config dialog before exit
struct window_param {
	DWORD threadId = 0;
	DWORD processId = 0;
	HWND dialogWindow = NULL;
};

//PRISM/Xiewei/20220119/#11238/resolve ui block, close config dialog before exit
BOOL CALLBACK enum_windows_proc(HWND window, LPARAM lParam)
{
	struct window_param *data = (struct window_param *)(lParam);
	DWORD processId = 0;
	DWORD threadId = GetWindowThreadProcessId(window, &processId);
	if (!processId)
		return TRUE;

	if (processId != data->processId)
		return TRUE;

	if (threadId == data->threadId) {
		data->dialogWindow = window;
		PostMessageA(window, WM_CLOSE, NULL, NULL);
	}
	return TRUE;
}

enum ResType {
	ResType_Preferred,
	ResType_Custom,
};

enum class BufferingType : int64_t {
	Auto,
	On,
	Off,
};

void ffmpeg_log(void *bla, int level, const char *msg, va_list args)
{
	DStr str;
	if (level == AV_LOG_WARNING) {
		dstr_copy(str, "warning: ");
	} else if (level == AV_LOG_ERROR) {
		/* only print first of this message to avoid spam */
		static bool suppress_app_field_spam = false;
		if (strcmp(msg, "unable to decode APP fields: %s\n") == 0) {
			if (suppress_app_field_spam)
				return;

			suppress_app_field_spam = true;
		}

		dstr_copy(str, "error:   ");
	} else if (level < AV_LOG_ERROR) {
		dstr_copy(str, "fatal:   ");
	} else {
		return;
	}

	dstr_cat(str, msg);
	if (dstr_end(str) == '\n')
		dstr_resize(str, str->len - 1);

	//PRISM/Zengqin/20230726/none/silence log
	blogva(LOG_DEBUG, str, args);
	av_log_default_callback(bla, level, msg, args);
}

class Decoder {
	struct ffmpeg_decode decode;

public:
	inline Decoder() { memset(&decode, 0, sizeof(decode)); }
	inline ~Decoder() { ffmpeg_decode_free(&decode); }

	inline operator ffmpeg_decode *() { return &decode; }
	inline ffmpeg_decode *operator->() { return &decode; }
};

class CriticalSection {
	CRITICAL_SECTION mutex;

public:
	inline CriticalSection() { InitializeCriticalSection(&mutex); }
	inline ~CriticalSection() { DeleteCriticalSection(&mutex); }

	inline operator CRITICAL_SECTION *() { return &mutex; }
};

class CriticalScope {
	CriticalSection &mutex;

	CriticalScope() = delete;
	CriticalScope &operator=(CriticalScope &cs) = delete;

public:
	inline CriticalScope(CriticalSection &mutex_) : mutex(mutex_)
	{
		EnterCriticalSection(mutex);
	}

	inline ~CriticalScope() { LeaveCriticalSection(mutex); }
};

enum class Action {
	None,
	Activate,
	ActivateBlock,
	Deactivate,
	Shutdown,
	ConfigVideo,
	ConfigAudio,
	ConfigCrossbar1,
	ConfigCrossbar2,
};

static DWORD CALLBACK DShowThread(LPVOID ptr);

struct DShowInput {
	obs_source_t *source;
	Device device;
	bool deactivateWhenNotShowing = false;
	bool deviceHasAudio = false;
	bool deviceHasSeparateAudioFilter = false;
	bool flip = false;
	//PRISM/Xiewei/20240621/none/add more logs to trace video data
	std::atomic<bool> active = false;
	bool autorotation = true;
	bool hw_decode = false;
	//PRISM/Xiewei/20230712/noissue/add log
	std::atomic<bool> is_first_video = true;
	std::atomic<bool> is_first_audio = true;

	//PRISM/FanZirong/start/20240131/#3752/one of av device is lens, transform PTS to start from 0
	bool is_need_transform_pts = false;
	long long first_video_pts = 0;
	long long first_audio_pts = 0;
	//PRISM/FanZirong/end/20240131/#3752/one of av device is lens, transform PTS to start from 0

	//PRISM/Xiewei/202312125/noissue/add debug log start
#if defined(_DEBUG_AV_DATA)
	std::atomic<std::optional<int64_t>> m_lastAudioOriginPts;
	std::atomic<std::optional<int64_t>> m_lastAudioFramePts;
	std::atomic<std::optional<int64_t>> m_lastVideoOriginPts;
	std::atomic<std::optional<int64_t>> m_lastVideoFramePts;
#endif
	//PRISM/Xiewei/202312125/noissue/add debug log end
	char video_device[MAX_PATH];

	Decoder audio_decoder;
	Decoder video_decoder;

	VideoConfig videoConfig;
	AudioConfig audioConfig;

	enum video_colorspace cs;
	obs_source_frame2 frame;
	obs_source_audio audio;
	long lastRotation = 0;

	//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
	//WinHandle semaphore;
	WinHandle activated_event;
	WinHandle thread;
	CriticalSection mutex;
	vector<Action> actions;

	//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
	HANDLE thread_exit_event = nullptr;

	//PRISM/Xiewei/20220119/#11238/resolve ui block, close config dialog
	std::atomic<bool> showing_dialog = false;
	//PRISM/Zengqin/20231229/#3744/notify ui to not support HDR color space.
	std::atomic<bool> not_support_hdr = false;
	//PRISM/Xiewei/20240621/none/add more logs to trace video data start
	std::unique_ptr<DataMonitor> data_monitor;
	PLSMonitorCameraVideo monitor_video;
	int subscribeId = INVALID_NOTIFY_ID;
	//PRISM/Xiewei/20240621/none/add more logs to trace video data end

	//PRISM/FanZirong/20240823/PRISM_PC-1042/reduce log
	int decodeErrorNum = 0;

	inline void QueueAction(Action action)
	{
		CriticalScope scope(mutex);
		actions.push_back(action);

		//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
		//ReleaseSemaphore(semaphore, 1, nullptr);
	}

	inline void QueueActivate(obs_data_t *settings)
	{
		bool block =
			obs_data_get_bool(settings, "synchronous_activate");
		QueueAction(block ? Action::ActivateBlock : Action::Activate);
		if (block) {
			obs_data_erase(settings, "synchronous_activate");
			WaitForSingleObject(activated_event, INFINITE);
		}
	}

	inline DShowInput(obs_source_t *source_, obs_data_t *settings)
		: source(source_),
		  device(InitGraph::False)
	{
		memset(&audio, 0, sizeof(audio));
		memset(&frame, 0, sizeof(frame));

		av_log_set_level(AV_LOG_WARNING);
		av_log_set_callback(ffmpeg_log);

		//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
		/*semaphore = CreateSemaphore(nullptr, 0, 0x7FFFFFFF, nullptr);
		if (!semaphore)
			throw "Failed to create semaphore";*/

		activated_event = CreateEvent(nullptr, false, false, nullptr);
		if (!activated_event)
			throw "Failed to create activated_event";

		//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
		thread_exit_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (!thread_exit_event)
			throw "Failed to create thread_exit_event";
		ResetEvent(thread_exit_event);

		thread =
			CreateThread(nullptr, 0, DShowThread, this, 0, nullptr);
		if (!thread)
			throw "Failed to create thread";

		deactivateWhenNotShowing =
			obs_data_get_bool(settings, DEACTIVATE_WNS);

		if (obs_data_get_bool(settings, "active")) {
			bool showing = obs_source_showing(source);
			if (!deactivateWhenNotShowing || showing)
				QueueActivate(settings);

			active = true;
		}

		//PRISM/Xiewei/20240621/none/add more logs to trace video data start
		data_monitor = std::make_unique<DataMonitor>(
			3 * 1000,
			[this]() {
				if (active) {
					do_log(LOG_WARNING,
					       "Stop push frame for more than 3 seconds.",
					       (void *)source);
					data_monitor->Reset();
				}
			},
			(void *)source);
		//PRISM/Xiewei/20240621/none/add more logs to trace video data end
	}

	inline ~DShowInput()
	{
		//PRISM/Xiewei/20240621/none/add more logs to trace video data
		if (data_monitor) {
			data_monitor->Reset();
			data_monitor->Stop();
		}	

		{
			CriticalScope scope(mutex);
			actions.resize(1);
			actions[0] = Action::Shutdown;
		}

		//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
		//ReleaseSemaphore(semaphore, 1, nullptr);
		SetEvent(thread_exit_event);

		//PRISM/Xiewei/20220119/#11238/resolve ui block, close config dialog
		while (WAIT_OBJECT_0 != WaitForSingleObject(thread, 20)) {
			if (showing_dialog) {
				do_log(LOG_INFO,
				       "config dialog is showing, try to close it.");
				closeDialogWindow(GetCurrentProcessId(),
						  GetThreadId(thread));
			}
		}

		//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
		if (thread_exit_event &&
		    (thread_exit_event != INVALID_HANDLE_VALUE)) {
			::CloseHandle(thread_exit_event);
			thread_exit_event = nullptr;
		}

		//PRISM/Xiewei/20240621/none/add more logs to trace video data
		monitor_video.SendCatchLogs();

		//PRISM/Xiewei/20240621/none/add more logs to trace video data
		if (data_monitor) {
			data_monitor.reset();
		}

		//PRISM/FanZirong/20240920/none/#6103/put it after the thread ends to avoid crash
		//PRISM/Xiewei/20240621/none/add more logs to trace video data
		DeviceNotification::instance()->Unsubscribe(subscribeId);
	}

	void OnEncodedVideoData(enum AVCodecID id, unsigned char *data,
				size_t size, long long ts);
	void OnEncodedAudioData(enum AVCodecID id, unsigned char *data,
				size_t size, long long ts);

	void OnReactivate();
	void OnVideoData(const VideoConfig &config, unsigned char *data,
			 size_t size, long long startTime, long long endTime,
			 long rotation);
	void OnAudioData(const AudioConfig &config, unsigned char *data,
			 size_t size, long long startTime, long long endTime);

	bool UpdateVideoConfig(obs_data_t *settings);
	bool UpdateAudioConfig(obs_data_t *settings);
	void SetActive(bool active);
	inline enum video_colorspace GetColorSpace(obs_data_t *settings) const;
	inline enum video_range_type GetColorRange(obs_data_t *settings) const;
	inline bool Activate(obs_data_t *settings);
	inline void Deactivate();

	inline void SetupBuffering(obs_data_t *settings);

	void DShowLoop();

	//PRISM/Xiewei/20220119/#11238/resolve ui block, close config dialog before exit
	void closeDialogWindow(DWORD processId, DWORD threadId);
};

static DWORD CALLBACK DShowThread(LPVOID ptr)
{
	DShowInput *dshowInput = (DShowInput *)ptr;

	os_set_thread_name("win-dshow: DShowThread");

	CoInitialize(nullptr);
	dshowInput->DShowLoop();
	CoUninitialize();
	return 0;
}

static inline void ProcessMessages()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

/* Always keep directshow in a single thread for a given device */
void DShowInput::DShowLoop()
{
	while (true) {
		//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
		/*DWORD ret = MsgWaitForMultipleObjects(1, &semaphore, false,
						      INFINITE, QS_ALLINPUT);
		if (ret == (WAIT_OBJECT_0 + 1)) {
			ProcessMessages();
			continue;
		} else if (ret != WAIT_OBJECT_0) {
			break;
		}*/

		//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
		if (WAIT_OBJECT_0 == WaitForSingleObject(thread_exit_event, 5))
			break;

		Action action = Action::None;
		{
			CriticalScope scope(mutex);
			if (actions.size()) {
				action = actions.front();
				actions.erase(actions.begin());
			}
		}

		switch (action) {
		case Action::Activate:
		case Action::ActivateBlock: {
			bool block = action == Action::ActivateBlock;

			obs_data_t *settings;
			settings = obs_source_get_settings(source);

			//PRISM/Xiewei/20240315/#4686/catch an internal exception
			try{
			if (!Activate(settings)) {
				obs_source_output_video2(source, nullptr);
			}
			} catch (...){
				do_log(LOG_WARNING, "Failed to Activate device: An internal exception happend.");
				obs_source_output_video2(source, nullptr);
			}
			if (block)
				SetEvent(activated_event);
			obs_data_release(settings);
			break;
		}

		case Action::Deactivate:
			Deactivate();
			break;

		case Action::Shutdown:
			device.ShutdownGraph();
			return;

		case Action::ConfigVideo:
			//PRISM/Xiewei/20230829/#11707/check device validation before open config dialog
			if (!device.Valid())
				break;
			showing_dialog = true;
			device.OpenDialog(nullptr, DialogType::ConfigVideo);
			showing_dialog = false;
			break;

		case Action::ConfigAudio:
			//PRISM/Xiewei/20230829/#11707/check device validation before open config dialog
			if (!device.Valid())
				break;
			showing_dialog = true;
			device.OpenDialog(nullptr, DialogType::ConfigAudio);
			showing_dialog = false;
			break;

		case Action::ConfigCrossbar1:
			//PRISM/Xiewei/20230829/#11707/check device validation before open config dialog
			if (!device.Valid())
				break;
			showing_dialog = true;
			device.OpenDialog(nullptr, DialogType::ConfigCrossbar);
			showing_dialog = false;
			break;

		case Action::ConfigCrossbar2:
			//PRISM/Xiewei/20230829/#11707/check device validation before open config dialog
			if (!device.Valid())
				break;
			showing_dialog = true;
			device.OpenDialog(nullptr, DialogType::ConfigCrossbar2);
			showing_dialog = false;
			break;

		case Action::None:;
		}
		//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
		ProcessMessages();
	}
	//PRISM/FanZirong/20241113/PRISM_PC_NELO-45/avoid block
	device.ShutdownGraph();
}

//PRISM/Xiewei/20220119/#11238/resolve ui block, close config dialog before exit
void DShowInput::closeDialogWindow(DWORD processId, DWORD threadId)
{
	window_param param;
	param.threadId = threadId;
	param.processId = processId;
	EnumWindows(enum_windows_proc, (LPARAM)&param);
	if (NULL == param.dialogWindow) {
		do_log(LOG_WARNING,
		       "Could not find config dialog, dshowloop thread will not stop \
			until users close config dialog manually.");
		assert(false && "close config dialog failed");
	}
}

#define FPS_HIGHEST 0LL
#define FPS_MATCHING -1LL

template<typename T, typename U, typename V>
static bool between(T &&lower, U &&value, V &&upper)
{
	return value >= lower && value <= upper;
}

static bool ResolutionAvailable(const VideoInfo &cap, int cx, int cy)
{
	return between(cap.minCX, cx, cap.maxCX) &&
	       between(cap.minCY, cy, cap.maxCY);
}

#define DEVICE_INTERVAL_DIFF_LIMIT 20

static bool FrameRateAvailable(const VideoInfo &cap, long long interval)
{
	return interval == FPS_HIGHEST || interval == FPS_MATCHING ||
	       between(cap.minInterval - DEVICE_INTERVAL_DIFF_LIMIT, interval,
		       cap.maxInterval + DEVICE_INTERVAL_DIFF_LIMIT);
}

static long long FrameRateInterval(const VideoInfo &cap,
				   long long desired_interval)
{
	return desired_interval < cap.minInterval
		       ? cap.minInterval
		       : min(desired_interval, cap.maxInterval);
}

static inline video_format ConvertVideoFormat(VideoFormat format)
{
	switch (format) {
	case VideoFormat::ARGB:
		return VIDEO_FORMAT_BGRA;
	case VideoFormat::XRGB:
		return VIDEO_FORMAT_BGRX;
	case VideoFormat::I420:
		return VIDEO_FORMAT_I420;
	case VideoFormat::YV12:
		return VIDEO_FORMAT_I420;
	case VideoFormat::NV12:
		return VIDEO_FORMAT_NV12;
	case VideoFormat::Y800:
		return VIDEO_FORMAT_Y800;
	case VideoFormat::YVYU:
		return VIDEO_FORMAT_YVYU;
	case VideoFormat::YUY2:
		return VIDEO_FORMAT_YUY2;
	case VideoFormat::UYVY:
		return VIDEO_FORMAT_UYVY;
	case VideoFormat::HDYC:
		return VIDEO_FORMAT_UYVY;
	case VideoFormat::P010:
		return VIDEO_FORMAT_P010;
	default:
		return VIDEO_FORMAT_NONE;
	}
}

static inline audio_format ConvertAudioFormat(AudioFormat format)
{
	switch (format) {
	case AudioFormat::Wave16bit:
		return AUDIO_FORMAT_16BIT;
	case AudioFormat::WaveFloat:
		return AUDIO_FORMAT_FLOAT;
	default:
		return AUDIO_FORMAT_UNKNOWN;
	}
}

static inline enum speaker_layout convert_speaker_layout(uint8_t channels)
{
	switch (channels) {
	case 0:
		return SPEAKERS_UNKNOWN;
	case 1:
		return SPEAKERS_MONO;
	case 2:
		return SPEAKERS_STEREO;
	case 3:
		return SPEAKERS_2POINT1;
	case 4:
		return SPEAKERS_4POINT0;
	case 5:
		return SPEAKERS_4POINT1;
	case 6:
		return SPEAKERS_5POINT1;
	case 8:
		return SPEAKERS_7POINT1;
	default:
		return SPEAKERS_UNKNOWN;
	}
}

//#define LOG_ENCODED_VIDEO_TS 1
//#define LOG_ENCODED_AUDIO_TS 1

void DShowInput::OnEncodedVideoData(enum AVCodecID id, unsigned char *data,
				    size_t size, long long ts)
{
	/* If format or hw decode changes, recreate the decoder */
	if (ffmpeg_decode_valid(video_decoder) &&
	    ((video_decoder->codec->id != id) ||
	     (video_decoder->hw != hw_decode))) {
		ffmpeg_decode_free(video_decoder);
	}

	if (!ffmpeg_decode_valid(video_decoder)) {
		if (ffmpeg_decode_init(video_decoder, id, hw_decode) < 0) {
			blog(LOG_WARNING,
			     "%s: Could not initialize video decoder",
			     obs_source_get_name(source));
			return;
		}
	}

	bool got_output;
	bool success = ffmpeg_decode_video(video_decoder, data, size, &ts, cs,
					   frame.range, &frame, &got_output);
	if (!success) {
		//PRISM/Xiewei/20230712/none/silence log
		//PRISM/FanZirong/20240823/PRISM_PC-1042/reduce log
		if (decodeErrorNum % 10000 == 0) {
			do_log(LOG_DEBUG, "Error decoding video");
		}
		decodeErrorNum++;
		return;
	}

	if (got_output) {
		frame.timestamp = (uint64_t)ts * 100;
		if (flip)
			frame.flip = !frame.flip;
#if LOG_ENCODED_VIDEO_TS
		blog(LOG_DEBUG, "video ts: %llu", frame.timestamp);
#endif
		obs_source_output_video2(source, &frame);
	}
}

void DShowInput::OnReactivate()
{
	SetActive(true);
}

void DShowInput::OnVideoData(const VideoConfig &config, unsigned char *data,
			     size_t size, long long startTime,
			     long long endTime, long rotation)
{
	//PRISM/Xiewei/20230712/noissue/add log
	if (is_first_video) {
		is_first_video = false;
		DStr fmt = GetVideoFormatName(videoConfig.format);
		//PRISM/FanZirong/start/20240131/#3752/one of av device is lens, transform PTS to start from 0
		first_video_pts = startTime;

		char source_p[50];
		snprintf(source_p, sizeof(source_p), "%p", source);
		const char *fields[][2] = {{PTS_LOG_TYPE, PTS_TYPE_EVENT},
					   {"source", source_p}};
		do_logex(false, LOG_INFO, fields, 2,
			 "first video received. %dx%d %s size:%llu", config.cx,
			 config.cy_abs, fmt->array, (ULONG64)size);

		//PRISM/Xiewei/20240621/none/add more logs to trace video data
		if (data_monitor)
			data_monitor->Resum();
		monitor_video.Reset();
	}

	//PRISM/Xiewei/20240621/none/add more logs to trace video data
	if (data_monitor)
		data_monitor->PushData();
	monitor_video.BeginHandleVideo();
	std::shared_ptr<int> autoRun(nullptr, [this](int *) {
		monitor_video.EndHandleVideo();
	});

	//PRISM/FanZirong/start/20240131/#3752/one of av device is lens, transform PTS to start from 0
	if (is_need_transform_pts) {
		startTime -= first_video_pts;
		endTime -= first_video_pts;
	}
	//PRISM/FanZirong/end/20240131/#3752/one of av device is lens, transform PTS to start from 0

	//PRISM/Xiewei/202312125/noissue/add debug log start
#if defined(_DEBUG_AV_DATA)
	auto now = std::chrono::steady_clock::now().time_since_epoch().count();
	if (!m_lastVideoOriginPts.load().has_value()) {
		m_lastVideoOriginPts.store(now);
	}

	if (!m_lastVideoFramePts.load().has_value()) {
		m_lastVideoFramePts = startTime;
	}

	auto sysSpace = (now - m_lastVideoOriginPts.load().value()) / 1000000;
	auto dataSpace =
		(startTime - m_lastVideoFramePts.load().value()) / 10000;
	if (sysSpace > dataSpace && sysSpace - dataSpace >= 30) {
		do_log(LOG_INFO,
		       "-------------- Video frame late, steady time gap: %lld, frame pts gap: %lld\n",
		       sysSpace, dataSpace);
	}
	m_lastVideoFramePts = startTime;
	m_lastVideoOriginPts.store(now);
#endif
	//PRISM/Xiewei/202312125/noissue/add debug log end

	if (autorotation && rotation != lastRotation) {
		lastRotation = rotation;
		obs_source_set_async_rotation(source, rotation);
	}

	if (videoConfig.format == VideoFormat::H264) {
		OnEncodedVideoData(AV_CODEC_ID_H264, data, size, startTime);
		return;
	}

#ifdef ENABLE_HEVC
	if (videoConfig.format == VideoFormat::HEVC) {
		OnEncodedVideoData(AV_CODEC_ID_HEVC, data, size, startTime);
		return;
	}
#endif

	if (videoConfig.format == VideoFormat::MJPEG) {
		OnEncodedVideoData(AV_CODEC_ID_MJPEG, data, size, startTime);
		return;
	}

	const int cx = config.cx;
	const int cy_abs = config.cy_abs;

	frame.timestamp = (uint64_t)startTime * 100;
	frame.width = config.cx;
	frame.height = cy_abs;
	frame.format = ConvertVideoFormat(config.format);
	frame.flip = flip;
	frame.flags = OBS_SOURCE_FRAME_LINEAR_ALPHA;

	/* YUV DIBS are always top-down */
	if (config.format == VideoFormat::XRGB ||
	    config.format == VideoFormat::ARGB) {
		/* RGB DIBs are bottom-up by default */
		if (!config.cy_flip)
			frame.flip = !frame.flip;
	}

	if (videoConfig.format == VideoFormat::XRGB ||
	    videoConfig.format == VideoFormat::ARGB) {
		frame.data[0] = data;
		frame.linesize[0] = cx * 4;

		//PRISM//Xiewei/20230823/#2243/check size
		int expect_size = frame.linesize[0] * cy_abs;
		if (size != expect_size)
			return;

	} else if (videoConfig.format == VideoFormat::YVYU ||
		   videoConfig.format == VideoFormat::YUY2 ||
		   videoConfig.format == VideoFormat::HDYC ||
		   videoConfig.format == VideoFormat::UYVY) {
		frame.data[0] = data;
		frame.linesize[0] = cx * 2;

		//PRISM//Xiewei/20230823/#2243/check size
		int expect_size = frame.linesize[0] * cy_abs;
		if (size != expect_size)
			return;

	} else if (videoConfig.format == VideoFormat::I420) {
		frame.data[0] = data;
		frame.data[1] = frame.data[0] + (cx * cy_abs);
		frame.data[2] = frame.data[1] + (cx * cy_abs / 4);
		frame.linesize[0] = cx;
		frame.linesize[1] = cx / 2;
		frame.linesize[2] = cx / 2;

		//PRISM//Xiewei/20230823/#2243/check size
		int expect_size = frame.linesize[0] * cy_abs +
				  frame.linesize[1] * cy_abs / 2 +
				  frame.linesize[2] * cy_abs / 2;
		if (size != expect_size)
			return;

	} else if (videoConfig.format == VideoFormat::YV12) {
		frame.data[0] = data;
		frame.data[2] = frame.data[0] + (cx * cy_abs);
		frame.data[1] = frame.data[2] + (cx * cy_abs / 4);
		frame.linesize[0] = cx;
		frame.linesize[1] = cx / 2;
		frame.linesize[2] = cx / 2;

		//PRISM//Xiewei/20230823/#2243/check size
		int expect_size = frame.linesize[0] * cy_abs +
				  frame.linesize[1] * cy_abs / 2 +
				  frame.linesize[2] * cy_abs / 2;
		if (size != expect_size)
			return;

	} else if (videoConfig.format == VideoFormat::NV12) {
		frame.data[0] = data;
		frame.data[1] = frame.data[0] + (cx * cy_abs);
		frame.linesize[0] = cx;
		frame.linesize[1] = cx;

		//PRISM//Xiewei/20230823/#2243/check size
		int expect_size = frame.linesize[0] * cy_abs +
				  frame.linesize[1] * cy_abs / 2;
		if (size != expect_size)
			return;

	} else if (videoConfig.format == VideoFormat::Y800) {
		frame.data[0] = data;
		frame.linesize[0] = cx;

		//PRISM//Xiewei/20230823/#2243/check size
		int expect_size = frame.linesize[0] * cy_abs;
		if (size != expect_size)
			return;

	} else if (videoConfig.format == VideoFormat::P010) {
		frame.data[0] = data;
		frame.data[1] = frame.data[0] + (cx * cy_abs) * 2;
		frame.linesize[0] = cx * 2;
		frame.linesize[1] = cx * 2;

		//PRISM//Xiewei/20230823/#2243/check size
		int expect_size = frame.linesize[0] * cy_abs +
				  frame.linesize[1] * cy_abs / 2;
		if (size != expect_size)
			return;

	} else {
		/* TODO: other formats */
		return;
	}

	obs_source_output_video2(source, &frame);

	UNUSED_PARAMETER(endTime); /* it's the enndd tiimmes! */
}

void DShowInput::OnEncodedAudioData(enum AVCodecID id, unsigned char *data,
				    size_t size, long long ts)
{
	if (!ffmpeg_decode_valid(audio_decoder)) {
		if (ffmpeg_decode_init(audio_decoder, id, false) < 0) {
			blog(LOG_WARNING,
			     "%s: Could not initialize audio decoder",
			     obs_source_get_name(source));
			return;
		}
	}

	bool got_output = false;
	do {
		bool success = ffmpeg_decode_audio(audio_decoder, data, size,
						   &audio, &got_output);
		if (!success) {
			blog(LOG_WARNING, "%s: Error decoding audio",
			     obs_source_get_name(source));
			return;
		}

		if (got_output) {
			audio.timestamp = (uint64_t)ts * 100;
#if LOG_ENCODED_AUDIO_TS
			blog(LOG_DEBUG, "audio ts: %llu", audio.timestamp);
#endif
			obs_source_output_audio(source, &audio);
		} else {
			break;
		}

		ts += int64_t(audio_decoder->frame->nb_samples) * 10000000LL /
		      int64_t(audio_decoder->frame->sample_rate);
		size = 0;
		data = nullptr;
	} while (got_output);
}

void DShowInput::OnAudioData(const AudioConfig &config, unsigned char *data,
			     size_t size, long long startTime,
			     long long endTime)
{
	//PRISM/Xiewei/20230712/noissue/add log
	if (is_first_audio) {
		is_first_audio = false;
		//PRISM/FanZirong/start/20240131/#3752/one of av device is lens, transform PTS to start from 0
		first_audio_pts = startTime;

		//PRISM/FanZirong/20241203/PRISM_PC-1675/add log fields
		char source_p[50];
		snprintf(source_p, sizeof(source_p), "%p", source);
		const char *fields[][2] = {{PTS_LOG_TYPE, PTS_TYPE_EVENT},
					   {"source", source_p}};
		do_logex(false, LOG_INFO, fields, 2,
			 "first audio received. channels: %d, format: %d",
			 config.channels, config.format);
	}
	//PRISM/FanZirong/start/20240131/#3752/one of av device is lens, transform PTS to start from 0
	if (is_need_transform_pts) {
		startTime -= first_audio_pts;
		endTime -= first_audio_pts;
	}
	//PRISM/FanZirong/end/20240131/#3752/one of av device is lens, transform PTS to start from 0

	//PRISM/Xiewei/202312125/noissue/add debug log start
#if defined(_DEBUG_AV_DATA)
	auto now = std::chrono::steady_clock::now().time_since_epoch().count();
	if (!m_lastAudioOriginPts.load().has_value()) {
		m_lastAudioOriginPts.store(now);
	}

	if (!m_lastAudioFramePts.load().has_value()) {
		m_lastAudioFramePts = startTime;
	}

	auto sysSpace = (now - m_lastAudioOriginPts.load().value()) / 1000000;
	auto dataSpace =
		(startTime - m_lastAudioFramePts.load().value()) / 10000;
	if (sysSpace > dataSpace && sysSpace - dataSpace >= 30) {
		do_log(LOG_INFO,
		       "++++++++++++++ Audio frame late, steady time gap: %lld, frame pts gap: %lld\n",
		       sysSpace, dataSpace);
	}
	m_lastAudioFramePts = startTime;
	m_lastAudioOriginPts.store(now);
#endif
	//PRISM/Xiewei/202312125/noissue/add debug log end

	size_t block_size;

	if (config.format == AudioFormat::AAC) {
		OnEncodedAudioData(AV_CODEC_ID_AAC, data, size, startTime);
		return;
	} else if (config.format == AudioFormat::AC3) {
		OnEncodedAudioData(AV_CODEC_ID_AC3, data, size, startTime);
		return;
	} else if (config.format == AudioFormat::MPGA) {
		OnEncodedAudioData(AV_CODEC_ID_MP2, data, size, startTime);
		return;
	}

	audio.speakers = convert_speaker_layout((uint8_t)config.channels);
	audio.format = ConvertAudioFormat(config.format);
	audio.samples_per_sec = (uint32_t)config.sampleRate;
	audio.data[0] = data;

	block_size = get_audio_bytes_per_channel(audio.format) *
		     get_audio_channels(audio.speakers);

	//PRISM/Xiewei/20240520/#5406/check zero
	if (!block_size){
		return;
	}
	audio.frames = (uint32_t)(size / block_size);
	audio.timestamp = (uint64_t)startTime * 100;

	if (audio.format != AUDIO_FORMAT_UNKNOWN)
		obs_source_output_audio(source, &audio);

	UNUSED_PARAMETER(endTime);
}

struct PropertiesData {
	DShowInput *input;
	vector<VideoDevice> devices;
	vector<AudioDevice> audioDevices;

	bool GetDevice(VideoDevice &device, const char *encoded_id) const
	{
		DeviceId deviceId;
		DecodeDeviceId(deviceId, encoded_id);

		for (const VideoDevice &curDevice : devices) {
			if (deviceId.name == curDevice.name &&
			    deviceId.path == curDevice.path) {
				device = curDevice;
				return true;
			}
		}

		return false;
	}
};

static inline bool ConvertRes(int &cx, int &cy, const char *res)
{
	return sscanf(res, "%dx%d", &cx, &cy) == 2;
}

static inline bool FormatMatches(VideoFormat left, VideoFormat right)
{
	return left == VideoFormat::Any || right == VideoFormat::Any ||
	       left == right;
}

static inline bool ResolutionValid(const string &res, int &cx, int &cy)
{
	if (!res.size())
		return false;

	return ConvertRes(cx, cy, res.c_str());
}

static inline bool CapsMatch(const VideoInfo &)
{
	return true;
}

template<typename... F> static bool CapsMatch(const VideoDevice &dev, F... fs);

template<typename F, typename... Fs>
static inline bool CapsMatch(const VideoInfo &info, F &&f, Fs... fs)
{
	return f(info) && CapsMatch(info, fs...);
}

template<typename... F> static bool CapsMatch(const VideoDevice &dev, F... fs)
{
	// no early exit, trigger all side effects.
	bool match = false;
	for (const VideoInfo &info : dev.caps)
		if (CapsMatch(info, fs...))
			match = true;
	return match;
}

static inline bool MatcherMatchVideoFormat(VideoFormat format, bool &did_match,
					   const VideoInfo &info)
{
	bool match = FormatMatches(format, info.format);
	did_match = did_match || match;
	return match;
}

static inline bool MatcherClosestFrameRateSelector(long long interval,
						   long long &best_match,
						   const VideoInfo &info)
{
	long long current = FrameRateInterval(info, interval);
	if (llabs(interval - best_match) > llabs(interval - current))
		best_match = current;
	return true;
}

#if 0
auto ResolutionMatcher = [](int cx, int cy)
{
	return [cx, cy](const VideoInfo &info)
	{
		return ResolutionAvailable(info, cx, cy);
	};
};

auto FrameRateMatcher = [](long long interval)
{
	return [interval](const VideoInfo &info)
	{
		return FrameRateAvailable(info, interval);
	};
};

auto VideoFormatMatcher = [](VideoFormat format, bool &did_match)
{
	return [format, &did_match](const VideoInfo &info)
	{
		return MatcherMatchVideoFormat(format, did_match, info);
	};
};

auto ClosestFrameRateSelector = [](long long interval, long long &best_match)
{
	return [interval, &best_match](const VideoInfo &info) mutable -> bool
	{
		MatcherClosestFrameRateSelector(interval, best_match, info);
	};
}
#else
#define ResolutionMatcher(cx, cy)                         \
	[cx, cy](const VideoInfo &info) -> bool {         \
		return ResolutionAvailable(info, cx, cy); \
	}
#define FrameRateMatcher(interval)                         \
	[interval](const VideoInfo &info) -> bool {        \
		return FrameRateAvailable(info, interval); \
	}
#define VideoFormatMatcher(format, did_match)                            \
	[format, &did_match](const VideoInfo &info) mutable -> bool {    \
		return MatcherMatchVideoFormat(format, did_match, info); \
	}
#define ClosestFrameRateSelector(interval, best_match)                       \
	[interval, &best_match](const VideoInfo &info) mutable -> bool {     \
		return MatcherClosestFrameRateSelector(interval, best_match, \
						       info);                \
	}
#endif

static bool ResolutionAvailable(const VideoDevice &dev, int cx, int cy)
{
	return CapsMatch(dev, ResolutionMatcher(cx, cy));
}

static bool DetermineResolution(int &cx, int &cy, obs_data_t *settings,
				VideoDevice &dev)
{
	const char *res = obs_data_get_autoselect_string(settings, RESOLUTION);
	if (obs_data_has_autoselect_value(settings, RESOLUTION) &&
	    ConvertRes(cx, cy, res) && ResolutionAvailable(dev, cx, cy))
		return true;

	res = obs_data_get_string(settings, RESOLUTION);
	if (ConvertRes(cx, cy, res) && ResolutionAvailable(dev, cx, cy))
		return true;

	res = obs_data_get_string(settings, LAST_RESOLUTION);
	if (ConvertRes(cx, cy, res) && ResolutionAvailable(dev, cx, cy))
		return true;

	return false;
}

static long long GetOBSFPS();

static inline bool IsDelayedDevice(const VideoConfig &config)
{
	return config.format > VideoFormat::MJPEG ||
	       wstrstri(config.name.c_str(), L"elgato game capture hd") !=
		       nullptr ||
	       wstrstri(config.name.c_str(), L"stream engine") != nullptr;
}

static inline bool IsDecoupled(const VideoConfig &config)
{
	return wstrstri(config.name.c_str(), L"GV-USB2") != NULL;
}

inline void DShowInput::SetupBuffering(obs_data_t *settings)
{
	BufferingType bufType;
	bool useBuffering;

	bufType = (BufferingType)obs_data_get_int(settings, BUFFERING_VAL);

	if (bufType == BufferingType::Auto)
		useBuffering = IsDelayedDevice(videoConfig);
	else
		useBuffering = bufType == BufferingType::On;

	obs_source_set_async_unbuffered(source, !useBuffering);
	obs_source_set_async_decoupled(source, IsDecoupled(videoConfig));
}

//PRISM/Xiewei/20230712/noissue/add log
//static DStr GetVideoFormatName(VideoFormat format);

//PRISM/Xiewei/20230616/prism cam issues: 748/reorder devicel ist
static bool EnumReorderedVideoDevices(std::vector<VideoDevice> &devices)
{
	bool ok = Device::EnumVideoDevices(devices);
	if (!ok)
		return false;

	static const int vcam_number = 4;
	VideoDevice prism_vcam[vcam_number] = {};
	const wchar_t *device_name[vcam_number] = {L"PRISM Live Studio",
						   L"PRISM Lens 1",
						   L"PRISM Lens 2",
						   L"PRISM Lens 3"};

	auto find_func = [device_name](const wchar_t *name) {
		for (int i = 0; i < vcam_number; i++) {
			if (wstrcmpi(name, device_name[i]) == 0)
				return i;
		}
		return -1;
	};

	size_t idx = 0;
	for (auto iter = devices.begin(); iter != devices.end();) {
		auto i = find_func((*iter).name.c_str());
		if (i != -1) {
			if (i != 0)
				prism_vcam[i] = (*iter);
			iter = devices.erase(iter);
			continue;
		}
		iter++;
	}

	for (int i = vcam_number - 1; i >= 0; i--) {
		if (!prism_vcam[i].name.empty())
			devices.insert(devices.begin(), prism_vcam[i]);
	}

	return true;
}

//PRISM/Xiewei/20230906/voc/set ARGB as default format for PRISM lens devices
static bool IsPRISMLensDevice(const std::wstring &device)
{
	const static std::set<std::wstring> lens = {
		L"PRISM Lens 1", L"PRISM Lens 2", L"PRISM Lens 3"};
	return (lens.find(device) != lens.end());
}

bool DShowInput::UpdateVideoConfig(obs_data_t *settings)
{
	string video_device_id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	deactivateWhenNotShowing = obs_data_get_bool(settings, DEACTIVATE_WNS);
	flip = obs_data_get_bool(settings, FLIP_IMAGE);
	autorotation = obs_data_get_bool(settings, AUTOROTATION);
	hw_decode = obs_data_get_bool(settings, HW_DECODE);

	DeviceId id;
	if (!DecodeDeviceId(id, video_device_id.c_str())) {
		blog(LOG_WARNING, "%s: DecodeDeviceId failed",
		     obs_source_get_name(source));
		return false;
	}

	//PRISM/Xiewei/20230712/noissue/add log
	BPtr<char> name_utf8;
	BPtr<char> path_utf8;
	os_wcs_to_utf8_ptr(id.name.c_str(), id.name.size(), &name_utf8);
	os_wcs_to_utf8_ptr(id.path.c_str(), id.path.size(), &path_utf8);
	sprintf_s(video_device, "%s", (const char *)name_utf8);

	PropertiesData data;
	//PRISM/Xiewei/20230616/prism cam issues: 748 /reorder devicel ist
	EnumReorderedVideoDevices(data.devices);
	VideoDevice dev;
	if (!data.GetDevice(dev, video_device_id.c_str())) {
		//PRISM/Xiewei/20230712/noissue/add log
		do_log(LOG_WARNING, "%s: data.GetDevice failed",
		       obs_source_get_name(source));
		return false;
	}

	int resType = (int)obs_data_get_int(settings, RES_TYPE);
	int cx = 0, cy = 0;
	long long interval = 0;
	VideoFormat format = VideoFormat::Any;

	if (resType == ResType_Custom) {
		bool has_autosel_val;
		string resolution = obs_data_get_string(settings, RESOLUTION);
		if (!ResolutionValid(resolution, cx, cy)) {
			//PRISM/Xiewei/20230712/noissue/add log
			do_log(LOG_WARNING, "%s: ResolutionValid failed, %s",
			       obs_source_get_name(source), resolution.c_str());
			return false;
		}

		has_autosel_val =
			obs_data_has_autoselect_value(settings, FRAME_INTERVAL);
		interval = has_autosel_val
				   ? obs_data_get_autoselect_int(settings,
								 FRAME_INTERVAL)
				   : obs_data_get_int(settings, FRAME_INTERVAL);

		if (interval == FPS_MATCHING)
			interval = GetOBSFPS();

		format = (VideoFormat)obs_data_get_int(settings, VIDEO_FORMAT);

		long long best_interval = numeric_limits<long long>::max();
		bool video_format_match = false;
		bool caps_match = CapsMatch(
			dev, ResolutionMatcher(cx, cy),
			VideoFormatMatcher(format, video_format_match),
			ClosestFrameRateSelector(interval, best_interval),
			FrameRateMatcher(interval));

		if (!caps_match && !video_format_match) {
			//PRISM/Xiewei/20230712/noissue/add log
			do_log(LOG_WARNING, "%s: Video format match failed, %s",
			       obs_source_get_name(source),
			       GetVideoFormatName(format)->array);
			return false;
		}

		interval = best_interval;
	}

	videoConfig.name = id.name;
	videoConfig.path = id.path;
	videoConfig.useDefaultConfig = resType == ResType_Preferred;
	videoConfig.cx = cx;
	videoConfig.cy_abs = abs(cy);
	videoConfig.cy_flip = cy < 0;
	videoConfig.frameInterval = interval;
	//PRISM/Xiewei/20230906/voc/set ARGB as default format for PRISM lens devices
	if (IsPRISMLensDevice(id.name) && VideoFormat::Any == format) {
		videoConfig.useDefaultConfig = false;
		videoConfig.internalFormat = VideoFormat::ARGB;
	} else {
		videoConfig.internalFormat = format;
	}

	deviceHasAudio = dev.audioAttached;
	deviceHasSeparateAudioFilter = dev.separateAudioFilter;

	videoConfig.callback = std::bind(&DShowInput::OnVideoData, this,
					 placeholders::_1, placeholders::_2,
					 placeholders::_3, placeholders::_4,
					 placeholders::_5, placeholders::_6);
	videoConfig.reactivateCallback =
		std::bind(&DShowInput::OnReactivate, this);

	videoConfig.format = videoConfig.internalFormat;

	if (!device.SetVideoConfig(&videoConfig)) {
		//PRISM/Xiewei/20230712/noissue/add log
		do_log(LOG_WARNING, "%s: device.SetVideoConfig failed",
		       obs_source_get_name(source));
		return false;
	}

	DStr formatName = GetVideoFormatName(videoConfig.internalFormat);

	double fps = 0.0;

	if (videoConfig.frameInterval)
		fps = 10000000.0 / double(videoConfig.frameInterval);

	//PRISM/Xiewei/20230712/noissue/add log
	/*
	BPtr<char> name_utf8;
	BPtr<char> path_utf8;
	os_wcs_to_utf8_ptr(videoConfig.name.c_str(), videoConfig.name.size(),
			   &name_utf8);
	os_wcs_to_utf8_ptr(videoConfig.path.c_str(), videoConfig.path.size(),
			   &path_utf8);
	*/

	SetupBuffering(settings);

	blog(LOG_INFO, "---------------------------------");
	blog(LOG_INFO,
	     "[DShow Device: '%s'] settings updated: \n"
	     "\tvideo device: %s\n"
	     "\tvideo path: %s\n"
	     "\tresolution: %dx%d\n"
	     "\tflip: %d\n"
	     "\tfps: %0.2f (interval: %lld)\n"
	     "\tformat: %s\n"
	     "\tbuffering: %s\n"
	     "\thardware decode: %s",
	     obs_source_get_name(source), (const char *)name_utf8,
	     (const char *)path_utf8, videoConfig.cx, videoConfig.cy_abs,
	     (int)videoConfig.cy_flip, fps, videoConfig.frameInterval,
	     formatName->array,
	     obs_source_async_unbuffered(source) ? "disabled" : "enabled",
	     hw_decode ? "enabled" : "disabled");

	return true;
}

bool DShowInput::UpdateAudioConfig(obs_data_t *settings)
{
	string audio_device_id = obs_data_get_string(settings, AUDIO_DEVICE_ID);
	bool useCustomAudio = obs_data_get_bool(settings, USE_CUSTOM_AUDIO);

	if (useCustomAudio) {
		DeviceId id;
		if (!DecodeDeviceId(id, audio_device_id.c_str())) {
			obs_source_set_audio_active(source, false);
			return false;
		}

		audioConfig.name = id.name;
		audioConfig.path = id.path;

	} else if (!deviceHasAudio) {
		obs_source_set_audio_active(source, false);
		return true;
	}

	audioConfig.useVideoDevice = !useCustomAudio &&
				     !deviceHasSeparateAudioFilter;
	audioConfig.useSeparateAudioFilter = deviceHasSeparateAudioFilter;

	audioConfig.callback = std::bind(&DShowInput::OnAudioData, this,
					 placeholders::_1, placeholders::_2,
					 placeholders::_3, placeholders::_4,
					 placeholders::_5);

	audioConfig.mode =
		(AudioMode)obs_data_get_int(settings, AUDIO_OUTPUT_MODE);

	bool success = device.SetAudioConfig(&audioConfig);
	if (!success) {
		obs_source_set_audio_active(source, false);
		return false;
	}

	obs_source_set_audio_active(source, true);

	BPtr<char> name_utf8;
	os_wcs_to_utf8_ptr(audioConfig.name.c_str(), audioConfig.name.size(),
			   &name_utf8);

	blog(LOG_INFO, "\tusing video device audio: %s",
	     audioConfig.useVideoDevice ? "yes" : "no");

	if (!audioConfig.useVideoDevice) {
		if (audioConfig.useSeparateAudioFilter)
			blog(LOG_INFO, "\tseparate audio filter");
		else
			blog(LOG_INFO, "\taudio device: %s",
			     (const char *)name_utf8);
	}

	const char *mode = "";

	switch (audioConfig.mode) {
	case AudioMode::Capture:
		mode = "Capture";
		break;
	case AudioMode::DirectSound:
		mode = "DirectSound";
		break;
	case AudioMode::WaveOut:
		mode = "WaveOut";
		break;
	}

	//PRISM/FanZirong/20241203/PRISM_PC-1675/add log fields
	char source_p[50];
	snprintf(source_p, sizeof(source_p), "%p", source);
	const char *fields[][2] = {{PTS_LOG_TYPE, PTS_TYPE_EVENT},
				   {"source", source_p}};

	blogex(false, LOG_INFO, fields, 2,
	       "\tsample rate: %d\n"
	       "\tchannels: %d\n"
	       "\taudio type: %s",
	       audioConfig.sampleRate, audioConfig.channels, mode);
	return true;
}

void DShowInput::SetActive(bool active_)
{
	obs_data_t *settings = obs_source_get_settings(source);
	QueueAction(active_ ? Action::Activate : Action::Deactivate);
	obs_data_set_bool(settings, "active", active_);
	active = active_;
	obs_data_release(settings);
}

inline enum video_colorspace
DShowInput::GetColorSpace(obs_data_t *settings) const
{
	const char *space = obs_data_get_string(settings, COLOR_SPACE);

	if (astrcmpi(space, "709") == 0)
		return VIDEO_CS_709;

	if (astrcmpi(space, "601") == 0)
		return VIDEO_CS_601;

	if (astrcmpi(space, "2100PQ") == 0)
		return VIDEO_CS_2100_PQ;

	if (astrcmpi(space, "2100HLG") == 0)
		return VIDEO_CS_2100_HLG;

	if (videoConfig.format == VideoFormat::P010)
		return VIDEO_CS_2100_PQ;

	return VIDEO_CS_DEFAULT;
}

inline enum video_range_type
DShowInput::GetColorRange(obs_data_t *settings) const
{
	const char *range = obs_data_get_string(settings, COLOR_RANGE);

	if (astrcmpi(range, "full") == 0)
		return VIDEO_RANGE_FULL;
	if (astrcmpi(range, "partial") == 0)
		return VIDEO_RANGE_PARTIAL;
	return VIDEO_RANGE_DEFAULT;
}

inline bool DShowInput::Activate(obs_data_t *settings)
{
	if (!device.ResetGraph()) {
		obs_source_set_audio_active(source, false);
		return false;
	}

	//PRISM/Xiewei/20230712/noissue/add log
	is_first_video = true;
	is_first_audio = true;
	//PRISM/Xiewei/202312125/noissue/add debug log start
#if defined(_DEBUG_AV_DATA)
	m_lastAudioOriginPts = std::nullopt;
	m_lastAudioFramePts = std::nullopt;
	m_lastVideoOriginPts = std::nullopt;
	m_lastVideoFramePts = std::nullopt;
#endif
	//PRISM/Xiewei/202312125/noissue/add debug log end
	video_device[0] = 0;

	//PRISM/FanZirong/20240823/PRISM_PC-1042/reduce log
	decodeErrorNum = 0;

	if (!UpdateVideoConfig(settings)) {
		//PRISM/FanZirong/20241203/PRISM_PC-1675/add log fields
		const char *fields[][2] = {{PTS_LOG_TYPE, PTS_TYPE_EVENT}};
		blogex(false, LOG_WARNING, fields, 1,
		       "%s: Video configuration failed",
		       obs_source_get_name(source));
		obs_source_set_audio_active(source, false);
		return false;
	}

	if (!UpdateAudioConfig(settings)) {
		//PRISM/FanZirong/20241203/PRISM_PC-1675/add log fields
		const char *fields[][2] = {{PTS_LOG_TYPE, PTS_TYPE_EVENT}};
		blogex(false, LOG_WARNING, fields, 1,
		       "%s: Audio configuration failed, ignoring audio",
		       obs_source_get_name(source));
	}
		
	if (!device.ConnectFilters())
		return false;

	//PRISM/FanZirong/start/20240131/#3752/one of av device is lens, transform PTS to start from 0
	is_need_transform_pts = false;
	bool useCustomAudio = obs_data_get_bool(settings, USE_CUSTOM_AUDIO);
	if (useCustomAudio) {
		bool audioDeviceIsLens = (audioConfig.name.find(L"PRISM Lens") != std::wstring::npos);
		bool videoDeviceIsLens = (videoConfig.name.find(L"PRISM Lens") != std::wstring::npos);

		if ((audioDeviceIsLens && !videoDeviceIsLens) || (!audioDeviceIsLens && videoDeviceIsLens)) {
			do_log(LOG_INFO, "only %s device use lens", audioDeviceIsLens ? "audio" : "video");
			is_need_transform_pts = true;
		}
	}
	//PRISM/FanZirong/end/20240131/#3752/one of av device is lens, transform PTS to start from 0

	//PRISM/Xiewei/20230712/noissue/add log
	if (device.Start() != Result::Success) {
		do_log(LOG_WARNING, "start device failed");
		return false;
	}

	//PRISM/Xiewei/20240621/none/add more logs to trace video data start
	BPtr<char> name_utf8;
	os_wcs_to_utf8_ptr(videoConfig.name.c_str(), videoConfig.name.size(),
			   &name_utf8);
	DeviceNotification::instance()->Unsubscribe(subscribeId);
	subscribeId = DeviceNotification::instance()->Subscribe(
		[this, path = videoConfig.path](const std::wstring &id,
						DeviceEvent e) {
			if (0 == wstrcmpi(id.c_str(), path.c_str())) {
				do_log(LOG_INFO, "device %s",
				       (DeviceEvent::Inserted == e)
					       ? "inserted"
					       : "removed");
				if (data_monitor)
					data_monitor->Reset();
			}
		});
	if (data_monitor)
		data_monitor->Reset();
	if (videoConfig.frameInterval > 0)
		monitor_video.SetFPS(10000000.0 /
				     double(videoConfig.frameInterval));
	monitor_video.SetDevName((const char *)name_utf8);
	monitor_video.SetContext((void *)source);
	//PRISM/Xiewei/20240621/none/add more logs to trace video data end

	cs = GetColorSpace(settings);
	const enum video_range_type range = GetColorRange(settings);

	enum video_trc trc = VIDEO_TRC_DEFAULT;
	switch (cs) {
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_601:
	case VIDEO_CS_709:
	case VIDEO_CS_SRGB:
		trc = VIDEO_TRC_SRGB;
		break;
	case VIDEO_CS_2100_PQ:
		trc = VIDEO_TRC_PQ;
		break;
	case VIDEO_CS_2100_HLG:
		trc = VIDEO_TRC_HLG;
	}

	frame.range = range;
	frame.trc = trc;

	bool success = video_format_get_parameters_for_format(
		cs, range, ConvertVideoFormat(videoConfig.format),
		frame.color_matrix, frame.color_range_min,
		frame.color_range_max);
	if (!success) {
		blog(LOG_ERROR,
		     "Failed to get video format parameters for "
		     "video format %u",
		     cs);
	}

	//PRISM/Zengqin/20231229/#3744/notify ui to not support HDR color space.
	if ((videoConfig.format == VideoFormat::ARGB || videoConfig.format == VideoFormat::XRGB || videoConfig.format == VideoFormat::RGB24) && (trc == VIDEO_TRC_PQ || trc == VIDEO_TRC_HLG)) {
		pls_source_send_notify(source, OBS_SOURCE_PROPERTY_ERROR_STATUS, 1);
		not_support_hdr = true;
	}
	else {
		pls_source_send_notify(source, OBS_SOURCE_PROPERTY_ERROR_STATUS, 0);
		not_support_hdr = false;
	}
	obs_data_set_bool(settings, "not_support_hdr", not_support_hdr);

	//PRISM/Xiewei/20230712/noissue/add log
	char source_p[50];
	snprintf(source_p, sizeof(source_p), "%p", source);
	const char *fields[][2] = {{PTS_LOG_TYPE, PTS_TYPE_EVENT},
				   {"source", source_p}};
	do_logex(false, LOG_INFO, fields, 2, "sucess to init OBS camera")

	//PRISM/FanZirong/20231227/noissue/add log
	uint64_t source_update_time = os_gettime_ns();
	obs_source_notify_update(source, source_update_time);

	return true;
}

inline void DShowInput::Deactivate()
{
	device.ResetGraph();
	obs_source_output_video2(source, nullptr);

	//PRISM/Xiewei/20240621/none/add more logs to trace video data
	if (data_monitor)
		data_monitor->Reset();
}

/* ------------------------------------------------------------------------- */

static const char *GetDShowInputName(void *)
{
	return TEXT_INPUT_NAME;
}

static void proc_activate(void *data, calldata_t *cd)
{
	bool activate = calldata_bool(cd, "active");
	DShowInput *input = reinterpret_cast<DShowInput *>(data);
	input->SetActive(activate);
}

static void *CreateDShowInput(obs_data_t *settings, obs_source_t *source)
{
	DShowInput *dshow = nullptr;

	try {
		dshow = new DShowInput(source, settings);
		proc_handler_t *ph = obs_source_get_proc_handler(source);
		proc_handler_add(ph, "void activate(bool active)",
				 proc_activate, dshow);
	} catch (const char *error) {
		blog(LOG_ERROR, "Could not create device '%s': %s",
		     obs_source_get_name(source), error);
	}

	return dshow;
}

static void DestroyDShowInput(void *data)
{
	delete reinterpret_cast<DShowInput *>(data);
}

static void UpdateDShowInput(void *data, obs_data_t *settings)
{
	DShowInput *input = reinterpret_cast<DShowInput *>(data);
	if (input->active)
		input->QueueActivate(settings);
}

static void GetDShowDefaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, FRAME_INTERVAL, FPS_MATCHING);
	obs_data_set_default_int(settings, RES_TYPE, ResType_Preferred);
	obs_data_set_default_int(settings, VIDEO_FORMAT, (int)VideoFormat::Any);
	obs_data_set_default_bool(settings, "active", true);
	obs_data_set_default_string(settings, COLOR_SPACE, "default");
	obs_data_set_default_string(settings, COLOR_RANGE, "default");
	obs_data_set_default_int(settings, AUDIO_OUTPUT_MODE,
				 (int)AudioMode::Capture);
	obs_data_set_default_bool(settings, AUTOROTATION, true);
	obs_data_set_default_bool(settings, HW_DECODE, false);
}

struct Resolution {
	int cx, cy;

	inline Resolution(int cx, int cy) : cx(cx), cy(cy) {}
};

static void InsertResolution(vector<Resolution> &resolutions, int cx, int cy)
{
	int bestCY = 0;
	size_t idx = 0;

	for (; idx < resolutions.size(); idx++) {
		const Resolution &res = resolutions[idx];
		if (res.cx > cx)
			break;

		if (res.cx == cx) {
			if (res.cy == cy)
				return;

			if (!bestCY)
				bestCY = res.cy;
			else if (res.cy > bestCY)
				break;
		}
	}

	resolutions.insert(resolutions.begin() + idx, Resolution(cx, cy));
}

static inline void AddCap(vector<Resolution> &resolutions, const VideoInfo &cap)
{
	InsertResolution(resolutions, cap.minCX, cap.minCY);
	InsertResolution(resolutions, cap.maxCX, cap.maxCY);
}

#define MAKE_DSHOW_FPS(fps) (10000000LL / (fps))
#define MAKE_DSHOW_FRACTIONAL_FPS(den, num) ((num) * 10000000LL / (den))

static long long GetOBSFPS()
{
	obs_video_info ovi;
	if (!obs_get_video_info(&ovi))
		return 0;

	return MAKE_DSHOW_FRACTIONAL_FPS(ovi.fps_num, ovi.fps_den);
}

struct FPSFormat {
	const char *text;
	long long interval;
};

static const FPSFormat validFPSFormats[] = {
	{"360", MAKE_DSHOW_FPS(360)},
	{"240", MAKE_DSHOW_FPS(240)},
	{"144", MAKE_DSHOW_FPS(144)},
	{"120", MAKE_DSHOW_FPS(120)},
	{"119.88 NTSC", MAKE_DSHOW_FRACTIONAL_FPS(120000, 1001)},
	{"100", MAKE_DSHOW_FPS(100)},
	{"60", MAKE_DSHOW_FPS(60)},
	{"59.94 NTSC", MAKE_DSHOW_FRACTIONAL_FPS(60000, 1001)},
	{"50", MAKE_DSHOW_FPS(50)},
	{"48 film", MAKE_DSHOW_FRACTIONAL_FPS(48000, 1001)},
	{"40", MAKE_DSHOW_FPS(40)},
	{"30", MAKE_DSHOW_FPS(30)},
	{"29.97 NTSC", MAKE_DSHOW_FRACTIONAL_FPS(30000, 1001)},
	{"25", MAKE_DSHOW_FPS(25)},
	{"24 film", MAKE_DSHOW_FRACTIONAL_FPS(24000, 1001)},
	{"20", MAKE_DSHOW_FPS(20)},
	{"15", MAKE_DSHOW_FPS(15)},
	{"10", MAKE_DSHOW_FPS(10)},
	{"5", MAKE_DSHOW_FPS(5)},
	{"4", MAKE_DSHOW_FPS(4)},
	{"3", MAKE_DSHOW_FPS(3)},
	{"2", MAKE_DSHOW_FPS(2)},
	{"1", MAKE_DSHOW_FPS(1)},
};

static bool DeviceIntervalChanged(obs_properties_t *props, obs_property_t *p,
				  obs_data_t *settings);

static bool TryResolution(const VideoDevice &dev, const string &res)
{
	int cx, cy;
	if (!ConvertRes(cx, cy, res.c_str()))
		return false;

	return ResolutionAvailable(dev, cx, cy);
}

static bool SetResolution(obs_properties_t *props, obs_data_t *settings,
			  const string &res, bool autoselect = false)
{
	if (autoselect)
		obs_data_set_autoselect_string(settings, RESOLUTION,
					       res.c_str());
	else
		obs_data_unset_autoselect_value(settings, RESOLUTION);

	DeviceIntervalChanged(props, obs_properties_get(props, FRAME_INTERVAL),
			      settings);

	if (!autoselect)
		obs_data_set_string(settings, LAST_RESOLUTION, res.c_str());
	return true;
}

static bool DeviceResolutionChanged(obs_properties_t *props, obs_property_t *p,
				    obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	PropertiesData *data =
		(PropertiesData *)obs_properties_get_param(props);
	const char *id;
	VideoDevice device;

	id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	string res = obs_data_get_string(settings, RESOLUTION);
	string last_res = obs_data_get_string(settings, LAST_RESOLUTION);

	if (!data->GetDevice(device, id))
		return false;

	if (TryResolution(device, res))
		return SetResolution(props, settings, res);

	if (TryResolution(device, last_res))
		return SetResolution(props, settings, last_res, true);

	return false;
}

struct VideoFormatName {
	VideoFormat format;
	const char *name;
};

static const VideoFormatName videoFormatNames[] = {
	/* autoselect format*/
	{VideoFormat::Any, "VideoFormat.Any"},

	/* raw formats */
	{VideoFormat::ARGB, "ARGB"},
	{VideoFormat::XRGB, "XRGB"},

	/* planar YUV formats */
	{VideoFormat::I420, "I420"},
	{VideoFormat::NV12, "NV12"},
	{VideoFormat::YV12, "YV12"},
	{VideoFormat::Y800, "Y800"},
	{VideoFormat::P010, "P010"},

	/* packed YUV formats */
	{VideoFormat::YVYU, "YVYU"},
	{VideoFormat::YUY2, "YUY2"},
	{VideoFormat::UYVY, "UYVY"},
	{VideoFormat::HDYC, "HDYC"},

	/* encoded formats */
	{VideoFormat::MJPEG, "MJPEG"},
	{VideoFormat::H264, "H264"},
#ifdef ENABLE_HEVC
	{VideoFormat::HEVC, "HEVC"},
#endif
};

static bool ResTypeChanged(obs_properties_t *props, obs_property_t *p,
			   obs_data_t *settings);

static size_t AddDevice(obs_property_t *device_list, const string &id)
{
	DStr name, path;
	if (!DecodeDeviceDStr(name, path, id.c_str()))
		return numeric_limits<size_t>::max();

	return obs_property_list_add_string(device_list, name, id.c_str());
}

static bool UpdateDeviceList(obs_property_t *list, const string &id)
{
	size_t size = obs_property_list_item_count(list);
	bool found = false;
	bool disabled_unknown_found = false;

	for (size_t i = 0; i < size; i++) {
		if (obs_property_list_item_string(list, i) == id) {
			found = true;
			continue;
		}
		if (obs_property_list_item_disabled(list, i))
			disabled_unknown_found = true;
	}

	if (!found && !disabled_unknown_found) {
		size_t idx = AddDevice(list, id);
		obs_property_list_item_disable(list, idx, true);
		return true;
	}

	if (found && !disabled_unknown_found)
		return false;

	for (size_t i = 0; i < size;) {
		if (obs_property_list_item_disabled(list, i)) {
			obs_property_list_item_remove(list, i);
			continue;
		}
		i += 1;
	}

	return true;
}

static bool DeviceSelectionChanged(obs_properties_t *props, obs_property_t *p,
				   obs_data_t *settings)
{
	PropertiesData *data =
		(PropertiesData *)obs_properties_get_param(props);
	VideoDevice device;

	string id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	string old_id = obs_data_get_string(settings, LAST_VIDEO_DEV_ID);

	bool device_list_updated = UpdateDeviceList(p, id);

	if (!data->GetDevice(device, id.c_str()))
		return !device_list_updated;

	vector<Resolution> resolutions;
	for (const VideoInfo &cap : device.caps)
		AddCap(resolutions, cap);

	p = obs_properties_get(props, RESOLUTION);
	obs_property_list_clear(p);

	for (size_t idx = resolutions.size(); idx > 0; idx--) {
		const Resolution &res = resolutions[idx - 1];

		string strRes;
		strRes += to_string(res.cx);
		strRes += "x";
		strRes += to_string(res.cy);

		obs_property_list_add_string(p, strRes.c_str(), strRes.c_str());
	}

	/* only refresh properties if device legitimately changed */
	if (!id.size() || !old_id.size() || id != old_id) {
		p = obs_properties_get(props, RES_TYPE);
		ResTypeChanged(props, p, settings);
		obs_data_set_string(settings, LAST_VIDEO_DEV_ID, id.c_str());
	}

	return true;
}

static bool VideoConfigClicked(obs_properties_t *props, obs_property_t *p,
			       void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput *>(data);
	input->QueueAction(Action::ConfigVideo);

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return false;
}

/*static bool AudioConfigClicked(obs_properties_t *props, obs_property_t *p,
		void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput*>(data);
	input->QueueAction(Action::ConfigAudio);

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return false;
}*/

static bool CrossbarConfigClicked(obs_properties_t *props, obs_property_t *p,
				  void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput *>(data);
	input->QueueAction(Action::ConfigCrossbar1);

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return false;
}

/*static bool Crossbar2ConfigClicked(obs_properties_t *props, obs_property_t *p,
		void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput*>(data);
	input->QueueAction(Action::ConfigCrossbar2);

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return false;
}*/

static bool AddDevice(obs_property_t *device_list, const VideoDevice &device)
{
	DStr name, path, device_id;

	dstr_from_wcs(name, device.name.c_str());
	dstr_from_wcs(path, device.path.c_str());

	encode_dstr(path);

	dstr_copy_dstr(device_id, name);
	encode_dstr(device_id);
	dstr_cat(device_id, ":");
	dstr_cat_dstr(device_id, path);

	obs_property_list_add_string(device_list, name, device_id);

	return true;
}

static bool AddAudioDevice(obs_property_t *device_list,
			   const AudioDevice &device)
{
	DStr name, path, device_id;

	dstr_from_wcs(name, device.name.c_str());
	dstr_from_wcs(path, device.path.c_str());

	encode_dstr(path);

	dstr_copy_dstr(device_id, name);
	encode_dstr(device_id);
	dstr_cat(device_id, ":");
	dstr_cat_dstr(device_id, path);

	obs_property_list_add_string(device_list, name, device_id);

	//PRISM/Xiewei/20240520/#5406/add log start
	const auto &caps = device.caps;
	if (std::string(name->array ? name : "").find("PRISM Lens audio") !=
	    std::string::npos) {
		for (size_t i = 0; i < caps.size(); i++) {
			std::string log;
			auto info = caps[i];
			log.append("\tcap ")
				.append(std::to_string(i))
				.append("\n");
			log.append("\tminChannels/maxChannels: ")
				.append(std::to_string(info.minChannels))
				.append("/")
				.append(std::to_string(info.maxChannels))
				.append("\n");
			log.append("\tminSampleRate/maxSampleRate: ")
				.append(std::to_string(info.minSampleRate))
				.append("/")
				.append(std::to_string(info.maxSampleRate))
				.append("\n");
			log.append("\tformat: ")
				.append(std::to_string(
					static_cast<int>(info.format)));

			blog(LOG_INFO, "['%s'] audio enumerate:\n %s",
			     name->array, log.c_str());
		}
	}
	//PRISM/Xiewei/20240520/#5406/add log end

	return true;
}

static void PropertiesDataDestroy(void *data)
{
	delete reinterpret_cast<PropertiesData *>(data);
}

static bool ResTypeChanged(obs_properties_t *props, obs_property_t *p,
			   obs_data_t *settings)
{
	int val = (int)obs_data_get_int(settings, RES_TYPE);
	bool enabled = (val != ResType_Preferred);

	p = obs_properties_get(props, RESOLUTION);
	obs_property_set_enabled(p, enabled);

	p = obs_properties_get(props, FRAME_INTERVAL);
	obs_property_set_enabled(p, enabled);

	p = obs_properties_get(props, VIDEO_FORMAT);
	obs_property_set_enabled(p, enabled);

	if (val == ResType_Custom) {
		p = obs_properties_get(props, RESOLUTION);
		DeviceResolutionChanged(props, p, settings);
	} else {
		obs_data_unset_autoselect_value(settings, FRAME_INTERVAL);
	}

	return true;
}

static DStr GetFPSName(long long interval)
{
	DStr name;

	if (interval == FPS_MATCHING) {
		dstr_cat(name, TEXT_FPS_MATCHING);
		return name;
	}

	if (interval == FPS_HIGHEST) {
		dstr_cat(name, TEXT_FPS_HIGHEST);
		return name;
	}

	for (const FPSFormat &format : validFPSFormats) {
		if (format.interval != interval)
			continue;

		dstr_cat(name, format.text);
		return name;
	}

	dstr_cat(name, to_string(10000000. / interval).c_str());
	return name;
}

static void UpdateFPS(VideoDevice &device, VideoFormat format,
		      long long interval, int cx, int cy,
		      obs_properties_t *props)
{
	obs_property_t *list = obs_properties_get(props, FRAME_INTERVAL);

	obs_property_list_clear(list);

	obs_property_list_add_int(list, TEXT_FPS_MATCHING, FPS_MATCHING);
	obs_property_list_add_int(list, TEXT_FPS_HIGHEST, FPS_HIGHEST);

	bool interval_added = interval == FPS_HIGHEST ||
			      interval == FPS_MATCHING;
	for (const FPSFormat &fps_format : validFPSFormats) {
		bool video_format_match = false;
		long long format_interval = fps_format.interval;

		bool available = CapsMatch(
			device, ResolutionMatcher(cx, cy),
			VideoFormatMatcher(format, video_format_match),
			FrameRateMatcher(format_interval));

		if (!available && interval != fps_format.interval)
			continue;

		if (interval == fps_format.interval)
			interval_added = true;

		size_t idx = obs_property_list_add_int(list, fps_format.text,
						       fps_format.interval);
		obs_property_list_item_disable(list, idx, !available);
	}

	if (interval_added)
		return;

	size_t idx =
		obs_property_list_add_int(list, GetFPSName(interval), interval);
	obs_property_list_item_disable(list, idx, true);
}

static DStr GetVideoFormatName(VideoFormat format)
{
	DStr name;
	for (const VideoFormatName &format_ : videoFormatNames) {
		if (format_.format == format) {
			dstr_cat(name, obs_module_text(format_.name));
			return name;
		}
	}

	dstr_cat(name, TEXT_FORMAT_UNKNOWN);
	dstr_replace(name, "%1", std::to_string((long long)format).c_str());
	return name;
}

static void UpdateVideoFormats(VideoDevice &device, VideoFormat format_, int cx,
			       int cy, long long interval,
			       obs_properties_t *props)
{
	set<VideoFormat> formats = {VideoFormat::Any};
	auto format_gatherer =
		[&formats](const VideoInfo &info) mutable -> bool {
		formats.insert(info.format);
		return false;
	};

	CapsMatch(device, ResolutionMatcher(cx, cy), FrameRateMatcher(interval),
		  format_gatherer);

	obs_property_t *list = obs_properties_get(props, VIDEO_FORMAT);
	obs_property_list_clear(list);

	bool format_added = false;
	for (const VideoFormatName &format : videoFormatNames) {
		bool available = formats.find(format.format) != end(formats);

		if (!available && format.format != format_)
			continue;

		if (format.format == format_)
			format_added = true;

		size_t idx = obs_property_list_add_int(
			list, obs_module_text(format.name),
			(long long)format.format);
		obs_property_list_item_disable(list, idx, !available);
	}

	if (format_added)
		return;

	size_t idx = obs_property_list_add_int(
		list, GetVideoFormatName(format_), (long long)format_);
	obs_property_list_item_disable(list, idx, true);
}

static bool UpdateFPS(long long interval, obs_property_t *list)
{
	size_t size = obs_property_list_item_count(list);
	DStr name;

	for (size_t i = 0; i < size; i++) {
		if (obs_property_list_item_int(list, i) != interval)
			continue;

		obs_property_list_item_disable(list, i, true);
		if (size == 1)
			return false;

		dstr_cat(name, obs_property_list_item_name(list, i));
		break;
	}

	obs_property_list_clear(list);

	if (!name->len)
		name = GetFPSName(interval);

	obs_property_list_add_int(list, name, interval);
	obs_property_list_item_disable(list, 0, true);

	return true;
}

static bool DeviceIntervalChanged(obs_properties_t *props, obs_property_t *p,
				  obs_data_t *settings)
{
	long long val = obs_data_get_int(settings, FRAME_INTERVAL);

	PropertiesData *data =
		(PropertiesData *)obs_properties_get_param(props);
	const char *id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	VideoDevice device;

	if (!data->GetDevice(device, id))
		return UpdateFPS(val, p);

	int cx = 0, cy = 0;
	if (!DetermineResolution(cx, cy, settings, device)) {
		UpdateVideoFormats(device, VideoFormat::Any, 0, 0, 0, props);
		UpdateFPS(device, VideoFormat::Any, 0, 0, 0, props);
		return true;
	}

	int resType = (int)obs_data_get_int(settings, RES_TYPE);
	if (resType != ResType_Custom)
		return true;

	if (val == FPS_MATCHING)
		val = GetOBSFPS();

	VideoFormat format =
		(VideoFormat)obs_data_get_int(settings, VIDEO_FORMAT);

	bool video_format_matches = false;
	long long best_interval = numeric_limits<long long>::max();
	bool frameRateSupported =
		CapsMatch(device, ResolutionMatcher(cx, cy),
			  VideoFormatMatcher(format, video_format_matches),
			  ClosestFrameRateSelector(val, best_interval),
			  FrameRateMatcher(val));

	if (video_format_matches && !frameRateSupported &&
	    best_interval != val) {
		long long listed_val = 0;
		for (const FPSFormat &format : validFPSFormats) {
			long long diff = llabs(format.interval - best_interval);
			if (diff < DEVICE_INTERVAL_DIFF_LIMIT) {
				listed_val = format.interval;
				break;
			}
		}

		if (listed_val != val) {
			obs_data_set_autoselect_int(settings, FRAME_INTERVAL,
						    listed_val);
			val = listed_val;
		}

	} else {
		obs_data_unset_autoselect_value(settings, FRAME_INTERVAL);
	}

	UpdateVideoFormats(device, format, cx, cy, val, props);
	UpdateFPS(device, format, val, cx, cy, props);

	return true;
}

static bool UpdateVideoFormats(VideoFormat format, obs_property_t *list)
{
	size_t size = obs_property_list_item_count(list);
	DStr name;

	for (size_t i = 0; i < size; i++) {
		if ((VideoFormat)obs_property_list_item_int(list, i) != format)
			continue;

		if (size == 1)
			return false;

		dstr_cat(name, obs_property_list_item_name(list, i));
		break;
	}

	obs_property_list_clear(list);

	if (!name->len)
		name = GetVideoFormatName(format);

	obs_property_list_add_int(list, name, (long long)format);
	obs_property_list_item_disable(list, 0, true);

	return true;
}

static bool VideoFormatChanged(obs_properties_t *props, obs_property_t *p,
			       obs_data_t *settings)
{
	PropertiesData *data =
		(PropertiesData *)obs_properties_get_param(props);
	const char *id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	VideoDevice device;

	VideoFormat curFormat =
		(VideoFormat)obs_data_get_int(settings, VIDEO_FORMAT);

	if (!data->GetDevice(device, id))
		return UpdateVideoFormats(curFormat, p);

	int cx, cy;
	if (!DetermineResolution(cx, cy, settings, device)) {
		UpdateVideoFormats(device, VideoFormat::Any, cx, cy, 0, props);
		UpdateFPS(device, VideoFormat::Any, 0, 0, 0, props);
		return true;
	}

	long long interval = obs_data_get_int(settings, FRAME_INTERVAL);

	UpdateVideoFormats(device, curFormat, cx, cy, interval, props);
	UpdateFPS(device, curFormat, interval, cx, cy, props);
	return true;
}

static bool CustomAudioClicked(obs_properties_t *props, obs_property_t *p,
			       obs_data_t *settings)
{
	bool useCustomAudio = obs_data_get_bool(settings, USE_CUSTOM_AUDIO);
	p = obs_properties_get(props, AUDIO_DEVICE_ID);
	obs_property_set_visible(p, useCustomAudio);
	return true;
}

static bool ActivateClicked(obs_properties_t *, obs_property_t *p, void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput *>(data);

	if (input->active) {
		input->SetActive(false);
		obs_property_set_description(p, TEXT_ACTIVATE);
	} else {
		input->SetActive(true);
		obs_property_set_description(p, TEXT_DEACTIVATE);
	}

	return true;
}

static obs_properties_t *GetDShowProperties(void *obj)
{
	DShowInput *input = reinterpret_cast<DShowInput *>(obj);
	obs_properties_t *ppts = obs_properties_create();
	PropertiesData *data = new PropertiesData;

	data->input = input;

	obs_properties_set_param(ppts, data, PropertiesDataDestroy);

	obs_property_t *p = obs_properties_add_list(ppts, VIDEO_DEVICE_ID,
						    TEXT_DEVICE,
						    OBS_COMBO_TYPE_LIST,
						    OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, DeviceSelectionChanged);

	//PRISM/Xiewei/20230616/prism cam issues: 748/reorder devicel ist
	EnumReorderedVideoDevices(data->devices);
	for (const VideoDevice &device : data->devices)
		AddDevice(p, device);

	const char *activateText = TEXT_ACTIVATE;
	if (input) {
		if (input->active)
			activateText = TEXT_DEACTIVATE;
	}

	obs_properties_add_button(ppts, "activate", activateText,
				  ActivateClicked);
	obs_properties_add_button(ppts, "video_config", TEXT_CONFIG_VIDEO,
				  VideoConfigClicked);
	obs_properties_add_button(ppts, "xbar_config", TEXT_CONFIG_XBAR,
				  CrossbarConfigClicked);

	obs_properties_add_bool(ppts, DEACTIVATE_WNS, TEXT_DWNS);

	/* ------------------------------------- */
	/* video settings */

	p = obs_properties_add_list(ppts, RES_TYPE, TEXT_RES_FPS_TYPE,
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, ResTypeChanged);

	obs_property_list_add_int(p, TEXT_PREFERRED_RES, ResType_Preferred);
	obs_property_list_add_int(p, TEXT_CUSTOM_RES, ResType_Custom);

	p = obs_properties_add_list(ppts, RESOLUTION, TEXT_RESOLUTION,
				    OBS_COMBO_TYPE_EDITABLE,
				    OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, DeviceResolutionChanged);

	p = obs_properties_add_list(ppts, FRAME_INTERVAL, "FPS",
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, DeviceIntervalChanged);

	p = obs_properties_add_list(ppts, VIDEO_FORMAT, TEXT_VIDEO_FORMAT,
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, VideoFormatChanged);

	p = obs_properties_add_list(ppts, COLOR_SPACE, TEXT_COLOR_SPACE,
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, TEXT_COLOR_DEFAULT, "default");
	obs_property_list_add_string(p, TEXT_COLOR_709, "709");
	obs_property_list_add_string(p, TEXT_COLOR_601, "601");
	obs_property_list_add_string(p, TEXT_COLOR_2100PQ, "2100PQ");
	obs_property_list_add_string(p, TEXT_COLOR_2100HLG, "2100HLG");

	p = obs_properties_add_list(ppts, COLOR_RANGE, TEXT_COLOR_RANGE,
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, TEXT_RANGE_DEFAULT, "default");
	obs_property_list_add_string(p, TEXT_RANGE_PARTIAL, "partial");
	obs_property_list_add_string(p, TEXT_RANGE_FULL, "full");

	p = obs_properties_add_list(ppts, BUFFERING_VAL, TEXT_BUFFERING,
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, TEXT_BUFFERING_AUTO,
				  (int64_t)BufferingType::Auto);
	obs_property_list_add_int(p, TEXT_BUFFERING_ON,
				  (int64_t)BufferingType::On);
	obs_property_list_add_int(p, TEXT_BUFFERING_OFF,
				  (int64_t)BufferingType::Off);

	obs_property_set_long_description(p,
					  obs_module_text("Buffering.ToolTip"));

	obs_properties_add_bool(ppts, FLIP_IMAGE, TEXT_FLIP_IMAGE);

	obs_properties_add_bool(ppts, AUTOROTATION, TEXT_AUTOROTATION);

	obs_properties_add_bool(ppts, HW_DECODE, TEXT_HW_DECODE);

	/* ------------------------------------- */
	/* audio settings */

	Device::EnumAudioDevices(data->audioDevices);

	p = obs_properties_add_list(ppts, AUDIO_OUTPUT_MODE, TEXT_AUDIO_MODE,
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, TEXT_MODE_CAPTURE,
				  (int64_t)AudioMode::Capture);
	obs_property_list_add_int(p, TEXT_MODE_DSOUND,
				  (int64_t)AudioMode::DirectSound);
	obs_property_list_add_int(p, TEXT_MODE_WAVEOUT,
				  (int64_t)AudioMode::WaveOut);

	if (!data->audioDevices.size())
		return ppts;

	p = obs_properties_add_bool(ppts, USE_CUSTOM_AUDIO, TEXT_CUSTOM_AUDIO);

	obs_property_set_modified_callback(p, CustomAudioClicked);

	p = obs_properties_add_list(ppts, AUDIO_DEVICE_ID, TEXT_AUDIO_DEVICE,
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	for (const AudioDevice &device : data->audioDevices)
		AddAudioDevice(p, device);

	//PRISM/Zengqin/20231229/#3744/notify ui to not support HDR color space.
	if (input) {
		pls_source_send_notify(input->source,
				       OBS_SOURCE_PROPERTY_ERROR_STATUS,
				       input->not_support_hdr);
	}

	return ppts;
}

void DShowModuleLogCallback(LogType type, const wchar_t *msg, void *param)
{
	int obs_type = LOG_DEBUG;

	switch (type) {
	case LogType::Error:
		obs_type = LOG_ERROR;
		break;
	case LogType::Warning:
		obs_type = LOG_WARNING;
		break;
	case LogType::Info:
		obs_type = LOG_INFO;
		break;
	case LogType::Debug:
		obs_type = LOG_DEBUG;
		break;
	}

	DStr dmsg;

	dstr_from_wcs(dmsg, msg);
	blog(obs_type, "DShow: %s", dmsg->array);

	UNUSED_PARAMETER(param);
}

static void HideDShowInput(void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput *>(data);

	if (input->deactivateWhenNotShowing && input->active)
		input->QueueAction(Action::Deactivate);
}

static void ShowDShowInput(void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput *>(data);

	if (input->deactivateWhenNotShowing && input->active)
		input->QueueAction(Action::Activate);
}

void RegisterDShowSource()
{
	SetLogCallback(DShowModuleLogCallback, nullptr);

	obs_source_info info = {};
	info.id = "dshow_input";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_AUDIO |
			    OBS_SOURCE_ASYNC | OBS_SOURCE_DO_NOT_DUPLICATE;
	info.show = ShowDShowInput;
	info.hide = HideDShowInput;
	info.get_name = GetDShowInputName;
	info.create = CreateDShowInput;
	info.destroy = DestroyDShowInput;
	info.update = UpdateDShowInput;
	info.get_defaults = GetDShowDefaults;
	info.get_properties = GetDShowProperties;
	info.icon_type = OBS_ICON_TYPE_CAMERA;
	obs_register_source(&info);
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static bool EnumReorderedLensVideoDevices(std::vector<VideoDevice> &devices)
{
	bool ok = Device::EnumVideoDevices(devices);
	if (!ok)
		return false;

	static const int vcam_number = 3;
	const wchar_t *device_name[vcam_number] = {_T(TEXT_PRISM_LENS_1),
						   _T(TEXT_PRISM_LENS_2),
						   _T(TEXT_PRISM_LENS_3)};

	auto find_func = [device_name](const wchar_t *name) {
		for (int i = 0; i < vcam_number; i++) {
			if (wstrcmpi(name, device_name[i]) == 0)
				return i;
		}
		return -1;
	};
	std::vector<VideoDevice> prismLensDevices;
	for (auto iter = devices.begin(); iter != devices.end(); iter++) {
		auto i = find_func((*iter).name.c_str());
		if (i == -1) {
			continue;
		}
		prismLensDevices.push_back((*iter));
	}
	devices.swap(prismLensDevices);
	return true;
}

//PRISM/Liuying/20230808/new ux : add prism lens source
template<typename T> static string GetDeviceId(const T &device)
{
	DStr name, path, device_id;

	dstr_from_wcs(name, device.name.c_str());
	dstr_from_wcs(path, device.path.c_str());

	encode_dstr(path);

	dstr_copy_dstr(device_id, name);
	encode_dstr(device_id);
	dstr_cat(device_id, ":");
	dstr_cat_dstr(device_id, path);
	return device_id->array;
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static bool PrismLensDeviceSelectionChanged(void *obj, obs_properties_t *props,
					    obs_property_t *p,
					    obs_data_t *settings)
{
	DShowInput *input = reinterpret_cast<DShowInput *>(obj);

	string id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	string oldId = obs_data_get_string(settings, LAST_VIDEO_DEV_ID);
	DeviceSelectionChanged(props, p, settings);

	if (id == oldId) {
		return false;
	}

	string subStr = id.substr(0, strlen(TEXT_PRISM_LENS_1));
	wstring audioName;
	if (0 == subStr.compare(TEXT_PRISM_LENS_1)) {
		audioName = _T(TEXT_PRISM_LENS_AUDIO_1);
	} else if (0 == subStr.compare(TEXT_PRISM_LENS_2)) {
		audioName = _T(TEXT_PRISM_LENS_AUDIO_2);
	} else if (0 == subStr.compare(TEXT_PRISM_LENS_3)) {
		audioName = _T(TEXT_PRISM_LENS_AUDIO_3);
	}

	PropertiesData *data =
		(PropertiesData *)obs_properties_get_param(props);
	vector<AudioDevice> devices = data->audioDevices;
	for (auto iter = devices.cbegin(); iter != devices.cend(); iter++) {
		wstring name = (*iter).name;
		if (0 == name.compare(audioName)) {
			string audioDeviceId = GetDeviceId<AudioDevice>(*iter);
			obs_data_set_string(settings, AUDIO_DEVICE_ID,
					    audioDeviceId.c_str());
			if (!obs_data_has_default_value(settings,
							AUDIO_DEVICE_ID)) {
				obs_data_set_default_string(
					settings, AUDIO_DEVICE_ID,
					audioDeviceId.c_str());
			}
			obs_source_update(input->source, settings);
		}
	}

	return true;
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static bool PrismLensCustomAudioClicked(obs_properties_t *props,
					obs_property_t *p, obs_data_t *settings)
{
	bool useCustomAudio = obs_data_get_bool(settings, USE_CUSTOM_AUDIO);
	p = obs_properties_get(props, AUDIO_DEVICE_ID);
	obs_property_set_enabled(p, useCustomAudio);
	return true;
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static string
GetVideoDeviceDefaultValue(wstring videoDeviceName,
			   const std::vector<VideoDevice> &devices)
{
	for (auto iter = devices.cbegin(); iter != devices.cend(); iter++) {
		wstring name = (*iter).name;
		if (0 == name.compare(videoDeviceName)) {
			return GetDeviceId<VideoDevice>(*iter);
		}
	}

	return string();
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static obs_properties_t *
GetPrismLensInternalProperties(void *obj, const char *keyTip, wstring lensName)
{
	DShowInput *input = reinterpret_cast<DShowInput *>(obj);
	obs_properties_t *ppts = obs_properties_create();
	PropertiesData *data = new PropertiesData;

	data->input = input;

	obs_properties_set_param(ppts, data, PropertiesDataDestroy);

	obs_property_t *p = obs_properties_add_list(ppts, VIDEO_DEVICE_ID,
						    TEXT_PRISM_LENS_DEVICE,
						    OBS_COMBO_TYPE_LIST,
						    OBS_COMBO_FORMAT_STRING);
	obs_property_set_modified_callback2(p, PrismLensDeviceSelectionChanged,
					    obj);

	EnumReorderedLensVideoDevices(data->devices);
	Device::EnumAudioDevices(data->audioDevices);

	for (const VideoDevice &device : data->devices)
		AddDevice(p, device);

	if (!data->devices.empty()) {
		OBSDataAutoRelease settings =
			obs_source_get_settings(input->source);
		string deviceId =
			GetVideoDeviceDefaultValue(lensName, data->devices);
		if (!obs_data_has_default_value(settings, VIDEO_DEVICE_ID)) {
			obs_data_set_default_string(settings, VIDEO_DEVICE_ID,
						    deviceId.c_str());
			PrismLensDeviceSelectionChanged(obj, ppts, p, settings);
			obs_source_update(input->source, settings);
		}
		if (!obs_data_has_user_value(settings, VIDEO_DEVICE_ID)) {
			obs_data_set_string(settings, VIDEO_DEVICE_ID,
					    deviceId.c_str());
		}
	}

	const char *activateText = TEXT_ACTIVATE;
	if (input) {
		if (input->active)
			activateText = TEXT_DEACTIVATE;
	}

	obs_properties_add_button(ppts, "activate", activateText,
				  ActivateClicked);
	pls_properties_add_tips(ppts, "tip", keyTip);

	/* audio settings */

	p = obs_properties_add_bool(ppts, USE_CUSTOM_AUDIO, TEXT_CUSTOM_AUDIO);

	obs_property_set_modified_callback(p, PrismLensCustomAudioClicked);
	p = obs_properties_add_list(ppts, AUDIO_DEVICE_ID,
				    TEXT_PRISM_LENS_AUDIO_DEVICE,
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	for (const AudioDevice &device : data->audioDevices) {
		AddAudioDevice(p, device);
	}

	return ppts;
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static obs_properties_t *GetPrismLensProperties(void *obj)
{
	return GetPrismLensInternalProperties(obj, TEXT_PRISM_LENS_TIPS,
					      _T(TEXT_PRISM_LENS_1));
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static obs_properties_t *GetPrismLensMobileProperties(void *obj)
{
	return GetPrismLensInternalProperties(obj, TEXT_PRISM_LENS_MOBILE_TIPS,
					      _T(TEXT_PRISM_LENS_3));
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static void GetPrismLensDefaults(obs_data_t *settings)
{
	GetDShowDefaults(settings);
	obs_data_set_default_bool(settings, USE_CUSTOM_AUDIO, true);
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static void GetPrismMobileDefaults(obs_data_t *settings)
{
	GetDShowDefaults(settings);
	obs_data_set_default_bool(settings, USE_CUSTOM_AUDIO, true);
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static const char *GetPrismLensInputName(void *)
{
	return TEXT_PRISM_LENS_NAME;
}

//PRISM/Liuying/20230808/new ux : add prism lens source
static const char *GetPrismMobileInputName(void *)
{
	return TEXT_PRISM_LENS_MOBILE_NAME;
}

void RegisterPrismLensSource()
{
	SetLogCallback(DShowModuleLogCallback, nullptr);

	obs_source_info info = {};
	info.id = TEXT_PRISM_LENS_ID;
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_AUDIO |
			    OBS_SOURCE_ASYNC | OBS_SOURCE_DO_NOT_DUPLICATE;
	info.show = ShowDShowInput;
	info.hide = HideDShowInput;
	info.get_name = GetPrismLensInputName;
	info.create = CreateDShowInput;
	info.destroy = DestroyDShowInput;
	info.update = UpdateDShowInput;
	info.get_defaults = GetPrismLensDefaults;
	info.get_properties = GetPrismLensProperties;
	info.icon_type = static_cast<obs_icon_type>(PLS_ICON_TYPE_PRISM_LENS);
	obs_register_source(&info);
}

//PRISM/Liuying/20230808/new ux : add prism lens source
void RegisterPrismMobileSource()
{
	SetLogCallback(DShowModuleLogCallback, nullptr);

	obs_source_info info = {};
	info.id = TEXT_PRISM_LENS_MOBILE_ID;
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_AUDIO |
			    OBS_SOURCE_ASYNC | OBS_SOURCE_DO_NOT_DUPLICATE;
	info.show = ShowDShowInput;
	info.hide = HideDShowInput;
	info.get_name = GetPrismMobileInputName;
	info.create = CreateDShowInput;
	info.destroy = DestroyDShowInput;
	info.update = UpdateDShowInput;
	info.get_defaults = GetPrismMobileDefaults;
	info.get_properties = GetPrismLensMobileProperties;
	info.icon_type = static_cast<obs_icon_type>(PLS_ICON_TYPE_PRISM_MOBILE);
	obs_register_source(&info);
}
