/*
 * Copyright (c) 2018 TOYOTA MOTOR CORPORATION
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

#include <cstring>
#include "hs-client.h"
#include "hs-helper.h"

static const char _event[] = "event";
static const char _type[] = "type";
static const char _text[] = "text";
static const char _info[] = "info";
static const char _icon[] = "icon";
static const char _parameter[] = "parameter";
static const char _replyto[] = "replyto";
static const char _caller[] = "caller";

// homescreen-service event and event handler function list
const std::unordered_map<std::string, HS_Client::func_handler> HS_Client::func_list {
    {"tap_shortcut",        &HS_Client::tap_shortcut},
    {"showWindow",          &HS_Client::showWindow},
    {"hideWindow",          &HS_Client::hideWindow},
    {"replyShowWindow",     &HS_Client::replyShowWindow},
    {"on_screen_message",   &HS_Client::on_screen_message},
    {"on_screen_reply",     &HS_Client::on_screen_reply},
    {"subscribe",           &HS_Client::subscribe},
    {"unsubscribe",         &HS_Client::unsubscribe},
    {"showNotification",    &HS_Client::showNotification},
    {"showInformation",     &HS_Client::showInformation},
    {"application-list-changed", nullptr}
};

/**
 * HS_Client construction function
 *
 * #### Parameters
 *  - id: app's id
 *
 * #### Return
 * None
 *
 */
HS_Client::HS_Client(afb_req_t request, std::string id) : my_id(id)
{
    my_event = afb_api_make_event(request->api, id.c_str());
}

/**
 * HS_Client destruction function
 *
 * #### Parameters
 *  - null
 *
 * #### Return
 * None
 *
 */
HS_Client::~HS_Client()
{
    afb_event_unref(my_event);
}

/**
 * push tap_shortcut event
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::tap_shortcut(afb_req_t request)
{
    (void) request;

    AFB_INFO("request appid = %s.", my_id.c_str());
    struct json_object* push_obj = json_object_new_object();
    hs_add_object_to_json_object_str( push_obj, 4, _application_id, my_id.c_str(),
    _type, __FUNCTION__);
    afb_event_push(my_event, push_obj);
    return 0;
}

/**
 * push on_screen_message event
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::on_screen_message(afb_req_t request)
{
    int ret = 0;
    const char* value = afb_req_value(request, _display_message);
    if (value) {
        AFB_INFO("push %s event message [%s].", __FUNCTION__, value);
        struct json_object* push_obj = json_object_new_object();
        hs_add_object_to_json_object_str( push_obj, 4, _display_message, value,
        _type, __FUNCTION__);
        afb_event_push(my_event, push_obj);
    }
    else {
        AFB_WARNING("Please input display_message");
        ret = AFB_EVENT_BAD_REQUEST;
    }
    return ret;
}

/**
 * push on_screen_reply event
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::on_screen_reply(afb_req_t request)
{
    int ret = 0;
    const char* value = afb_req_value(request, _reply_message);
    if (value) {
        AFB_INFO("push %s event message [%s].", __FUNCTION__, value);
        struct json_object* push_obj = json_object_new_object();
        hs_add_object_to_json_object_str( push_obj, 4, _reply_message, value,
        _type, __FUNCTION__);
        afb_event_push(my_event, push_obj);
    }
    else {
        AFB_WARNING("Please input reply_message");
        ret = AFB_EVENT_BAD_REQUEST;
    }
    return ret;
}

/**
 * subscribe event
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::subscribe(afb_req_t request)
{
    int ret = 0;
    const char *value = afb_req_value(request, _event);
    if(value) {
        AFB_INFO("subscribe event %s", value);
        if(!isSupportEvent(value)) {
            AFB_WARNING("subscibe event isn't existing.");
            ret = AFB_EVENT_BAD_REQUEST;
        }
        else {
            event_list.insert(std::string(value));
            if(!subscription) {
                ret = afb_req_subscribe(request, my_event);
                if(ret == 0) {
                    subscription = true;
                }
            }
        }
    }
    else {
        AFB_WARNING("Please input event name");
        ret = AFB_EVENT_BAD_REQUEST;
    }
    return ret;
}

/**
 * unsubscribe event
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::unsubscribe(afb_req_t request)
{
    int ret = 0;
    const char *value = afb_req_value(request, _event);
    if(value) {
        AFB_INFO("unsubscribe %s event", value);
        event_list.erase(std::string(value));
        if(event_list.empty()) {
            ret = afb_req_unsubscribe(request, my_event);
        }
    }
    else {
        AFB_WARNING("Please input event name");
        ret = AFB_EVENT_BAD_REQUEST;
    }
    return ret;
}

/**
 * showWindow event
 *
 * #### Parameters
 * - request : the request
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::showWindow(afb_req_t request)
{
    AFB_INFO("%s application_id = %s.", __FUNCTION__, my_id.c_str());
    int ret = 0;
    const char* param = afb_req_value(request, _parameter);
    if(param) {
        std::string req_appid = std::move(get_application_id(request));
        if(req_appid.empty()) {
            AFB_WARNING("can't get application identifier");
            return AFB_REQ_GETAPPLICATIONID_ERROR;
        }

        struct json_object* push_obj = json_object_new_object();
        hs_add_object_to_json_object_str( push_obj, 4, _application_id, my_id.c_str(), _type, __FUNCTION__);
        struct json_object* param_obj = json_tokener_parse(param);
        json_object_object_add(param_obj, _replyto, json_object_new_string(req_appid.c_str()));
        json_object_object_add(push_obj, _parameter, param_obj);
        afb_event_push(my_event, push_obj);
    }
    else {
        AFB_WARNING("please input correct parameter.");
        ret = AFB_EVENT_BAD_REQUEST;
    }
    return ret;
}

/**
 * hideWindow event
 *
 * #### Parameters
 * - request : the request
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::hideWindow(afb_req_t request)
{
    std::string req_appid = std::move(get_application_id(request));
    if(req_appid.empty()) {
        AFB_WARNING("can't get application identifier");
        return AFB_REQ_GETAPPLICATIONID_ERROR;
    }

    struct json_object* push_obj = json_object_new_object();
    hs_add_object_to_json_object_str( push_obj, 4, _application_id, my_id.c_str(),
    _type, __FUNCTION__);
    struct json_object* param_obj = json_object_new_object();
    json_object_object_add(param_obj, _caller, json_object_new_string(req_appid.c_str()));
    json_object_object_add(push_obj, _parameter, param_obj);
    afb_event_push(my_event, push_obj);
    return 0;
}

/**
 * replyShowWindow event
 *
 * #### Parameters
 * - request : the request
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::replyShowWindow(afb_req_t request)
{
    AFB_INFO("%s application_id = %s.", __FUNCTION__, my_id.c_str());
    int ret = 0;
    const char* param = afb_req_value(request, _parameter);
    if(param) {
        struct json_object* push_obj = json_object_new_object();
        hs_add_object_to_json_object_str( push_obj, 4, _application_id, my_id.c_str(), _type, __FUNCTION__);
        json_object_object_add(push_obj, _parameter, json_tokener_parse(param));
        afb_event_push(my_event, push_obj);
    }
    else {
        AFB_WARNING("please input correct parameter.");
        ret = AFB_EVENT_BAD_REQUEST;
    }
    return ret;
}

/**
 * showNotification event
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::showNotification(afb_req_t request)
{
    int ret = 0;
    const char *value = afb_req_value(request, _text);
    if(value) {
        AFB_INFO("text is %s", value);
        std::string appid =std::move(get_application_id(request));
        if(appid.empty()) {
            AFB_WARNING("can't get application identifier");
            return AFB_REQ_GETAPPLICATIONID_ERROR;
        }

        const char *icon = afb_req_value(request, _icon);
        if(icon) {
            struct json_object* param_obj = json_object_new_object();
            json_object_object_add(param_obj, _icon, json_object_new_string(icon));
            json_object_object_add(param_obj, _text, json_object_new_string(value));
            json_object_object_add(param_obj, _caller, json_object_new_string(appid.c_str()));
            struct json_object* push_obj = json_object_new_object();
            hs_add_object_to_json_object_str( push_obj, 4, _application_id, my_id.c_str(), _type, __FUNCTION__);
            json_object_object_add(push_obj, _parameter, param_obj);
            afb_event_push(my_event, push_obj);
        }
        else {
            AFB_WARNING("please input icon.");
            ret = AFB_REQ_SHOWNOTIFICATION_ERROR;
        }
    }
    else {
        AFB_WARNING("please input text.");
        ret = AFB_REQ_SHOWNOTIFICATION_ERROR;
    }

    return ret;
}

/**
 * showInformation event
 *
 * #### Parameters
 *  - request : the request
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::showInformation(afb_req_t request)
{
    int ret = 0;
    const char *value = afb_req_value(request, _info);
    if(value) {
        AFB_INFO("info is %s", value);
        std::string appid = std::move(get_application_id(request));
        if(appid.empty()) {
            AFB_WARNING("can't get application identifier");
            return AFB_REQ_GETAPPLICATIONID_ERROR;
        }

        struct json_object* param_obj = json_object_new_object();
        json_object_object_add(param_obj, _info, json_object_new_string(value));
        struct json_object* push_obj = json_object_new_object();
        hs_add_object_to_json_object_str( push_obj, 4, _application_id, my_id.c_str(), _type, __FUNCTION__);
        json_object_object_add(push_obj, _parameter, param_obj);
        afb_event_push(my_event, push_obj);
    }
    else {
        AFB_WARNING("please input information.");
        ret = AFB_REQ_SHOWINFORMATION_ERROR;
    }

    return ret;
}

/**
 * check if client subscribe event
 *
 * #### Parameters
 *  - event: homescreen event, tap_shortcut etc.
 *
 * #### Return
 * true: found
 * false: not found
 *
 */
bool HS_Client::checkEvent(const char* event)
{
    auto ip = event_list.find(std::string(event));
    if(ip == event_list.end())
        return false;
    else
        return true;
}

/**
 * check if event is supporting
 *
 * #### Parameters
 *  - event: homescreen event, tap_shortcut etc.
 *
 * #### Return
 * true: support
 * false: not fosupportund
 *
 */
bool HS_Client::isSupportEvent(const char* event)
{
    auto ip = func_list.find(std::string(event));
    if(ip == func_list.end())
        return false;
    else
        return true;
}

/**
 * handle homescreen event
 *
 * #### Parameters
 *  - request : the request
 *  - verb: request verb name
 *
 * #### Return
 * 0: success
 * others: fail
 *
 */
int HS_Client::handleRequest(afb_req_t request, const char *verb)
{
    if((strcasecmp(verb, "subscribe") && strcasecmp(verb, "unsubscribe")) && !checkEvent(verb))
        return 0;

    int ret = AFB_EVENT_BAD_REQUEST;
    auto ip = func_list.find(std::string(verb));
    if(ip != func_list.end() && ip->second != nullptr) {
        AFB_INFO("[%s]verb found", verb);
        ret = (this->*(ip->second))(request);
    }
    return ret;
}

/**
 * push event
 *
 * #### Parameters
 *  - event : the event want to push
 *  - param : the parameter contents of event
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_Client::pushEvent(const char *event, struct json_object *param)
{
    if(!checkEvent(event))
        return 0;

    AFB_INFO("called, event=%s.",event);
    struct json_object* push_obj = json_object_new_object();
    hs_add_object_to_json_object_str( push_obj, 4, _application_id, my_id.c_str(), _type, event);
    if(param != nullptr)
        json_object_object_add(push_obj, _parameter, param);
    afb_event_push(my_event, push_obj);
    return 0;
}
