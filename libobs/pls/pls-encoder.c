#include "obs-internal.h"
#include "pls-encoder.h"

//PRISM/cao.kewei/20240514/#5378/force keyframe
bool obs_encoder_request_keyframe(obs_encoder_t *encoder)
{
	return os_atomic_load_bool(&encoder->force_keyframe);
}

bool obs_encoder_request_keyframe_and_clean(obs_encoder_t *encoder)
{
	if (encoder == NULL) {
		return false;
	}
	return os_atomic_set_bool(&encoder->force_keyframe, false);
}

void obs_encoder_set_request_keyframe(obs_encoder_t *encoder, bool keyframe)
{
	os_atomic_set_bool(&encoder->force_keyframe, keyframe);
}

void obs_encoder_on_encoded_frame(obs_encoder_t *encoder, bool keyframe)
{
	if (keyframe || OBS_ENCODER_AUDIO == obs_encoder_get_type(encoder)) {
		// we send logs for the first 5 key-frames
		long new_value = os_atomic_inc_long(&encoder->keyframe_cnt);
		if (new_value <= 5) {
			const char *id = obs_encoder_get_id(encoder);
			blog(LOG_INFO,
			     "encoded keyframe %ld > encoder=%p id=%s",
			     new_value, encoder, id ? id : "");
		}
	}
}
