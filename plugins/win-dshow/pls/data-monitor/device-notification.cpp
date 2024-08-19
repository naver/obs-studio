#if defined(_WIN32)
#include <algorithm>
#include "device-notification.h"
#include "util/base.h"
#include "util/util.hpp"
#include "util/platform.h"

constexpr auto deviceNotificationClassName = L"PrismLiveStudioCameraDeviceNotificationClassname";

static std::wstring str_tolower(std::wstring s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		       [](wchar_t c) { return std::tolower(c); });
	return s;
}

DeviceNotification *DeviceNotification::instance()
{
	static DeviceNotification deviceNotify;
	return &deviceNotify;
}

DeviceNotification::DeviceNotification()
{
	m_thread = std::thread(&DeviceNotification::NotifyThread, this);
}

DeviceNotification::~DeviceNotification()
{
	m_threadExit = true;

	if (m_hdevnotify) {
		UnregisterDeviceNotification(m_hdevnotify);
	}

	if (m_hwnd) {
		DestroyWindow(m_hwnd);
		UnregisterClass(deviceNotificationClassName, GetModuleHandle(nullptr));
	}

	if (m_thread.joinable())
		m_thread.join();
}

int DeviceNotification::Subscribe(DeviceNotificationCallback callback)
{
	if (!callback)
		return INVALID_NOTIFY_ID;

	std::lock_guard locker(m_mutex);
	auto idx = m_idx;
	m_callbacks[std::to_string(idx)] = callback;
	m_idx++;
	return idx;
}

void DeviceNotification::Unsubscribe(int idx)
{
	if (idx == INVALID_NOTIFY_ID)
		return;

	std::lock_guard locker(m_mutex);
	auto iter = m_callbacks.find(std::to_string(idx));
	if (iter != m_callbacks.end()) {
		m_callbacks.erase(iter);
	}
}

void DeviceNotification::UnsubscribeAll()
{
	std::lock_guard locker(m_mutex);
	m_callbacks.clear();
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_DEVICECHANGE:
		if (lParam != 0) {
			auto long_ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
			DeviceNotification *param =
				reinterpret_cast<DeviceNotification *>(long_ptr);
			if (param) {
				(void)param->CheckDeviceLost((PDEV_BROADCAST_HDR)lParam, wParam);
			}
		}
		break;
	default:
		break;
	}

	return TRUE;
}

bool DeviceNotification::CreateMessageWindow()
{
	WNDCLASSEX wcx = {};
	wcx.cbSize = sizeof(WNDCLASSEX);                 // size of structure
	wcx.lpfnWndProc = MainWndProc;                   // points to window procedure
	wcx.hInstance = GetModuleHandle(nullptr);        // handle to instance
	wcx.lpszClassName = deviceNotificationClassName; // name of window class
	// Register the window class.
	if (!RegisterClassEx(&wcx)) {
		auto error_code = GetLastError();
		blog(LOG_WARNING, "Failed to register class: %lu", error_code);
		return false;
	}

	// Create the main window.
	m_hwnd = CreateWindowEx(0, deviceNotificationClassName, nullptr, 0, 0, 0, 0, 0,
				HWND_MESSAGE, nullptr, GetModuleHandle(nullptr), nullptr);
	if (!m_hwnd) {
		auto error_code = GetLastError();
		blog(LOG_WARNING, "Failed to create window: %lu", error_code);
		UnregisterClass(deviceNotificationClassName, GetModuleHandle(nullptr));
		return false;
	}

	// set user param
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	return true;
}

BOOL DeviceNotification::RegisterForDeviceNotification(HWND hwnd)
{
	DEV_BROADCAST_DEVICEINTERFACE di = {0};
	di.dbcc_size = sizeof(di);
	di.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	di.dbcc_classguid = KSCATEGORY_CAPTURE;

	m_hdevnotify = RegisterDeviceNotification(hwnd, &di, DEVICE_NOTIFY_WINDOW_HANDLE);

	if (m_hdevnotify == NULL) {
		return FALSE;
	}

	return TRUE;
}

HRESULT DeviceNotification::CheckDeviceLost(DEV_BROADCAST_HDR *pHdr, WPARAM wParam)
{
	DEV_BROADCAST_DEVICEINTERFACE *pDi = NULL;

	if (pHdr == NULL) {
		return S_OK;
	}
	if (pHdr->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE) {
		return S_OK;
	}

	if (DBT_DEVICEARRIVAL != wParam && DBT_DEVICEREMOVECOMPLETE != wParam) {
		return S_OK;
	}

	std::lock_guard locker(m_mutex);

	// Compare the device name with the symbolic link.
	pDi = (DEV_BROADCAST_DEVICEINTERFACE *)pHdr;

	bool arrival = DBT_DEVICEARRIVAL == wParam;
	DeviceEvent device_event = (arrival ? Inserted : Removed);
	for (const auto &item : m_callbacks) {
		if (item.second) {
			item.second(pDi->dbcc_name, device_event);
		}
	}

	return S_OK;
}

void DeviceNotification::NotifyThread()
{
	blog(LOG_INFO, "thread started. DeviceNotification::NotifyThread()");
	bool result = CreateMessageWindow();
	if (result) {
		RegisterForDeviceNotification(m_hwnd);
	}

	while (!m_threadExit) {
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	blog(LOG_INFO, "thread end. DeviceNotification::NotifyThread()");
}
#endif
