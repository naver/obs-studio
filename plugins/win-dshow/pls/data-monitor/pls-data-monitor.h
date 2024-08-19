#pragma once
#include "obs.h"
#include "util/c99defs.h"
#include <vector>
#include <string>
#include <functional>
#include <mutex>

class PLSMonitorCameraVideo {
public:
	PLSMonitorCameraVideo() {}
	void SetFPS(double fps);
	void SetDevName(const std::string &name);
	void SetContext(void *context);
	void Reset();
	void BeginHandleVideo();
	void EndHandleVideo();
	void SendCatchLogs();

private:
	void SaveStatisticsLog(uint64_t seconds);

	//-----------------------------------------------------
	std::string video_name;
	void *context = nullptr;
	double expect_fps = 0.0;
	int64_t expect_interval = 0; // in 1000000 == 1s

	uint64_t pre_log_systime = 0;    // in 1000000 == 1s
	uint64_t pre_video_systime = 0;  // in 1000000 == 1s
	uint64_t next_video_systime = 0; // in 1000000 == 1s
	uint64_t start_fps_systime = 0;  // in 1000000 == 1s
	uint64_t fps_video_count = 0;
	std::vector<double> fps_list;
	std::vector<uint64_t> late_list;   // in 1000 == 1s
	std::vector<std::string> log_list; // in 1000 == 1s
};

class DataMonitor {
public:
	DataMonitor(uint64_t timeMs, std::function<void()> callback, void *context);
	~DataMonitor();
	void PushData();
	bool CheckData();
	void Reset();
	void Resum();
	void Stop();

private:
	std::mutex m_mutex;
	std::thread m_thread;
	std::atomic_bool m_stop = false;
	std::atomic_bool m_reset = true;
	std::chrono::steady_clock::time_point m_lastTimeStamp;
	int m_dataCount = 0;
	std::function<void()> m_callback;
	uint64_t m_timeMs;
	void *m_pContext = nullptr;
};
