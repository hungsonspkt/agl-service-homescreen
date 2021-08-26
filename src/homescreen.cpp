/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "homescreen.h"

const char _error[] = "error";
const char _application_id[] = "application_id";
const char _display_message[] = "display_message";
const char _reply_message[] = "reply_message";
const char _keyData[] = "data";
const char _keyId[] = "id";

/**
 * init function
 *
 * #### Parameters
 * - api : the api serving the request
 *
 * #### Return
 * 0 : init success
 * 1 : init fail
 *
 */
int hs_instance::init(afb_api_t api)
{
    if(client_manager == nullptr) {
        AFB_ERROR("client_manager is nullptr.");
        return -1;
    }
    client_manager->init();

    if(app_info == nullptr) {
        AFB_ERROR("app_info is nullptr.");
        return -1;
    }
    app_info->init(api);

    return 0;
}

/**
 * set event hook
 *
 * #### Parameters
 *  - event  : event name
 *  - f : hook function
 *
 * #### Return
 * Nothing
 */
void hs_instance::setEventHook(const char *event, const event_hook_func f)
{
    if(event == nullptr || f == nullptr) {
        AFB_WARNING("argument is null.");
        return;
    }

    std::string ev(event);
    auto it = event_hook_list.find(ev);
    if(it != event_hook_list.end()) {
        it->second.push_back(f);
    }
    else {
        std::list<event_hook_func> l;
        l.push_back(f);
        event_hook_list[ev] = std::move(l);
    }
}

/**
 * onEvent function
 *
 * #### Parameters
 *  - api : the api serving the request
 *  - event  : event name
 *  - object : event json object
 *
 * #### Return
 * Nothing
 */
void hs_instance::onEvent(afb_api_t api, const char *event, struct json_object *object)
{
    std::string ev(event);
    auto it = event_hook_list.find(ev);
    if(it != event_hook_list.end()) {
        for(auto &ref : it->second) {
            if(ref(api, event, object))
                break;
        }
    }
}

static struct hs_instance *g_hs_instance;

/**
 * set event hook
 *
 * #### Parameters
 *  - event  : event name
 *  - f : hook function pointer
 *
 * #### Return
 * Nothing
 */
void setEventHook(const char *event, const event_hook_func f)
{
    if(g_hs_instance == nullptr) {
        AFB_ERROR("g_hs_instance is null.");
        return;
    }

    g_hs_instance->setEventHook(event, f);
}

/*
********** Method of HomeScreen Service (API) **********
*/

static void pingSample(afb_req_t request)
{
   static int pingcount = 0;
   afb_req_success_f(request, json_object_new_int(pingcount), "Ping count = %d", pingcount);
   AFB_DEBUG("Verbosity macro at level notice invoked at ping invocation count = %d", pingcount);
   pingcount++;
}

/**
 * tap_shortcut notify for homescreen
 * When Shortcut area is tapped,  notify these applciations
 *
 * #### Parameters
 * Request key
 * - application_id   : application id
 *
 * #### Return
 * None
 *
 */
static void tap_shortcut (afb_req_t request)
{
    int ret = 0;
    const char* value = afb_req_value(request, _application_id);
    if (value) {
        AFB_INFO("request appid = %s.", value);
        ret = g_hs_instance->client_manager->handleRequest(request, __FUNCTION__, value);
        if(ret == AFB_REQ_NOT_STARTED_APPLICATION) {
            std::string id = g_hs_instance->app_info->getAppProperty(value, _keyId);
	    if (!id.empty()) {
		    HS_AfmMainProxy afm_proxy;
		    afm_proxy.start(g_hs_instance, request, id);
		    ret = 0;
	    } else {
		    ret = AFB_EVENT_BAD_REQUEST;
	    }
        }
    }
    else {
        ret = AFB_EVENT_BAD_REQUEST;
    }

    if (ret) {
        afb_req_fail_f(request, "failed", "called %s, Unknown parameter", __FUNCTION__);
    }
    else {
        struct json_object *res = json_object_new_object();
        hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
          _error,  ret);
        afb_req_success(request, res, "afb_event_push event [tap_shortcut]");
    }
}

/**
 * HomeScreen OnScreen message
 *
 * #### Parameters
 * Request key
 * - display_message   : message for display
 *
 * #### Return
 * None
 *
 */
static void on_screen_message (afb_req_t request)
{
    int ret = g_hs_instance->client_manager->handleRequest(request, __FUNCTION__);
    if (ret) {
        afb_req_fail_f(request, "failed", "called %s, Unknown parameter", __FUNCTION__);
    }
    else {
        struct json_object *res = json_object_new_object();
        hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
          _error,  ret);
        afb_req_success(request, res, "afb_event_push event [on_screen_message]");
    }
}

/**
 * HomeScreen OnScreen Reply
 *
 * #### Parameters
 * Request key
 * - reply_message   : message for reply
 *
 * #### Return
 * None
 *
 */
static void on_screen_reply (afb_req_t request)
{
    int ret = g_hs_instance->client_manager->handleRequest(request, __FUNCTION__);
    if (ret) {
        afb_req_fail_f(request, "failed", "called %s, Unknown parameter", __FUNCTION__);
    }
    else {
        struct json_object *res = json_object_new_object();
        hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
          _error,  ret);
        afb_req_success(request, res, "afb_event_push event [on_screen_reply]");
    }
}

/**
 * Subscribe event
 *
 * #### Parameters
 *  - event  : Event name. Event list is written in libhomescreen.cpp
 *
 * #### Return
 * None
 *
 */
static void subscribe(afb_req_t request)
{
    int ret = 0;
    std::string req_appid = std::move(get_application_id(request));
    if(!req_appid.empty()) {
        ret = g_hs_instance->client_manager->handleRequest(request, __FUNCTION__, req_appid.c_str());
    }
    else {
        ret = AFB_EVENT_BAD_REQUEST;
    }

    if(ret) {
        afb_req_fail_f(request, "afb_req_subscribe failed", "called %s.", __FUNCTION__);
    }
    else {
        struct json_object *res = json_object_new_object();
        hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
            _error, ret);
        afb_req_success_f(request, res, "homescreen binder subscribe.");
    }
}

/**
 * Unsubscribe event
 *
 * #### Parameters
 *  - event  : Event name. Event list is written in libhomescreen.cpp
 *
 * #### Return
 * None
 *
 */
static void unsubscribe(afb_req_t request)
{
    int ret = 0;
    std::string req_appid = std::move(get_application_id(request));
    if(!req_appid.empty()) {
        ret = g_hs_instance->client_manager->handleRequest(request, __FUNCTION__, req_appid.c_str());
    }
    else {
        ret = AFB_EVENT_BAD_REQUEST;
    }

    if(ret) {
        afb_req_fail_f(request, "afb_req_unsubscribe failed", "called %s.", __FUNCTION__);
    }
    else {
        struct json_object *res = json_object_new_object();
        hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
            _error, ret);
        afb_req_success_f(request, res, "homescreen binder unsubscribe success.");
    }
}

/**
 * showWindow event
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * None
 *
 */
static void showWindow(afb_req_t request)
{
    int ret = 0;
    const char* value = afb_req_value(request, _application_id);
    if (value) {
        ret = g_hs_instance->client_manager->handleRequest(request, __FUNCTION__, value);
        if(ret == AFB_REQ_NOT_STARTED_APPLICATION) {
            std::string id = g_hs_instance->app_info->getAppProperty(value, _keyId);
	    if (!id.empty()) {
		    HS_AfmMainProxy afm_proxy;
		    afm_proxy.start(g_hs_instance, request, id);
		    ret = 0;
	    } else {
		    ret = AFB_EVENT_BAD_REQUEST;
	    }
        }
    }
    else {
        ret = AFB_EVENT_BAD_REQUEST;
    }

    if (ret) {
        afb_req_fail_f(request, "failed", "called %s, Unknown parameter", __FUNCTION__);
    }
    else {
        struct json_object *res = json_object_new_object();
        hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
          _error,  ret);
        afb_req_success(request, res, "afb_event_push event [showWindow]");
    }
}

/**
 * hideWindow event
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * None
 *
 */
static void hideWindow(afb_req_t request)
{
    int ret = 0;
    const char* value = afb_req_value(request, _application_id);
    if (value) {
        ret = g_hs_instance->client_manager->handleRequest(request, __FUNCTION__, value);
    }
    else {
        ret = AFB_EVENT_BAD_REQUEST;
    }

    if (ret) {
        afb_req_fail_f(request, "failed", "called %s, Unknown parameter", __FUNCTION__);
    }
    else {
        struct json_object *res = json_object_new_object();
        hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
          _error,  ret);
        afb_req_success(request, res, "afb_event_push event [hideWindow]");
    }
}

/**
 * replyShowWindow event
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 *  None
 *
 */
static void replyShowWindow(afb_req_t request)
{
    int ret = 0;
    const char* value = afb_req_value(request, _application_id);
    if (value) {
        ret = g_hs_instance->client_manager->handleRequest(request, __FUNCTION__, value);
    }
    else {
        ret = AFB_EVENT_BAD_REQUEST;
    }

    if (ret) {
        afb_req_fail_f(request, "failed", "called %s, Unknown parameter", __FUNCTION__);
    }
    else {
        struct json_object *res = json_object_new_object();
        hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
          _error,  ret);
        afb_req_success(request, res, "afb_event_push event [replyShowWindow]");
    }
}

/**
 * showNotification event
 *
 * the contents to homescreen which display at top area.
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * None
 *
 */
static void showNotification(afb_req_t request)
{
    int ret = g_hs_instance->client_manager->handleRequest(request, __FUNCTION__, "homescreen");
    if (ret) {
        afb_req_fail_f(request, "failed", "called %s, Unknown parameter", __FUNCTION__);
    }
    else {
        struct json_object *res = json_object_new_object();
        hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
          _error,  ret);
        afb_req_success(request, res, "afb_event_push event [showNotification]");
    }
}

/**
 * showInformation event
 *
 * the contents to homescreen which display at bottom area.
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * None
 *
 */
static void showInformation(afb_req_t request)
{
    int ret = g_hs_instance->client_manager->handleRequest(request,  __FUNCTION__, "homescreen");
    if (ret) {
        afb_req_fail_f(request, "failed", "called %s, Unknown parameter", __FUNCTION__);
    }
    else {
        struct json_object *res = json_object_new_object();
        hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
          _error,  ret);
        afb_req_success(request, res, "afb_event_push event [showInformation]");
    }
}

/**
 * get runnables list
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * None
 *
 */
static void getRunnables(afb_req_t request)
{
    struct json_object* j_runnable = json_object_new_array();
    g_hs_instance->app_info->getRunnables(&j_runnable);

    /*create response json object*/
    struct json_object *res = json_object_new_object();
    hs_add_object_to_json_object_func(res, __FUNCTION__, 2, _error, 0);
    json_object_object_add(res, _keyData, j_runnable);
    afb_req_success_f(request, res, "homescreen binder unsubscribe success.");
}

/*
 * array of the verbs exported to afb-daemon
 */
static const afb_verb_t verbs[]= {
    /* VERB'S NAME                 FUNCTION TO CALL                  */
    { .verb="ping",              .callback=pingSample             },
    { .verb="tap_shortcut",      .callback=tap_shortcut           },
    { .verb="showWindow",        .callback=showWindow             },
    { .verb="hideWindow",        .callback=hideWindow             },
    { .verb="replyShowWindow",   .callback=replyShowWindow        },
    { .verb="on_screen_message", .callback=on_screen_message      },
    { .verb="on_screen_reply",   .callback=on_screen_reply        },
    { .verb="subscribe",         .callback=subscribe              },
    { .verb="unsubscribe",       .callback=unsubscribe            },
    { .verb="showNotification",  .callback=showNotification       },
    { .verb="showInformation",   .callback=showInformation        },
    { .verb="getRunnables",      .callback=getRunnables           },
    {NULL } /* marker for end of the array */
};

/**
 * homescreen binding preinit function
 *
 * #### Parameters
 *  - api : the api serving the request
 *
 * #### Return
 * None
 *
 */
static int preinit(afb_api_t api)
{
   (void)  api;
   AFB_DEBUG("binding preinit (was register)");
   return 0;
}

/**
 * homescreen binding init function
 *
 * #### Parameters
 *  - api : the api serving the request
 *
 * #### Return
 * None
 *
 */
static int init(afb_api_t api)
{
    AFB_DEBUG("binding init");

    if(g_hs_instance != nullptr) {
        AFB_WARNING( "g_hs_instance isn't null.");
        delete g_hs_instance->client_manager;
        delete g_hs_instance->app_info;
        delete g_hs_instance;
        g_hs_instance = nullptr;
    }
    g_hs_instance = new hs_instance();
    if(g_hs_instance == nullptr) {
        return -1;
    }

    return g_hs_instance->init(api);
}

/**
 * homescreen binding event function
 *
 * #### Parameters
 *  - api : the api serving the request
 *  - event  : event name
 *  - object : event json object
 *
 * #### Return
 * None
 *
 */
static void onevent(afb_api_t api, const char *event, struct json_object *object)
{
    AFB_INFO("on_event %s", event);
    g_hs_instance->onEvent(api, event, object);
}

const afb_binding_t afbBindingExport = {
    .api = "homescreen",
    .specification = NULL,
    .info = NULL,
    .verbs = verbs,
    .preinit = preinit,
    .init = init,
    .onevent = onevent
};
