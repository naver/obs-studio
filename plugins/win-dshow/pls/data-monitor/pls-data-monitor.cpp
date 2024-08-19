#include "pls-data-monitor.h"
#include "util/base.h"
#include <mutex>

DataMonitor::DataMonitor(uint64_t timeMs, std::function<void()> callback, void *context)
	: m_timeMs(timeMs),
	  m_callback(callback),
	  m_pContext(context)
{
	m_thread = std::thread([this]() {
		uint64_t tickCount = 0;
		blog(LOG_INFO, "[obs_camera] %p: thread started. DataMonitor()", (void *)m_pContext);
		while (!m_stop) {
			if (m_reset) {
				std::this_thread::sleep_for(std::chrono::milliseconds(15));
				continue;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(15));

			m_mutex.lock();
			bool timeValid = CheckData();
			m_mutex.unlock();

			if (!timeValid) {
				m_callback();
			}
		}

		blog(LOG_INFO, "[obs_camera] %p: thread end. DataMonitor()", (void *)m_pContext);
	});
}

DataMonitor::~DataMonitor()
{
	m_stop = true;
	if (m_thread.joinable())
		m_thread.join();
}

void DataMonitor::PushData()
{
	std::lock_guard locker(m_mutex);
	if (m_reset)
		m_reset = false;
	m_lastTimeStamp = std::chrono::steady_clock::now();
}

bool DataMonitor::CheckData()
{
	auto duration = std::chrono::steady_clock::now() - m_lastTimeStamp;
	if (std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() >= static_cast<long long>(m_timeMs))
		return false;
	return true;
}

void DataMonitor::Reset()
{
	std::lock_guard locker(m_mutex);
	m_reset = true;
	m_lastTimeStamp = std::chrono::steady_clock::now();
}

void DataMonitor::Resum()
{
	m_reset = false;
}

void DataMonitor::Stop() {
	m_stop = true;
	if (m_thread.joinable())
		m_thread.join();
}
