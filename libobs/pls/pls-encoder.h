#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//PRISM/cao.kewei/20240514/#5378/force keyframe
EXPORT bool obs_encoder_request_keyframe(obs_encoder_t *encoder);
EXPORT bool obs_encoder_request_keyframe_and_clean(obs_encoder_t *encoder);
EXPORT void obs_encoder_set_request_keyframe(obs_encoder_t *encoder,
					     bool keyframe);

EXPORT void obs_encoder_on_encoded_frame(obs_encoder_t *encoder, bool keyframe);

EXPORT bool encoder_alive_func(obs_encoder_t *encoder, const char *func_name, int line);
EXPORT void on_encoder_created(obs_encoder_t *encoder);
EXPORT void on_encoder_destoryed(obs_encoder_t *encoder);

#define encoder_alive(encoder) encoder_alive_func(encoder, __FUNCTION__, __LINE__)

typedef struct PtsStats {
	double sum;
	int64_t count;
	double min;
	double max;
	uint64_t start_time;
	const obs_encoder_t *encoder;
	const char *encoder_context_name;
	const char *encode_info_id;
} PtsStats;

EXPORT void init_pts_stats(PtsStats *stats, const obs_encoder_t *encoder,
			   const char *encoder_context_name, const char *encode_info_id);

EXPORT void record_pts_stats(PtsStats *stats, double frame_pts_ms, double pkt_pts_ms);

#ifdef __cplusplus
}
#endif
