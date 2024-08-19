#include <util/darray.h>
#include <util/crc32.h>
#include "find-font.h"
#include "text-freetype2.h"

#import <Foundation/Foundation.h>
// PRISM/cao.kewei/20240426/#5231/font path
#import <AppKit/AppKit.h>

extern void save_font_list(void);

static inline void add_path_font(const char *path)
{
    FT_Face face;
    FT_Long idx = 0;
    FT_Long max_faces = 1;

    while (idx < max_faces) {
        if (FT_New_Face(ft2_lib, path, idx, &face) != 0)
            break;

        build_font_path_info(face, idx++, path);
        max_faces = face->num_faces;
        FT_Done_Face(face);
    }
}

static void add_path_fonts(NSFileManager *file_manager, NSString *path)
{
    NSArray *files = NULL;

    files = [file_manager contentsOfDirectoryAtPath:path error:nil];

    for (NSString *file in files) {
        NSString *full_path = [path stringByAppendingPathComponent:file];

        BOOL is_dir = FALSE;
        bool folder_exists = [file_manager fileExistsAtPath:full_path isDirectory:&is_dir];

        if (folder_exists && is_dir) {
            add_path_fonts(file_manager, full_path);
        } else {
            add_path_font(full_path.fileSystemRepresentation);
        }
    }
}

// PRISM/cao.kewei/20240426/#5231/font path
NSString *pathForFontNamed(NSString *fontName) {
	// Create a font descriptor with the provided name
	CTFontDescriptorRef fontDescriptor = CTFontDescriptorCreateWithNameAndSize((CFStringRef)fontName, 0.0f);
	if (!fontDescriptor) {
		return nil;
	}
	
	// Extract the font URL attribute (if it exists)
	CFURLRef fontURL = (CFURLRef)CTFontDescriptorCopyAttribute(fontDescriptor, kCTFontURLAttribute);
	
	// Release the font descriptor
	CFRelease(fontDescriptor);
	
	// Check if URL exists and convert to NSString
	if (fontURL) {
		NSString *path = [(__bridge_transfer NSURL *)fontURL path];
		return path;
	} else {
		return nil;
	}
}

void load_os_font_list(void)
{
    @autoreleasepool {
      //PRISM/cao.kewei/20240426/#5231/font path
      __block BOOL isDir;

      NSFontManager *fontManager = [NSFontManager sharedFontManager];
      [fontManager.availableFonts enumerateObjectsUsingBlock:^(NSString * _Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
        UNUSED_PARAMETER(idx);
        UNUSED_PARAMETER(stop);

        NSString *fontPath = pathForFontNamed(obj);
        if (fontPath) {
          BOOL fileExists = [[NSFileManager defaultManager] fileExistsAtPath:fontPath isDirectory:&isDir];

          if (fileExists && !isDir) {
            add_path_font(fontPath.UTF8String);
          }
        }
      }];

      save_font_list();
    }
}

static uint32_t add_font_checksum(uint32_t checksum, const char *path)
{
    if (path && *path)
        checksum = calc_crc32(checksum, path, strlen(path));
    return checksum;
}

static uint32_t add_font_checksum_path(uint32_t checksum, NSFileManager *file_manager, NSString *path)
{
    NSArray *files = NULL;

    files = [file_manager contentsOfDirectoryAtPath:path error:nil];

    for (NSString *file in files) {
        NSString *full_path = [path stringByAppendingPathComponent:file];

        checksum = add_font_checksum(checksum, full_path.fileSystemRepresentation);
    }

    return checksum;
}

uint32_t get_font_checksum(void)
{
    __block uint32_t checksum = 0;

    @autoreleasepool {
      //PRISM/cao.kewei/20240426/#5231/font path
      __block BOOL isDir;

      NSFileManager *fileManager = [NSFileManager defaultManager];
      NSFontManager *fontManager = [NSFontManager sharedFontManager];
      [fontManager.availableFonts enumerateObjectsUsingBlock:^(NSString * _Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
        UNUSED_PARAMETER(idx);
        UNUSED_PARAMETER(stop);

        NSString *fontPath = pathForFontNamed(obj);
        if (fontPath) {
          BOOL fileExists = [fileManager fileExistsAtPath:fontPath isDirectory:&isDir];

          if (fileExists && !isDir) {
			  checksum = add_font_checksum(checksum, fontPath.UTF8String);
          }
        }
      }];
    }

    return checksum;
}

//PRISM/cao.kewei/20240524/custom font path
void load_custom_font(const char *path) {
	if (path == NULL) {
		return;
	}
	add_path_font(path);
}
