#include "pls-statistics.h"
#include "pls-base.h"
#include "../util/threading.h"
#include "../util/base.h"
#include "../util/darray.h"

#include "obs-internal.h"

struct pls_output_statistics {
	DARRAY(struct pls_statistics_item) items;
	pthread_t tick_thread;
	bool ticking;
	os_event_t *tick_event;
	obs_output_t *current_output;
};

static bool pls_statistics_valid_item(struct pls_statistics_item *item) {
	return
	item->encoding_start_ts != 0 &&
	item->encoding_end_ts != 0 &&
	item->rtmp_pop_ts != 0 &&
	item->rtmp_delivered_ts != 0 &&
	item->rtmp_sent_ts != 0;
}

static void pls_statistics_update_item(struct pls_statistics_item *item, struct pls_statistics_item *update) {
	if (update->encoding_start_ts > 0) {
		item->encoding_start_ts = update->encoding_start_ts;
	}
	if (update->encoding_end_ts > 0) {
		item->encoding_end_ts = update->encoding_end_ts;
	}

	if (update->rtmp_delivered_ts > 0) {
		item->rtmp_delivered_ts = update->rtmp_delivered_ts;
	}

	if (update->rtmp_pop_ts > 0) {
		item->rtmp_pop_ts = update->rtmp_pop_ts;
	}
	if (update->rtmp_sent_ts > 0) {
		item->rtmp_sent_ts = update->rtmp_sent_ts;
	}
}

void pls_statistics_get_dump(obs_output_t *output, struct pls_statistics_dump *dump) {
	assert(output);

	memset(dump, 0, sizeof(struct pls_statistics_dump));

	if (!output) {
		return;
	}

	struct pls_output_statistics *statistics = output->statistics;
	if (!statistics) {
		return;
	}

	pthread_mutex_lock(&output->statistics_mutex);

	uint64_t encoder_sum_ts = 0;
	uint64_t interleave_sum_ts = 0;
	uint64_t rtmp_queue_ts = 0;
	uint64_t rtmp_send_ts = 0;
	uint64_t total_ts = 0;

	size_t valid_num = 0;
	DARRAY(struct pls_statistics_item) copied_items;
	da_init(copied_items);
	da_copy(copied_items, statistics->items);

	for (size_t i = 0; i < copied_items.num; i++) {
		struct pls_statistics_item *item = copied_items.array + i;

		if (!pls_statistics_valid_item(item)) {
			continue;
		}

		encoder_sum_ts += (item->encoding_end_ts - item->encoding_start_ts);
		interleave_sum_ts += (item->rtmp_delivered_ts - item->encoding_end_ts);
		rtmp_queue_ts += (item->rtmp_pop_ts - item->rtmp_delivered_ts);
		rtmp_send_ts += (item->rtmp_sent_ts - item->rtmp_pop_ts);
		total_ts += (item->rtmp_sent_ts - item->encoding_start_ts);

		valid_num += 1;
		da_erase_item(statistics->items, item);
	}

	if (valid_num == 0) {
		da_free(copied_items);
		pthread_mutex_unlock(&output->statistics_mutex);
		return;
	}

	da_free(copied_items);
	pthread_mutex_unlock(&output->statistics_mutex);

	uint64_t encoder_avg_ts = encoder_sum_ts / valid_num;
	uint64_t interleave_avg_ts = interleave_sum_ts / valid_num;
	uint64_t rtmp_queue_avg_ts = rtmp_queue_ts / valid_num;
	uint64_t rtmp_send_avg_ts = rtmp_send_ts / valid_num;
	uint64_t total_avg_ts = total_ts / valid_num;

	dump->encoder_avg_ts = encoder_avg_ts;
	dump->interleave_avg_ts = interleave_avg_ts;
	dump->rtmp_queue_avg_ts = rtmp_queue_avg_ts;
	dump->rtmp_send_avg_ts = rtmp_send_avg_ts;
	dump->total_avg_ts = total_avg_ts;
}

	// static void pls_statistics_send_log(obs_output_t *output) {
	// 	struct pls_statistics_dump *dump = pls_statistics_dump_log(output);
	// 	if (!dump) {
	// 		return;
	// 	}

	// 	uint64_t encoder_avg_ts = dump->encoder_avg_ts;
	// 	uint64_t interleave_avg_ts = dump->interleave_avg_ts;
	// 	uint64_t rtmp_queue_avg_ts = dump->rtmp_queue_avg_ts;
	// 	uint64_t rtmp_send_avg_ts = dump->rtmp_send_avg_ts;
	// 	uint64_t total_avg_ts = dump->total_avg_ts;

	// 	char encoder_avg_ts_s[64];
	// 	snprintf(encoder_avg_ts_s, sizeof(encoder_avg_ts_s), "%f", encoder_avg_ts / 1000000.0);

	// 	char interleave_avg_ts_s[64];
	// 	snprintf(interleave_avg_ts_s, sizeof(interleave_avg_ts_s), "%f", interleave_avg_ts / 1000000.0);

	// 	char rtmp_queue_avg_ts_s[64];
	// 	snprintf(rtmp_queue_avg_ts_s, sizeof(rtmp_queue_avg_ts_s), "%f", rtmp_queue_avg_ts / 1000000.0);

	// 	char rtmp_send_avg_ts_s[64];
	// 	snprintf(rtmp_send_avg_ts_s, sizeof(rtmp_send_avg_ts_s), "%f", rtmp_send_avg_ts / 1000000.0);

	// 	char total_avg_ts_s[64];
	// 	snprintf(total_avg_ts_s, sizeof(total_avg_ts_s), "%f", total_avg_ts / 1000000.0);

	// 	char total_avg_seconds_s[64];
	// 	int64_t total_avg_seconds = (int64_t)floor(total_avg_ts / 1000000000.0);
	// 	if (total_avg_seconds < 1) {
	// 		strcpy(total_avg_seconds_s, "less than 1s");
	// 	} else {
	// 		snprintf(total_avg_seconds_s, sizeof(total_avg_seconds_s), "%llds", total_avg_seconds);
	// 	}

	// 	char stream_delay_sec_s[64];
	// 	snprintf(stream_delay_sec_s, sizeof(stream_delay_sec_s), "%d", output->delay_sec);

	// 	const char *fields[][2] = {
	// 		{"rtmp_encoder_latency", encoder_avg_ts_s},
	// 		{"rtmp_interleave_latency", interleave_avg_ts_s},
	// 		{"rtmp_queue_latency", rtmp_queue_avg_ts_s},
	// 		{"rtmp_send_latency", rtmp_send_avg_ts_s},
	// 		{"rtmp_total_latency", total_avg_ts_s},
	// 		{"rtmp_total_latency_seconds", total_avg_seconds_s}
	// 	};

	// 	blogex(false, LOG_INFO, fields, 6,
	// 		   "%p-%s -> rtmp_total_latency_seconds: %s\nrtmp_total_latency: %sms\nrtmp_encoder_latency: %sms\nrtmp_interleave_latency: %sms\nrtmp_queue_latency: %sms\nrtmp_send_latency: %sms\nstream_delay: %ss\n",
	// 		   output, __FUNCTION__, total_avg_seconds_s, total_avg_ts_s, encoder_avg_ts_s,
	// 		   interleave_avg_ts_s, rtmp_queue_avg_ts_s, rtmp_send_avg_ts_s, stream_delay_sec_s);

	// 	bfree(dump);
	// }

	// static void* pls_statistics_tick(void *params) {
	// 	struct obs_output *output = (struct obs_output *)params;
	// 	struct pls_output_statistics *statistics = output->statistics;
	// 		// every 30 seconds
	// 	while (os_event_timedwait(statistics->tick_event, 30000) == ETIMEDOUT) {
	// 			// send the log
	// 		pls_statistics_send_log(output);
	// 	}
	// 	return NULL;
	// }

EXPORT void pls_statistics_init(obs_output_t *output) {
	if (strcmp(obs_output_get_id(output), "rtmp_output") != 0) {
		return;
	}

		// ignore new_socket_loop
	obs_data_t *settings = obs_output_get_settings(output);
	if (settings) {
		bool new_socket_loop_enabled = obs_data_get_bool(settings, "new_socket_loop_enabled");
		obs_data_release(settings);

		if (new_socket_loop_enabled) {
			return;
		}
	}

	struct pls_output_statistics *statistics = bmalloc(sizeof(struct pls_output_statistics));
	statistics->ticking = false;

	pthread_mutex_init(&output->statistics_mutex, NULL);

	da_init(statistics->items);
		// os_event_init(&statistics->tick_event, OS_EVENT_TYPE_AUTO);

	output->statistics = statistics;
}

EXPORT void pls_statistics_start(obs_output_t *output) {
	struct pls_output_statistics *statistics = output->statistics;
	if (!statistics) {
		return;
	}

	pls_statistics_stop(output);

		// 	pthread_mutex_lock(&output->statistics_mutex);
		//
		// 	pthread_create(&statistics->tick_thread, NULL, pls_statistics_tick,
		// 				   output);
		// 	statistics->ticking = true;
		//
		// 	pthread_mutex_unlock(&output->statistics_mutex);
}

EXPORT void pls_statistics_stop(obs_output_t *output) {
	struct pls_output_statistics *statistics = output->statistics;
	if (!statistics) {
		return;
	}

	pthread_mutex_lock(&output->statistics_mutex);

	da_clear(statistics->items);

	pthread_mutex_unlock(&output->statistics_mutex);

		//		// Signal the thread to stop before acquiring the mutex
		//	os_event_signal(statistics->tick_event);
		//
		//		// Store thread handle and ticking state while holding mutex
		//	pthread_t thread = statistics->tick_thread;
		//	bool should_join = false;
		//
		//	pthread_mutex_lock(&output->statistics_mutex);
		//
		//	os_event_signal(statistics->tick_event);
		//
		//#if __APPLE__
		//	should_join = statistics->tick_thread != 0;
		//#else
		//	should_join = statistics->ticking;
		//#endif
		//	statistics->ticking = false;
		//
		//	da_clear(statistics->items);
		//
		//	pthread_mutex_unlock(&output->statistics_mutex);
		//
		//		// Join thread after releasing mutex
		//	if (should_join) {
		//		pthread_join(thread, NULL);
		//	}
		//
		//		// Reset event after thread is joined
		//	os_event_reset(statistics->tick_event);
}

EXPORT void pls_statistics_log_pts(obs_output_t *output, struct pls_statistics_item update) {
	assert(output);

	if (!output) {
		return;
	}

	struct pls_output_statistics *statistics = output->statistics;
	if (!statistics) {
		return;
	}

	pthread_mutex_lock(&output->statistics_mutex);

	size_t current_size = statistics->items.num;
	struct pls_statistics_item *exist_item = NULL;
	for (size_t i = 0; i < current_size; i++) {
		struct pls_statistics_item *item = statistics->items.array + i;
		if (item->packet_pts == update.packet_pts) {
			exist_item = item;
			break;
		}
	}

	if (exist_item) {
		pls_statistics_update_item(exist_item, &update);
	} else {
		da_push_back(statistics->items, &update);
	}
	pthread_mutex_unlock(&output->statistics_mutex);
}

EXPORT void pls_statistics_log_encoder_pts(struct obs_encoder *encoder, struct pls_statistics_item update) {
	pthread_mutex_lock(&encoder->outputs_mutex);
	for (size_t i = 0; i < encoder->outputs.num; i++) {
		struct obs_output *output = encoder->outputs.array[i];
		if (obs_output_active(output)) {
			pls_statistics_log_pts(output, update);
		}
	}
	pthread_mutex_unlock(&encoder->outputs_mutex);
}

EXPORT void pls_statistics_destroy(obs_output_t *output) {
	struct pls_output_statistics *statistics = output->statistics;
	if (!statistics) {
		return;
	}

	pls_statistics_stop(output);

	pthread_mutex_lock(&output->statistics_mutex);

	da_free(statistics->items);

	output->statistics = NULL;
	struct pls_output_statistics *stats_to_free = statistics;

	pthread_mutex_unlock(&output->statistics_mutex);

	pthread_mutex_destroy(&output->statistics_mutex);

	bfree(stats_to_free);
}

EXPORT void pls_statistics_apply_offset(obs_output_t *output, int64_t original_pts, int64_t adjusted_pts) {
	struct pls_output_statistics *statistics = output->statistics;
	if (!statistics) {
		return;
	}

	pthread_mutex_lock(&output->statistics_mutex);

	for (size_t i = 0; i < statistics->items.num; i++) {
		struct pls_statistics_item *item = statistics->items.array + i;
		if (item->packet_pts == original_pts) {
			item->packet_pts = adjusted_pts;
			break;
		}
	}

	pthread_mutex_unlock(&output->statistics_mutex);
}
