#if defined(_WIN32)
#pragma once
#include <string>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <Windows.h>
#include <Dbt.h>
#include <ks.h>
#include <ksmedia.h>

#define INVALID_NOTIFY_ID -1
enum DeviceEvent {
	Inserted = 0,
	Removed,
	Occupied,
};

using DeviceNotificationCallback = std::function<void(const std::wstring &id, DeviceEvent e)>;
struct NotifyCallbackInfo {
	std::string name;
	DeviceNotificationCallback callback;
};

class DeviceNotification {
public:
	static DeviceNotification *instance();
	~DeviceNotification();

	int Subscribe(DeviceNotificationCallback callback);
	void Unsubscribe(int idx);
	void UnsubscribeAll();
	HRESULT CheckDeviceLost(DEV_BROADCAST_HDR *pHdr, WPARAM wParam);
	void NotifyThread();

private:
	DeviceNotification();
	bool CreateMessageWindow();
	BOOL RegisterForDeviceNotification(HWND hwnd);

	std::recursive_mutex m_mutex;
	std::map<std::string, DeviceNotificationCallback> m_callbacks;
	HWND m_hwnd = nullptr;
	HDEVNOTIFY m_hdevnotify = nullptr;
	int m_idx = 0;
	std::thread m_thread;
	std::atomic_bool m_threadExit = false;
};
#endif
