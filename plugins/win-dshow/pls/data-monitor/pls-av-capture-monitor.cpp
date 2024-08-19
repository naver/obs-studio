#include "pls-data-monitor.h"
#include "util/base.h"
#include <chrono>
#include <array>
#include <algorithm>

const auto STATISTICS_LIST_COUNT = 10;
const auto PER_STATISTICS_INTERVAL = 60; // in seconds

uint64_t GetTimeIn1000NS()
{
	return std::chrono::steady_clock::now().time_since_epoch().count() /
	       1000;
}

void PLSMonitorCameraVideo::SetFPS(double fps)
{
	if (fps > 0) {
		expect_fps = fps;
		expect_interval = int64_t(double(1000000) / fps);
	}
}

void PLSMonitorCameraVideo::SetDevName(const std::string &name)
{
	video_name = name;
}

void PLSMonitorCameraVideo::SetContext(void *_context)
{
	context = _context;
}

void PLSMonitorCameraVideo::Reset()
{
	SendCatchLogs();

	pre_video_systime = 0;
	next_video_systime = 0;
	start_fps_systime = 0;
	fps_video_count = 0;
	fps_list.clear();
	late_list.clear();
	pre_log_systime = GetTimeIn1000NS();
}

void PLSMonitorCameraVideo::BeginHandleVideo()
{
	uint64_t current_time = GetTimeIn1000NS();

	if (!pre_video_systime || !start_fps_systime) {
		pre_video_systime = current_time;
		start_fps_systime = current_time;
		next_video_systime = 0;
		fps_video_count = 1;
		fps_list.clear();
		late_list.clear();
		pre_log_systime = GetTimeIn1000NS();
		return;
	}

	if (current_time < pre_video_systime) {
		Reset();
		return;
	}

	if ((current_time - pre_video_systime) > (1 * 1000000)) {
		blog(LOG_WARNING,
		     "[obs_camera] %p['%s']: Current video is %llums behind previous frame.",
		     context, video_name.c_str(),
		     (current_time - pre_video_systime) / 1000);
	}

	++fps_video_count;

	// check fps
	uint64_t interval = (current_time - start_fps_systime);
	if (interval >= 1000000) {
		double fps =
			double(fps_video_count * 1000000) / double(interval);

		fps_list.push_back(fps);

		// start new circle for calculating FPS
		start_fps_systime = GetTimeIn1000NS();
		fps_video_count = 0;
	}

	// check frame late
	if (next_video_systime != 0) {
		if (current_time > next_video_systime) {
			uint64_t late_ms = (current_time / 1000 -
					    next_video_systime / 1000);
			late_list.push_back(late_ms);
		} else {
			late_list.push_back(0);
		}
	}

	interval = (current_time - pre_log_systime);
	if (!fps_list.empty() && !late_list.empty() &&
	    interval >= (1000000 * PER_STATISTICS_INTERVAL)) {
		SaveStatisticsLog(interval / 1000000);
	}
}

void PLSMonitorCameraVideo::EndHandleVideo()
{
	uint64_t current_time = GetTimeIn1000NS();
	pre_video_systime = current_time;
	next_video_systime = (current_time + expect_interval);
}

void PLSMonitorCameraVideo::SendCatchLogs()
{
	if (log_list.empty()) {
		return;
	}

	std::string log = "";
	std::for_each(log_list.begin(), log_list.end(),
		      [&log](const auto &item) {
			      log += item;
			      log += "\n";
		      });

	log_list.clear();

	blog(LOG_INFO, "[obs_camera] %p['%s']: Camera video information.\n%s",
	     context, video_name.c_str(), log.c_str());
}

void PLSMonitorCameraVideo::SaveStatisticsLog(uint64_t seconds)
{
	double avg_fps = 0;
	double min_fps = 0xffff;
	double max_fps = 0;

	std::for_each(fps_list.begin(), fps_list.end(),
		      [&avg_fps, &min_fps, &max_fps](const auto &fps) {
			      avg_fps += fps;
			      if (fps < min_fps) {
				      min_fps = fps;
			      }
			      if (fps > max_fps) {
				      max_fps = fps;
			      }
		      });

	avg_fps /= double(fps_list.size());

	uint64_t total_late = 0;
	uint64_t max_late = 0;
	uint64_t late_frames = 0;
	uint64_t total_frames = late_list.size();

	std::for_each(late_list.begin(), late_list.end(),
		      [&total_late, &max_late, &late_frames](const auto &late) {
			      total_late += late;
			      if (late > max_late) {
				      max_late = late;
			      }
			      if (late > 0) {
				      ++late_frames;
			      }
		      });

	double avg_late = double(total_late) / double(late_list.size());

	fps_list.clear();
	late_list.clear();
	pre_log_systime = GetTimeIn1000NS();

	auto clockNow = std::chrono::system_clock::now();
	auto timeNow = std::chrono::system_clock::to_time_t(clockNow);
	tm tmNow;
#ifdef _WIN32
	localtime_s(&tmNow, &timeNow);
#else
	tmNow = *localtime(&timeNow);
#endif
	std::array<char, 32> bufTime;
	strftime(bufTime.data(), bufTime.size(), "%Y-%m-%dT%H:%M:%S", &tmNow);

	std::array<char, 4096> log;
	snprintf(
		log.data(), log.size(),
		"%s+%llus FPS:%.2f minFPS:%.2f maxFPS:%.2f avgFPS:%.2f maxLate:%llums avgLate:%.2fms lateFrames:%llu/%llu",
		bufTime.data(), seconds, expect_fps, min_fps, max_fps, avg_fps,
		max_late, avg_late, late_frames, total_frames);

	log_list.emplace_back(log.data());

	if (log_list.size() >= STATISTICS_LIST_COUNT) {
		SendCatchLogs();
	}
}
