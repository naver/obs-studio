#ifdef __APPLE__

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <CoreGraphics/CGWindow.h>
#include <stdio.h>
#include <notify.h>
#include <libproc.h>

// Event type enum matching pls-lens-event.cpp
enum PLSLensEventType {
	PLS_LENS_EVENT_ACTIVE = 0,
	PLS_LENS_EVENT_LICENSE,
	PLS_LENS_EVENT_VB,
	PLS_LENS_EVENT_CAPTURE,
	PLS_LENS_EVENT_ONOFF,
	PLS_LENS_EVENT_UI_CONTROLLABLE,
	PLS_LENS_EVENT_COUNT
};

typedef void (*pls_lens_event_cb_t)(int lens_index, int event_type, uint64_t state, void *context);

static const char *event_channel_names[PLS_LENS_EVENT_COUNT] = {
	"active",
	"license",
	"vb",
	"capture",
	"lens_onoff",
	"ui_controllable"
};

// Register all notify channels for a lens index
// tokens_out must have space for PLS_LENS_EVENT_COUNT tokens
void pls_register_all_lens_notify(int lens_index, pls_lens_event_cb_t cb, void *context, int *tokens_out)
{
	if (!cb || !tokens_out)
		return;

	for (int event_type = 0; event_type < PLS_LENS_EVENT_COUNT; ++event_type) {
		char name[128];
		snprintf(name, sizeof(name), "com.prism.ipc.%d.%s", lens_index, event_channel_names[event_type]);

		int token = 0;
		uint32_t status = notify_register_dispatch(
			name, &token,
			dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
			^(int t) {
				uint64_t state = 0;
				if (notify_get_state(t, &state) != NOTIFY_STATUS_OK)
					state = 0;
				cb(lens_index, event_type, state, context);
			});

		tokens_out[event_type] = (status == NOTIFY_STATUS_OK && token > 0) ? token : 0;
	}
}

// Unregister all notify channels
void pls_unregister_all_lens_notify(int *tokens, int count)
{
	if (!tokens)
		return;

	for (int i = 0; i < count; ++i) {
		if (tokens[i] > 0) {
			notify_cancel(tokens[i]);
			tokens[i] = 0;
		}
	}
}

// Read current state from a token
bool pls_get_lens_state_from_token(int token, uint64_t *stateOut)
{
	if (token <= 0 || !stateOut)
		return false;

	uint64_t state = 0;
	if (notify_get_state(token, &state) != NOTIFY_STATUS_OK)
		return false;

	*stateOut = state;
	return true;
}

// Check if a process is running by its executable name using libproc
static bool pls_is_process_running(const char *process_name)
{
	int pid_count = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
	if (pid_count <= 0)
		return false;

	pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * pid_count);
	if (!pids)
		return false;

	pid_count = proc_listpids(PROC_ALL_PIDS, 0, pids, sizeof(pid_t) * pid_count);

	bool found = false;
	for (int i = 0; i < pid_count && !found; i++) {
		if (pids[i] == 0)
			continue;

		char path[PROC_PIDPATHINFO_MAXSIZE];
		if (proc_pidpath(pids[i], path, sizeof(path)) > 0) {
			// Check if the executable name matches
			const char *exe_name = strrchr(path, '/');
			if (exe_name && strcmp(exe_name + 1, process_name) == 0) {
				found = true;
			}
		}
	}

	free(pids);
	return found;
}

bool pls_is_lens_app_running(void)
{
    return pls_is_process_running("PRISMLensCore");
}

// Check if PRISMLens is installed by looking up its bundle identifier
bool pls_is_lens_installed(void)
{
	@autoreleasepool {
		NSArray<NSURL *> *appURLs = [[NSWorkspace sharedWorkspace] URLsForApplicationsWithBundleIdentifier:@"com.prismlive.camstudio"];
		for (NSURL *url in appURLs) {
			if ([url.path hasPrefix:@"/Applications"]) {
				return true;
			}
		}
		return false;
	}
}

#endif

bool pls_is_lens_state_supported()
{
    @autoreleasepool {
        // Find the Lens app under /Applications via bundle identifier
        NSArray<NSURL *> *appURLs = [[NSWorkspace sharedWorkspace]
            URLsForApplicationsWithBundleIdentifier:@"com.prismlive.camstudio"];

        NSURL *appURL = nil;
        for (NSURL *url in appURLs) {
            if ([url.path hasPrefix:@"/Applications"]) {
                appURL = url;
                break;
            }
        }
        if (!appURL) {
            return false;
        }

        // Read Info.plist directly from disk to bypass NSBundle's cache.
        // NSBundle caches infoDictionary and returns stale version info
        // after an over-install where the bundle is replaced in-place.
        NSString *infoPlistPath = [appURL.path stringByAppendingPathComponent:@"Contents/Info.plist"];
        NSDictionary *infoDict = [NSDictionary dictionaryWithContentsOfFile:infoPlistPath];
        if (!infoDict) {
            return false;
        }

        NSString *version = [infoDict objectForKey:@"CFBundleShortVersionString"];
        if (!version) {
            return false;
        }

        NSArray *components = [version componentsSeparatedByString:@"."];
        if (components.count < 3) {
            return false;
        }

        int major = [components[0] intValue];
        int minor = [components[1] intValue];
        int build = [components[2] intValue];

        if (major > 2 || (major == 2 && minor > 0) || (major == 2 && minor == 0 && build > 2)) {
            return true;
        }

        return false;
    }
}
