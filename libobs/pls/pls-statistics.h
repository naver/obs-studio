#pragma once

#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct pls_output_statistics pls_output_statistics_t;

	struct pls_statistics_item {
		int64_t packet_pts;
		int64_t encoding_start_ts;
		int64_t encoding_end_ts;
		int64_t rtmp_delivered_ts;
		int64_t rtmp_pop_ts;
		int64_t rtmp_sent_ts;
	};

	struct pls_statistics_dump {
		uint64_t encoder_avg_ts;
		uint64_t interleave_avg_ts;
		uint64_t rtmp_queue_avg_ts;
		uint64_t rtmp_send_avg_ts;
		uint64_t total_avg_ts;
	};

	EXPORT void pls_statistics_init(obs_output_t *output);
	EXPORT void pls_statistics_start(obs_output_t *output);
	EXPORT void pls_statistics_stop(obs_output_t *output);
	EXPORT void pls_statistics_log_pts(obs_output_t *output,struct pls_statistics_item update);
	EXPORT void pls_statistics_log_encoder_pts(struct obs_encoder *encoder, struct pls_statistics_item update);
	EXPORT void pls_statistics_apply_offset(obs_output_t *output, int64_t original_pts, int64_t adjusted_pts);
	EXPORT void pls_statistics_destroy(obs_output_t *output);

	EXPORT void pls_statistics_get_dump(obs_output_t *output, struct pls_statistics_dump *dump);
#ifdef __cplusplus
}
#endif
