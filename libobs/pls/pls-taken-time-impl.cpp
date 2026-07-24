#include "pls-taken-time-impl.h"

std::atomic<bool> time_uploader::render_drop_enabled = false;

//---------------------------------------------------------------------------------------------------------------
time_record_item::time_record_item()
{
	times_ms.reserve(120);
}

void time_record_item::insert(uint64_t ms, bool first_one)
{
	uint64_t now_tm = os_gettime_ns() / 1000000;
	if (now_tm - start_time_ms >= RECORD_ITEM_DURATION) {
		if (!first_one)
			times_ms.push_back(max_value); // insert max value for the past RECORD_ITEM_DURATION

		max_value = ms;
		start_time_ms = now_tm;
	} else {
		if (max_value < ms) {
			max_value = ms;
		}
	}
}

void time_record_item::complete()
{
	if (start_time_ms > 0) {
		times_ms.push_back(max_value);
	}
}

std::string time_record_item::generate_string(uint64_t &max_ms) const
{
	max_ms = 0;

	std::string result;
	result.reserve(1024);

	bool saved = false;
	for (const auto &t : times_ms) {
		if (t > max_ms)
			max_ms = t;

		if (t <= 3)
			continue; // ignore little taken time

		if (!result.empty())
			result += ",";

		saved = true;
		result += std::to_string(t);
	}

	if (!saved) {
		result = "max=";
		result += std::to_string(max_ms);
	}

	return result;
}

//---------------------------------------------------------------------------------------------------------------
time_uploader::time_uploader()
{
	start();
}

time_uploader::~time_uploader()
{
	stop();
}

static std::string generate_key(void *obj, const char *obj_plugin, const char *desc)
{
	std::ostringstream oss;

	if (obj)
		oss << std::hex << obj << " ";

	if (obj_plugin && *obj_plugin)
		oss << obj_plugin << " ";

	if (desc && *desc)
		oss << desc;

	std::string result = oss.str();
	return result;
}

void time_uploader::start()
{
	assert(!thread_hdl);
	thread_hdl = new std::thread(&time_uploader::upload_thread, this);
}

void time_uploader::stop()
{
	if (thread_hdl) {
		{
			std::unique_lock<std::mutex> lock(lock_thread);
			need_exit = true; // firstly set the flag, then notify event
			exit_event.notify_one();
		}

		if (thread_hdl->joinable()) {
			thread_hdl->join();
		}

		delete thread_hdl;
		thread_hdl = nullptr;
	}
}

void time_uploader::begin_taken_time(void *obj, const char *obj_plugin, const char *desc)
{
	std::string key = generate_key(obj, obj_plugin, desc);

	bool stack_empty = item_stack.empty();
	std::string parent_key = "";
	if (!stack_empty)
		parent_key = item_stack.top().key;

	time_stack_item item;
	item.key = key;
	item.start_time = os_gettime_ns();
	item_stack.push(std::move(item));

	//----------------------------------- insert function tree ------------------------------------
	std::lock_guard<std::recursive_mutex> lock(lock_cache);

	if (tree_item_cache.find(key) != tree_item_cache.end())
		return; // it is already cached

	auto new_item = std::make_shared<func_tree_item>();
	new_item->key = key;

	tree_item_cache.insert(std::make_pair(key, new_item));

	if (stack_empty) {
		top_tree_item.push_back(new_item);
	} else {
		auto itr = tree_item_cache.find(parent_key);
		if (itr != tree_item_cache.end()) {
			itr->second->child_items.push_back(new_item);
		} else {
			// tree_item_cache can be cleared in worker thread, then failed to find parent_key
		}
	}
}

void time_uploader::end_taken_time(void *obj, const char *obj_plugin, const char *desc, uint64_t min_ns)
{
	if (item_stack.empty()) {
		std::string key = generate_key(obj, obj_plugin, desc);
		if (!error_stacks[key]) {
			error_stacks[key] = true;
			blog(LOG_ERROR, "%s stack is empty. key=%s", __FUNCTION__, key.c_str());
		}

		assert(false); // wrong stack
		return;
	}

	auto now_ns = os_gettime_ns();

	time_stack_item top = item_stack.top();
	item_stack.pop();

	std::string key = generate_key(obj, obj_plugin, desc);
	if (key != top.key) {
		if (!error_stacks[key]) {
			error_stacks[key] = true;
			blog(LOG_ERROR, "%s stack is not paired. key=%s top.key=%s", __FUNCTION__, key.c_str(),
			     top.key.c_str());
		}

		assert(false); // wrong stack
		return;
	}

	//----------------------------------- insert time ---------------------------------------------
	auto tm_ns = (now_ns - top.start_time);
	if (min_ns > 0 && tm_ns < min_ns)
		return;

	insert_time(key, tm_ns / 1000000);
}

void time_uploader::insert_time(const std::string &key, uint64_t ms)
{
	if (key.empty()) {
		assert(false);
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(lock_cache);

	if (!render_drop_enabled) { // during app's startup or switching scene collector, we should ignore the taken time records
		return;
	}

	auto itr = cached_data.find(key);
	if (itr == cached_data.end()) {
		auto item = std::make_shared<time_record_item>();
		item->insert(ms, true);
		cached_data[key] = item;
	} else
		itr->second->insert(ms, false);
}

void time_uploader::get_cache(std::unordered_map<std::string, std::shared_ptr<time_record_item>> &time_items,
			      std::unordered_map<std::string, std::shared_ptr<func_tree_item>> &tree_items,
			      std::vector<std::shared_ptr<func_tree_item>> &top_tree_items)
{
	std::lock_guard<std::recursive_mutex> lock(lock_cache);
	time_items.swap(cached_data);
	tree_items.swap(tree_item_cache);
	top_tree_items.swap(top_tree_item);
}

void send_func_item(const std::unordered_map<std::string, std::shared_ptr<time_record_item>> &time_items,
		    const std::shared_ptr<func_tree_item> func_item, int dep)
{
	if (time_items.empty() || !func_item)
		return;

	std::string deps;
	deps.reserve(dep);
	deps.assign(dep, '\t');

	const auto &key = func_item->key;
	auto itr = time_items.find(key);
	if (itr != time_items.end() && itr->second && !itr->second->times_ms.empty()) {
		uint64_t max_ms = 0;
		std::string text = itr->second->generate_string(max_ms);
		if (dep == 0) {
			blog(LOG_INFO, "TakenTime [%s] [%zu] %s", key.c_str(), itr->second->times_ms.size(),
			     text.c_str());
		} else {
			bool blocked = max_ms >= BLOCK_SRE_THRESHOLD;
			if (blocked && func_item->child_items.empty()) {
				std::string tm = std::to_string(max_ms);
				const char *fields[][2] = {
					{"blocked_object", key.c_str()},
					{"max_block_ms", tm.c_str()},
				};
				blogex(false, LOG_INFO, fields, 2, "%s | TakenTime [%s] [%zu] %s [blocked]",
				       deps.c_str(), key.c_str(), itr->second->times_ms.size(), text.c_str());
			} else {
				blog(LOG_INFO, "%s | TakenTime [%s] [%zu] %s %s", deps.c_str(), key.c_str(),
				     itr->second->times_ms.size(), text.c_str(), blocked ? "[blocked]" : "");
			}
		}
	}

	for (const auto &temp : func_item->child_items) {
		send_func_item(time_items, temp, dep + 1);
	}
}

void time_uploader::send_cache(const std::unordered_map<std::string, std::shared_ptr<time_record_item>> &time_items,
			       const std::unordered_map<std::string, std::shared_ptr<func_tree_item>> &tree_items,
			       const std::vector<std::shared_ptr<func_tree_item>> &top_tree_items)
{
	if (time_items.empty() || top_tree_items.empty() || tree_items.empty())
		return;

	for (const auto &itr : time_items) {
		if (itr.second)
			itr.second->complete();
	}

	for (const auto &top : top_tree_items) {
		auto itr = time_items.find(top->key);
		if (itr != time_items.end() && itr->second && !itr->second->times_ms.empty()) {
			bool exception = false;
			for (const auto &tm : itr->second->times_ms) {
				if (tm >= 10) { // if render loop takes more than 10ms, we will send the taken time
					exception = true;
					break;
				}
			}

			if (exception)
				send_func_item(time_items, top, 0);
			else
				blog(LOG_INFO, "[%s] %s TakenTime is normal", __FUNCTION__, top->key.c_str());
		}
	}
}

void time_uploader::check_cache()
{
	std::unordered_map<std::string, std::shared_ptr<time_record_item>> time_items;
	std::unordered_map<std::string, std::shared_ptr<func_tree_item>> tree_items;
	std::vector<std::shared_ptr<func_tree_item>> top_tree;

	get_cache(time_items, tree_items, top_tree);
	send_cache(time_items, tree_items, top_tree);
}

void time_uploader::upload_thread()
{
	blog(LOG_INFO, "%s enter", __FUNCTION__);

	while (true) {
		std::unique_lock<std::mutex> lock(lock_thread);
		exit_event.wait_for(lock, std::chrono::milliseconds(UPLOAD_INTERVAL_MS));
		if (need_exit)
			break;

		check_cache();
	}

	check_cache();

	blog(LOG_INFO, "%s leave", __FUNCTION__);
}
