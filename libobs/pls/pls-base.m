#ifdef __APPLE__

#include "pls-base.h"
#import <Cocoa/Cocoa.h>

//PRISM/Zhongling/20230816/#2251/crash on `gl_update`
void os_async_on_main_queue(void *context, void (*callback)(void *context))
{
	dispatch_async(dispatch_get_main_queue(), ^() {
		if (callback) {
			callback(context);
		}
	});
}

#endif

