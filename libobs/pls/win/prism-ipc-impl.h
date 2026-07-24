#pragma once
#include <functional>
#include <mutex>
#include <memory>
#include "prism-ipc-buffer.h"

/*
#define VB_ORIGINAL_WITHOUT_BLUR 0
#define VB_ORIGINAL_WITH_BLUR 1
#define VB_WITHOUT_BK 2
#define VB_WITH_BK 3
*/

#define LENS_RUNNING_FLAG L"com.prism.cam.running.flag" // lens running flag
#define PRISM_RUNNING_FLAG L"PRISMLiveStudio" // prism running flag, refer to PLSApp::runProgram

// Warning: Since upgrade of PRISM and LENS is not synchronized,
// so please note the compatibility of different versions between PRISM and LENS.
// If you want to add new shared variables, you must define new struct same as struct ipc_header_vb.
//-------------------------------------------------------------------------------------
#pragma pack(push, 1)
struct ipc_header_vb {
	int vb_orignal_type = 0; // same with "type" in "update_vb", only written in lens
	int vb_type = 0;         // same with "type" in "update_vb"
	bool prism_request_update = false;
	bool lens_actived = false;
	bool capture_valid = false;
	bool lens_on = false;
	int lens_ver_major = 0;
	int lens_ver_minor = 0;
	int lens_ver_build = 0;

	void reset_prism_variables() { prism_request_update = false; }
	void reset_lens_variables()
	{
		lens_actived = false;
		capture_valid = false;
		lens_on = false;
		lens_ver_major = 0;
		lens_ver_minor = 0;
		lens_ver_build = 0;
	}
};

struct default_ipc_sample {
	bool reserve;
};
#pragma pack(pop)

//-------------------------------------------------------------------------------------
class IpcSessionV4 : public CircleBufferIPC {
public:
	IpcSessionV4(std::string name)
		: CircleBufferIPC(name.c_str(), 0, sizeof(ipc_header_vb), 1,
				  sizeof(default_ipc_sample))
	{
	}
};

//-------------------------------------------------------------------------------------
class IExitCallback {
public:
	IExitCallback() {}
	virtual ~IExitCallback() {}

	virtual void reset_prism_variables() = 0;
	virtual void reset_lens_variables() = 0;
};

class IPCPrismLens;
template<typename Type, typename Head, typename Sample>
class SessionWrapper : public IExitCallback {
public:
	static std::string type() { return typeid(Type).name(); }

	SessionWrapper()
	{
		std::lock_guard<std::mutex> lock(IPCPrismLens::exit_callback_mutex);
		IPCPrismLens::exit_callbacks.push_back(this);
	}

	~SessionWrapper()
	{
		std::lock_guard<std::mutex> lock(IPCPrismLens::exit_callback_mutex);
		auto &cbs = IPCPrismLens::exit_callbacks;
		auto itr = find(cbs.begin(), cbs.end(), this);
		if (itr != cbs.end())
			cbs.erase(itr);
	}

	void init_session(int index)
	{
		assert(!session);
		if (!session) {
			std::string name = SessionWrapper::type() + std::to_string(index);
			session = std::make_shared<Type>(name);

			std::string errorMsg;
			DWORD err = session->InitMapBuffer(errorMsg);
			if (err != 0) {
				session = nullptr;
				assert(false);
			}

			if (session->newCreateMemory) {
				reset_prism_variables();
				reset_lens_variables();
			} else {
				if (IPCPrismLens::is_lens_app()) {
					reset_lens_variables();
				} else {
					reset_prism_variables();
				}
			}
		}
	}

	Head *map_write(int &out_index)
	{
		out_index = -1;
		if (!session || !session->IsBufferValid()) {
			return nullptr;
		}

		Head *header = nullptr;
		default_ipc_sample *sample = nullptr;
		int index = session->MapWrite((void **)&header, sizeof(Head), (void **)&sample,
					      sizeof(Sample));
		if (index < 0) {
			return nullptr;
		}

		out_index = index;
		return header;
	}

	void unmap_write(int index)
	{
		if (session) {
			session->UnmapWrite(index);
		}
	}

protected:
	void reset_prism_variables() override
	{
		int index;
		auto header = map_write(index);
		if (!header)
			return;

		header->reset_prism_variables();
		unmap_write(index);
	}

	void reset_lens_variables() override
	{
		int index;
		auto header = map_write(index);
		if (!header)
			return;

		header->reset_lens_variables();
		unmap_write(index);
	}

private:
	std::shared_ptr<Type> session = nullptr;
};

//-------------------------------------------------------------------------------------
// it is not thread-safe, you need add lock to protect it if necessary
class IPCPrismLens {
public:
	static std::mutex exit_callback_mutex;
	static std::vector<IExitCallback *> exit_callbacks;

	static bool is_lens_app();
	static bool is_handle_existed(const wchar_t *name);

	static bool is_license_verified();
	static void notify_license_verified(bool verified);

	static bool is_lens_ui_controllable();
	static void notify_lens_ui_state(bool ui_controllable);

	//-----------------------------------------------------------------
	IPCPrismLens(int cam_index);
	virtual ~IPCPrismLens();

	void init_ipc_sessions();
	void tick();

	//---------------------------- vb ipc -----------------------------
	// vb: called by prism
	bool is_vb_removed();
	void request_lens_remove_vb(bool need_remove);

	// vb: called by lens
	void notify_lens_ui_vb_type(int vb_type);
	int get_prism_requested_vb_type();

	//---------------------------- active&capture state ---------------
	bool notify_active_state(bool active);
	bool notify_capture_state(bool valid);
	bool notify_lens_onoff(bool is_on);
	bool is_lens_actived();
	bool is_capture_valid();
	bool is_lens_on();

	//---------------------------- lens version -----------------------------
	void notify_lens_version(int major, int minor, int build);
	void get_lens_version(int &major, int &minor, int &build);

protected:
	bool time_to_tick();

private:
	const int camera_index = -1;
	DWORD64 pre_tick_time = 0;

	SessionWrapper<IpcSessionV4, ipc_header_vb, default_ipc_sample> ipcVB;
};

class CSignalEvent {
public:
	CSignalEvent(const char *name);
	~CSignalEvent();

	void set_sign(bool sign_flag);
	bool get_sign();

private:
	HANDLE event = 0;
};