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

#ifdef __cplusplus
}
#endif
