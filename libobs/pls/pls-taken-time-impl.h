#include "pls-base.h"
#include <sstream>
#include <assert.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <stack>
#include <condition_variable>
extern "C" {
#include "util/platform.h"
}

// in ms, interval for uploading NELO log
#define UPLOAD_INTERVAL_MS 30000

// in ms, insert one record item every RECORD_ITEM_DURATION, the inserted value is the max value in this duration
#define RECORD_ITEM_DURATION 1000

// in ms
#define BLOCK_SRE_THRESHOLD 10

struct time_stack_item {
	std::string key = "";
	uint64_t start_time = 0; // in ns
};

struct func_tree_item {
	std::string key = "";
	std::vector<std::shared_ptr<func_tree_item>> child_items;
};

//---------------------------------------------------------------------------------------------------------------
struct time_record_item {
	uint64_t start_time_ms = 0;     // the start time of the current RECORD_ITEM_DURATION
	uint64_t max_value = 0;         // we insert the max value for every RECORD_ITEM_DURATION
	std::vector<uint64_t> times_ms; // only insert one time every RECORD_ITEM_DURATION

	time_record_item();

	void insert(uint64_t ms, bool first_one);
	void complete();
	std::string generate_string(uint64_t &max_ms) const;
};

//---------------------------------------------------------------------------------------------------------------
class time_uploader {
public:
	static std::atomic<bool> render_drop_enabled;

	//------------------------------------------------------------------
	time_uploader();
	~time_uploader();

	void start();
	void stop();

	void begin_taken_time(void *obj, const char *obj_plugin, const char *desc);
	void end_taken_time(void *obj, const char *obj_plugin, const char *desc, uint64_t min_ns);

protected:
	void insert_time(const std::string &key, uint64_t ms);

	void upload_thread();
	void check_cache();

	void get_cache(std::unordered_map<std::string, std::shared_ptr<time_record_item>> &time_items,
		       std::unordered_map<std::string, std::shared_ptr<func_tree_item>> &tree_items,
		       std::vector<std::shared_ptr<func_tree_item>> &top_tree_items);
	void send_cache(const std::unordered_map<std::string, std::shared_ptr<time_record_item>> &time_items,
			const std::unordered_map<std::string, std::shared_ptr<func_tree_item>> &tree_items,
			const std::vector<std::shared_ptr<func_tree_item>> &top_tree_items);

private:
	std::stack<time_stack_item> item_stack; // will not be accessed from mult-threads

	// need using {lock_cache}
	std::unordered_map<std::string, std::shared_ptr<func_tree_item>> tree_item_cache;
	std::vector<std::shared_ptr<func_tree_item>> top_tree_item; // here store the tree structure for functions

	std::recursive_mutex lock_cache;
	std::unordered_map<std::string, std::shared_ptr<time_record_item>> cached_data;

	std::unordered_map<std::string, bool> error_stacks; // bool: whether log is sent

	std::atomic<bool> need_exit = false;
	std::mutex lock_thread;
	std::condition_variable exit_event;
	std::thread *thread_hdl = nullptr;
};
