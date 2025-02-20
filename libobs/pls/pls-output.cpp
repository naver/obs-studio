#include "pls-output.h"
#include "pls-base.h"
#include <map>
#include <mutex>
#include <chrono>
#include <optional>

std::recursive_mutex mutex;
std::map<void *, std::chrono::steady_clock::time_point> g_logTimeGap;

static std::optional<uint64_t> getTimeGapMs(void *rtmp)
{
	std::optional<uint64_t> value;
	if (!rtmp)
		return value;

	if (auto iter = g_logTimeGap.find(rtmp); iter != g_logTimeGap.end()) {
		auto now = std::chrono::steady_clock::now();
		value = std::chrono::duration_cast<std::chrono::milliseconds>(
				now - (*iter).second)
				.count();
	}

	return value;
}

void pls_rtmp_connect_success(void *rtmp, void *output)
{
	std::lock_guard locker(mutex);
	if (rtmp && output) {
		auto now = std::chrono::steady_clock::now();
		g_logTimeGap[rtmp] = now;
	}
}

void pls_rtmp_log_event_time_gap(void *rtmp, void *output, const char *event)
{
	std::lock_guard locker(mutex);
	if (rtmp && output && event) {
		if (auto gap = getTimeGapMs(rtmp); gap.has_value()) {
			//PRISM/Xiewei/20241104/PRISM_PC-1673/Add logs
			const char *fields[][2] = {
				{PTS_LOG_TYPE, PTS_TYPE_EVENT}};
			blogex(false, LOG_INFO, fields, 1,
			       "output=%p rtmp=%p Connection rtmp <-> %s. time gap: %lld ms",
			       output, rtmp, event, gap.value());
		}
	}
}
