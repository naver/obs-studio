#pragma once
#include "obs.h"
#include "pls/pls-lens-info.h"
#include <vector>
#include <Windows.h>

#ifdef OBS_LEGACY
#include "libdshowcapture/dshowcapture.hpp"
#else
#include <dshowcapture.hpp>
#endif

using namespace std;
using namespace DShow;

// Insert virtual devices of LENS. Since lens vcam is protected from loading, So we have to insert virtual devices.
void insert_lens_video_devices(vector<VideoDevice> &devices);
void insert_lens_audio_devices(vector<AudioDevice> &devices);

// The user switched devices in the combox, check and disable options that the lens does not support.
void on_selected_device_changed(string video_device_id, obs_properties_t *props);

// Compatible with old settings and convert them to new settings.
void adapt_lens_settings(obs_data_t *settings);

// Stop capturing in lens v2
void reset_lens_source(obs_source_t *lens_v2);

// DShow is using lens audio and does not use lens video, we must ensure timestamp of lens audio is started from 0 as old code
void force_new_timestamp(obs_source_t *lens_v2);

// Check if need to use lens v2 and update its settings if need
void check_use_lens(obs_source_t *dshow_source, obs_source_t *&lens_v2, obs_data_t *settings, bool &use_lens_video,
		    int &video_idx, bool &use_lens_audio);

//Check settings's video device is lens
bool use_lens_video(obs_data_t *settings);
