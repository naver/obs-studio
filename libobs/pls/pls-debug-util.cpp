#include "pls-base.h"
#include "obs.h"
#include <string>
#ifdef _WIN32

#pragma pack(push, 1)
struct bitmap_file_header {
	unsigned short bfType;
	unsigned int bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int bfOffBits;
};

struct bitmap_info_header {
	unsigned int biSize;
	int biWidth;
	int biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int biCompression;
	unsigned int biSizeImage;
	int biXPelsPerMeter;
	int biYPelsPerMeter;
	unsigned int biClrUsed;
	unsigned int biClrImportant;
};
#pragma pack(pop)

static bool save_bitmap_file(wchar_t const *path, const uint8_t *data, int linesize, int width, int height,
			     int per_pixel_byte, bool flip)
{
	if (!path || !data)
		return false;

	FILE *fp = NULL;
	auto err = _wfopen_s(&fp, path, L"wb+");
	if (err != 0 || !fp)
		return false;

	unsigned dest_stride = width * per_pixel_byte;

	struct bitmap_file_header file_head;
	memset(&file_head, 0, sizeof(file_head));
	file_head.bfType = 'MB';
	file_head.bfOffBits = sizeof(struct bitmap_file_header) + sizeof(struct bitmap_info_header);
	file_head.bfSize = file_head.bfOffBits + height * dest_stride;

	struct bitmap_info_header info_head;
	memset(&info_head, 0, sizeof(info_head));
	info_head.biSize = sizeof(info_head);
	info_head.biWidth = width;
	info_head.biHeight = height;
	info_head.biPlanes = 1;
	info_head.biBitCount = per_pixel_byte * 8;
	info_head.biCompression = 0;
	info_head.biSizeImage = height * dest_stride;

	fwrite(&file_head, 1, sizeof(struct bitmap_file_header), fp);
	fwrite(&info_head, 1, sizeof(struct bitmap_info_header), fp);
	for (int i = 0; i < height; ++i) {
		if (flip)
			fwrite(data + (height - 1 - i) * linesize, 1, dest_stride, fp);
		else
			fwrite(data + i * linesize, 1, dest_stride, fp);
	}

	fclose(fp);
	return true;
}

void pls_dump_texture(void *tex, wchar_t const *path)
{
#ifdef DEBUG
	if (!tex || !path)
		return;

	gs_texture_t *texture = (gs_texture_t *)tex;

	obs_enter_graphics();

	enum gs_color_format fmt = gs_texture_get_color_format(texture);
	if (fmt == GS_BGRX || fmt == GS_BGRA || fmt == GS_BGRX_UNORM || fmt == GS_BGRA_UNORM) {
		uint32_t cx = gs_texture_get_width(texture);
		uint32_t cy = gs_texture_get_height(texture);

		gs_stagesurf_t *stage = gs_stagesurface_create(cx, cy, fmt);
		if (stage) {
			gs_stage_texture(stage, texture);

			uint8_t *data = NULL;
			uint32_t linesize = 0;
			if (gs_stagesurface_map(stage, &data, &linesize)) {
				save_bitmap_file(path, data, linesize, cx, cy, 4, true);
				gs_stagesurface_unmap(stage);
			}

			gs_stagesurface_destroy(stage);
		}
	} else {
		assert(false);
	}

	obs_leave_graphics();
#endif // DEBUG
}

#endif //--------------- _WIN32 ------------------------------------------------------------
