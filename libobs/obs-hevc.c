/******************************************************************************
    Copyright (C) 2022 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "obs-hevc.h"
#include "obs-avc.h"

#include "obs.h"
#include "obs-nal.h"
#include "util/array-serializer.h"

enum {
	OBS_HEVC_NAL_TRAIL_N = 0,
	OBS_HEVC_NAL_TRAIL_R = 1,
	OBS_HEVC_NAL_TSA_N = 2,
	OBS_HEVC_NAL_TSA_R = 3,
	OBS_HEVC_NAL_STSA_N = 4,
	OBS_HEVC_NAL_STSA_R = 5,
	OBS_HEVC_NAL_RADL_N = 6,
	OBS_HEVC_NAL_RADL_R = 7,
	OBS_HEVC_NAL_RASL_N = 8,
	OBS_HEVC_NAL_RASL_R = 9,
	OBS_HEVC_NAL_VCL_N10 = 10,
	OBS_HEVC_NAL_VCL_R11 = 11,
	OBS_HEVC_NAL_VCL_N12 = 12,
	OBS_HEVC_NAL_VCL_R13 = 13,
	OBS_HEVC_NAL_VCL_N14 = 14,
	OBS_HEVC_NAL_VCL_R15 = 15,
	OBS_HEVC_NAL_BLA_W_LP = 16,
	OBS_HEVC_NAL_BLA_W_RADL = 17,
	OBS_HEVC_NAL_BLA_N_LP = 18,
	OBS_HEVC_NAL_IDR_W_RADL = 19,
	OBS_HEVC_NAL_IDR_N_LP = 20,
	OBS_HEVC_NAL_CRA_NUT = 21,
	OBS_HEVC_NAL_RSV_IRAP_VCL22 = 22,
	OBS_HEVC_NAL_RSV_IRAP_VCL23 = 23,
	OBS_HEVC_NAL_RSV_VCL24 = 24,
	OBS_HEVC_NAL_RSV_VCL25 = 25,
	OBS_HEVC_NAL_RSV_VCL26 = 26,
	OBS_HEVC_NAL_RSV_VCL27 = 27,
	OBS_HEVC_NAL_RSV_VCL28 = 28,
	OBS_HEVC_NAL_RSV_VCL29 = 29,
	OBS_HEVC_NAL_RSV_VCL30 = 30,
	OBS_HEVC_NAL_RSV_VCL31 = 31,
	OBS_HEVC_NAL_VPS = 32,
	OBS_HEVC_NAL_SPS = 33,
	OBS_HEVC_NAL_PPS = 34,
	OBS_HEVC_NAL_AUD = 35,
	OBS_HEVC_NAL_EOS_NUT = 36,
	OBS_HEVC_NAL_EOB_NUT = 37,
	OBS_HEVC_NAL_FD_NUT = 38,
	OBS_HEVC_NAL_SEI_PREFIX = 39,
	OBS_HEVC_NAL_SEI_SUFFIX = 40,
};

//PRISM/WuLongyue/20230726/None/support HEVC
struct HEVCDecoderConfigurationRecord {
	uint8_t configurationVersion;
	uint8_t general_profile_space;
	uint8_t general_tier_flag;
	uint8_t general_profile_idc;
	uint32_t general_profile_compatibility_flags;
	uint64_t general_constraint_indicator_flags;
	uint8_t general_level_idc;

	uint16_t min_spatial_segmentation_idc;
	uint8_t parallelismType;
	uint8_t chromaFormat;

	uint8_t bitDepthLumaMinus8;
	uint8_t bitDepthChromaMinus8;

	uint16_t avgFrameRate;
	uint8_t constantFrameRate;

	uint8_t numTemporalLayers;
	uint8_t temporalIdNested;
	uint8_t lengthSizeMinusOne;

	uint8_t numOfArrays;
};

bool obs_hevc_keyframe(const uint8_t *data, size_t size)
{
	const uint8_t *nal_start, *nal_end;
	const uint8_t *end = data + size;

	nal_start = obs_nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		const uint8_t type = (nal_start[0] & 0x7F) >> 1;

		if (type <= OBS_HEVC_NAL_RSV_IRAP_VCL23)
			return type >= OBS_HEVC_NAL_BLA_W_LP;

		nal_end = obs_nal_find_startcode(nal_start, end);
		nal_start = nal_end;
	}

	return false;
}

static int compute_hevc_keyframe_priority(const uint8_t *nal_start,
					  bool *is_keyframe, int priority)
{
	// HEVC contains NAL unit specifier at [6..1] bits of
	// the byte next to the startcode 0x000001
	const int type = (nal_start[0] & 0x7F) >> 1;

	// Mark IDR slices as key-frames and set them to highest
	// priority if needed. Assume other slices are non-key
	// frames and set their priority as high
	if (type >= OBS_HEVC_NAL_BLA_W_LP &&
	    type <= OBS_HEVC_NAL_RSV_IRAP_VCL23) {
		*is_keyframe = 1;
		priority = OBS_NAL_PRIORITY_HIGHEST;
	} else if (type >= OBS_HEVC_NAL_TRAIL_N &&
		   type <= OBS_HEVC_NAL_RASL_R) {
		if (priority < OBS_NAL_PRIORITY_HIGH)
			priority = OBS_NAL_PRIORITY_HIGH;
	}

	return priority;
}

static void serialize_hevc_data(struct serializer *s, const uint8_t *data,
				size_t size, bool *is_keyframe, int *priority)
{
	const uint8_t *const end = data + size;
	const uint8_t *nal_start = obs_nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		*priority = compute_hevc_keyframe_priority(
			nal_start, is_keyframe, *priority);

		const uint8_t *const nal_end =
			obs_nal_find_startcode(nal_start, end);
		const size_t nal_size = nal_end - nal_start;
		s_wb32(s, (uint32_t)nal_size);
		s_write(s, nal_start, nal_size);
		nal_start = nal_end;
	}
}

void obs_parse_hevc_packet(struct encoder_packet *hevc_packet,
			   const struct encoder_packet *src)
{
	struct array_output_data output;
	struct serializer s;
	long ref = 1;

	array_output_serializer_init(&s, &output);
	*hevc_packet = *src;

	serialize(&s, &ref, sizeof(ref));
	serialize_hevc_data(&s, src->data, src->size, &hevc_packet->keyframe,
			    &hevc_packet->priority);

	hevc_packet->data = output.bytes.array + sizeof(ref);
	hevc_packet->size = output.bytes.num - sizeof(ref);
	hevc_packet->drop_priority = hevc_packet->priority;
}

int obs_parse_hevc_packet_priority(const struct encoder_packet *packet)
{
	int priority = packet->priority;

	const uint8_t *const data = packet->data;
	const uint8_t *const end = data + packet->size;
	const uint8_t *nal_start = obs_nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		bool unused;
		priority = compute_hevc_keyframe_priority(nal_start, &unused,
							  priority);

		nal_start = obs_nal_find_startcode(nal_start, end);
	}

	return priority;
}

//PRISM/WuLongyue/20230726/None/support HEVC
static inline bool has_start_code(const uint8_t *data)
{
	if (data[0] != 0 || data[1] != 0)
		return false;

	return data[2] == 1 || (data[2] == 0 && data[3] == 1);
}

static void get_vps_sps_pps(const uint8_t *data, size_t size,
			    const uint8_t **vps, size_t *vps_size,
			    const uint8_t **sps, size_t *sps_size,
			    const uint8_t **pps, size_t *pps_size)
{
	const uint8_t *nal_start, *nal_end;
	const uint8_t *end = data + size;

	nal_start = obs_avc_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		nal_end = obs_avc_find_startcode(nal_start, end);

		short nalu_header = nal_start[0];
		int type = (nalu_header & 0x7F) >> 1;

		if (type == OBS_HEVC_NAL_VPS) {
			*vps = nal_start;
			*vps_size = nal_end - nal_start;
		} else if (type == OBS_HEVC_NAL_SPS) {
			*sps = nal_start;
			*sps_size = nal_end - nal_start;
		} else if (type == OBS_HEVC_NAL_PPS) {
			*pps = nal_start;
			*pps_size = nal_end - nal_start;
		}

		nal_start = nal_end;
	}
}

size_t obs_parse_hevc_header(uint8_t **header, const uint8_t *data, size_t size)
{
	struct array_output_data output;
	struct serializer s;

	uint8_t *vps = NULL, *sps = NULL, *pps = NULL;
	size_t vps_size = 0, sps_size = 0, pps_size = 0;

	struct HEVCDecoderConfigurationRecord mHvcc;
	memset(&mHvcc, 0, sizeof(mHvcc));
	mHvcc.configurationVersion = 1;
	mHvcc.numTemporalLayers = 1;
	mHvcc.lengthSizeMinusOne = 3;
	mHvcc.numOfArrays = 3;

	if (size <= 6) {
		assert(false);
		return 0;
	}

	if (!has_start_code(data)) {
		*header = bmemdup(data, size);
		return size;
	}

	get_vps_sps_pps(data, size, &vps, &vps_size, &sps, &sps_size, &pps,
			&pps_size);
	if (!vps || !sps || !pps || sps_size < 4) {
		assert(false);
		return 0;
	}

	array_output_serializer_init(&s, &output);

	// unsigned int(8) configurationVersion = 1;
	s_w8(&s, mHvcc.configurationVersion);

	// unsigned int(2) general_profile_space;
	// unsigned int(1) general_tier_flag;
	// unsigned int(5) general_profile_idc;
	s_w8(&s, mHvcc.general_profile_space << 6 |
			 mHvcc.general_tier_flag << 5 |
			 mHvcc.general_profile_idc);

	// unsigned int(32) general_profile_compatibility_flags;
	s_wb32(&s, mHvcc.general_profile_compatibility_flags);

	// unsigned int(48) general_constraint_indicator_flags;
	s_wb32(&s, (uint32_t)(mHvcc.general_constraint_indicator_flags >> 16));
	s_wb16(&s, (uint16_t)mHvcc.general_constraint_indicator_flags);

	// unsigned int(8) general_level_idc;
	s_w8(&s, mHvcc.general_level_idc);

	// bit(4) reserved = ‘1111’b;
	// unsigned int(12) min_spatial_segmentation_idc;
	s_wb16(&s, mHvcc.min_spatial_segmentation_idc | 0xf000);

	// bit(6) reserved = ‘111111’b;
	// unsigned int(2) parallelismType;
	s_w8(&s, mHvcc.parallelismType | 0xfc);

	// bit(6) reserved = ‘111111’b;
	// unsigned int(2) chromaFormat;
	s_w8(&s, mHvcc.chromaFormat | 0xfc);

	// bit(5) reserved = ‘11111’b;
	// unsigned int(3) bitDepthLumaMinus8;
	s_w8(&s, mHvcc.bitDepthLumaMinus8 | 0xf8);

	// bit(5) reserved = ‘11111’b;
	// unsigned int(3) bitDepthChromaMinus8;
	s_w8(&s, mHvcc.bitDepthChromaMinus8 | 0xf8);

	// bit(16) avgFrameRate;
	s_wb16(&s, mHvcc.avgFrameRate);

	// bit(2) constantFrameRate;
	// bit(3) numTemporalLayers;
	// bit(1) temporalIdNested;
	// unsigned int(2) lengthSizeMinusOne;
	s_w8(&s, mHvcc.constantFrameRate << 6 | mHvcc.numTemporalLayers << 3 |
			 mHvcc.temporalIdNested << 2 |
			 mHvcc.lengthSizeMinusOne);

	// unsigned int(8) numOfNalUnitType (VPS/SPS/PPS or VPS/SPS/PPS/SEI_PREFIX);
	s_w8(&s, mHvcc.numOfArrays);

	// VPS
	// bit(1) array_completeness = 0;
	// unsigned int(1) reserved = 0;
	// unsigned int(6) NAL_unit_type = 0x20;
	s_w8(&s, (0 << 7) | (0x20 & 0x3f));
	s_wb16(&s, 1);        // number of vps
	s_wb16(&s, (uint16_t)vps_size); // vps size
	s_write(&s, vps, vps_size);

	// SPS
	// bit(1) array_completeness = 0;
	// unsigned int(1) reserved = 0;
	// unsigned int(6) NAL_unit_type = 0x21;
	s_w8(&s, (0 << 7) | (0x21 & 0x3f));
	s_wb16(&s, 1);        // number of sps
	s_wb16(&s, (uint16_t)sps_size); // sps size
	s_write(&s, sps, sps_size);

	// PPS
	// bit(1) array_completeness = 0;
	// unsigned int(1) reserved = 0;
	// unsigned int(6) NAL_unit_type = 0x22;
	s_w8(&s, (0 << 7) | (0x22 & 0x3f));
	s_wb16(&s, 1);        // number of pps
	s_wb16(&s, (uint16_t)pps_size); // pps size
	s_write(&s, pps, pps_size);

	*header = output.bytes.array;
	return output.bytes.num;
}

void obs_extract_hevc_headers(const uint8_t *packet, size_t size,
			      uint8_t **new_packet_data,
			      size_t *new_packet_size, uint8_t **header_data,
			      size_t *header_size, uint8_t **sei_data,
			      size_t *sei_size)
{
	DARRAY(uint8_t) new_packet;
	DARRAY(uint8_t) header;
	DARRAY(uint8_t) sei;
	const uint8_t *nal_start, *nal_end, *nal_codestart;
	const uint8_t *end = packet + size;

	da_init(new_packet);
	da_init(header);
	da_init(sei);

	nal_start = obs_nal_find_startcode(packet, end);
	nal_end = NULL;
	while (nal_end != end) {
		nal_codestart = nal_start;

		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		const uint8_t type = (nal_start[0] & 0x7F) >> 1;

		nal_end = obs_nal_find_startcode(nal_start, end);
		if (!nal_end)
			nal_end = end;

		if (type == OBS_HEVC_NAL_VPS || type == OBS_HEVC_NAL_SPS ||
		    type == OBS_HEVC_NAL_PPS) {
			da_push_back_array(header, nal_codestart,
					   nal_end - nal_codestart);
		} else if (type == OBS_HEVC_NAL_SEI_PREFIX ||
			   type == OBS_HEVC_NAL_SEI_SUFFIX) {
			da_push_back_array(sei, nal_codestart,
					   nal_end - nal_codestart);

		} else {
			da_push_back_array(new_packet, nal_codestart,
					   nal_end - nal_codestart);
		}

		nal_start = nal_end;
	}

	*new_packet_data = new_packet.array;
	*new_packet_size = new_packet.num;
	*header_data = header.array;
	*header_size = header.num;
	*sei_data = sei.array;
	*sei_size = sei.num;
}
