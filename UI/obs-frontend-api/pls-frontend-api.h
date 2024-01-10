#pragma once

#include <obs.h>
#include <util/darray.h>

//PRISM/ChengBing/20230425/#porting from PRISM 3.1.3

/******************************************************************************/
// OBS Modification:
// wu.longyue@navercorp.com / 20200228 / new feature
// Reason: The host and javascript need communicate eachother
// Solution: override this function to receive message
typedef const char *(*TPrsimInvokedByWeb)(const char *data);
typedef void (*TPrsimDispatchJSEvent)(const char *type, const char *detail);
//PRISM/Zhangdewen/20200901/#/chat source
typedef void (*TPrsimDispatchJSEventToSource)(obs_source_t *source,
					      const char *type,
					      const char *detail);

#ifdef _WIN32
#ifdef OBS_FRONTEND_API_EXPORTS
#define OBS_FRONTEND_API __declspec(dllexport)
#else
#define OBS_FRONTEND_API __declspec(dllimport)
#endif
#else
#define OBS_FRONTEND_API
#endif

//PRISM/Zhangdewen/20230508/#754/modify api, fix mac nullptr error
OBS_FRONTEND_API
TPrsimInvokedByWeb pls_frontend_get_web_invoked_cb();
OBS_FRONTEND_API void
pls_frontend_set_web_invoked_cb(TPrsimInvokedByWeb web_invoked);
OBS_FRONTEND_API const char *pls_frontend_call_web_invoked_cb(const char *data);

//PRISM/Zhangdewen/20230508/#754/modify api, fix mac nullptr error
OBS_FRONTEND_API TPrsimDispatchJSEvent pls_frontend_get_dispatch_js_event_cb();
OBS_FRONTEND_API void
pls_frontend_set_dispatch_js_event_cb(TPrsimDispatchJSEvent dispatch_js_event);
OBS_FRONTEND_API void
pls_frontend_call_dispatch_js_event_cb(const char *type, const char *detail);

//PRISM/Zhangdewen/20230508/#754/modify api, fix mac nullptr error
OBS_FRONTEND_API TPrsimDispatchJSEventToSource
pls_frontend_get_dispatch_js_event_to_source_cb();
OBS_FRONTEND_API void pls_frontend_set_dispatch_js_event_to_source_cb(
	TPrsimDispatchJSEventToSource dispatch_js_event_to_source);
OBS_FRONTEND_API void pls_frontend_call_dispatch_js_event_to_source_cb(
	obs_source_t *source, const char *type, const char *detail);

/******************************************************************************/
