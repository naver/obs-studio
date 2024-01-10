#include "pls-frontend-api.h"

//PRISM/ChengBing/20230425/#porting from PRISM 3.1.3
static TPrsimInvokedByWeb prism_frontend_web_invoked = nullptr;
static TPrsimDispatchJSEvent prism_frontend_dispatch_js_event = nullptr;
//PRISM/Zhangdewen/20200901/#/chat source
static TPrsimDispatchJSEventToSource prism_frontend_dispatch_js_event_to_source =
	nullptr;

//PRISM/Zhangdewen/20230508/#754/modify api, fix mac nullptr error
OBS_FRONTEND_API
TPrsimInvokedByWeb pls_frontend_get_web_invoked_cb()
{
	return prism_frontend_web_invoked;
}
OBS_FRONTEND_API void
pls_frontend_set_web_invoked_cb(TPrsimInvokedByWeb web_invoked)
{
	prism_frontend_web_invoked = web_invoked;
}
OBS_FRONTEND_API const char *pls_frontend_call_web_invoked_cb(const char *data)
{
	if (prism_frontend_web_invoked)
		return prism_frontend_web_invoked(data);
	return "{}";
}

//PRISM/Zhangdewen/20230508/#754/modify api, fix mac nullptr error
OBS_FRONTEND_API TPrsimDispatchJSEvent pls_frontend_get_dispatch_js_event_cb()
{
	return prism_frontend_dispatch_js_event;
}
OBS_FRONTEND_API void
pls_frontend_set_dispatch_js_event_cb(TPrsimDispatchJSEvent dispatch_js_event)
{
	prism_frontend_dispatch_js_event = dispatch_js_event;
}
OBS_FRONTEND_API void pls_frontend_call_dispatch_js_event_cb(const char *type,
							     const char *detail)
{
	if (prism_frontend_dispatch_js_event)
		prism_frontend_dispatch_js_event(type, detail);
}

//PRISM/Zhangdewen/20230508/#754/modify api, fix mac nullptr error
OBS_FRONTEND_API TPrsimDispatchJSEventToSource
pls_frontend_get_dispatch_js_event_to_source_cb()
{
	return prism_frontend_dispatch_js_event_to_source;
}
OBS_FRONTEND_API void pls_frontend_set_dispatch_js_event_to_source_cb(
	TPrsimDispatchJSEventToSource dispatch_js_event_to_source)
{
	prism_frontend_dispatch_js_event_to_source =
		dispatch_js_event_to_source;
}
OBS_FRONTEND_API void pls_frontend_call_dispatch_js_event_to_source_cb(
	obs_source_t *source, const char *type, const char *detail)
{
	if (prism_frontend_dispatch_js_event_to_source)
		prism_frontend_dispatch_js_event_to_source(source, type,
							   detail);
}
