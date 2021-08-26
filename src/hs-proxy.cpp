/*
 * Copyright (c) 2019 TOYOTA MOTOR CORPORATION
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

#include "homescreen.h"
#include "hs-proxy.h"

struct closure_data {
	std::string appid;
	HS_ClientCtxt *clientCtx;
	struct hs_instance *hs_instance;
};

const char _afm_main[] = "afm-main";


/**
 * the callback function
 *
 * #### Parameters
 *  - closure : the user defined closure pointer 'closure'
 *  - object : a JSON object returned (can be NULL)
 *  - error : a string not NULL in case of error but NULL on success
 *  - info : a string handling some info (can be NULL)
 *  - api : the api
 *
 * #### Return
 *  None
 *
 */
static void api_callback(void *closure, struct json_object *object, const char *error, const char *info, afb_api_t api)
{
    AFB_INFO("asynchronous call, error=%s, info=%s, object=%s.", error, info, json_object_get_string(object));
    (void) api;
    struct closure_data *cdata = static_cast<struct closure_data *>(closure);

    if (!cdata->hs_instance) {
           return;
    }

    struct HS_ClientManager *clientManager = cdata->hs_instance->client_manager;
    if (!clientManager) {
           return;
    }

    /* if we have an error then we couldn't start the application so we remove it */
    if (error) {
	   AFB_INFO("asynchronous call, removing client %s", cdata->appid.c_str());
           clientManager->removeClient(cdata->appid);
    }

    free(cdata);
}

/**
 * call api asynchronous
 *
 * #### Parameters
 *  - api : the api serving the request
 *  - service : the api name of service
 *  - verb : the verb of service
 *  - args : parameter
 *
 * #### Return
 *  None
 *
 */
static void api_call(afb_api_t api, const char *service, const char *verb, struct json_object *args, struct closure_data *cdata)
{
    AFB_INFO("service=%s verb=%s, args=%s.", service, verb, json_object_get_string(args));
    afb_api_call(api, service, verb, args, api_callback, cdata);
}

/**
 * call api synchronous
 *
 * #### Parameters
 *  - api : the api serving the request
 *  - service : the api name of service
 *  - verb : the verb of afm-main
 *  - args : parameter
 *  - object : return the details of application
 *
 * #### Return
 *  0 : success
 *  1 : fail
 *
 */
static int api_call_sync(afb_api_t api, const char *service, const char *verb, struct json_object *args, struct json_object **object)
{
    char *error = nullptr, *info = nullptr;
    int ret = afb_api_call_sync(api, service, verb, args, object, &error, &info);
    AFB_INFO("synchronous call, error=%s, info=%s.", error, info);
    return ret;
}

/**
 * get runnables application list
 *
 * #### Parameters
 *  - api : the api serving the request
 *  - object : return the details of appid
 *
 * #### Return
 *  0 : success
 *  1 : fail
 *
 */
int HS_AfmMainProxy::runnables(afb_api_t api, struct json_object **object)
{
    return api_call_sync(api, _afm_main, __FUNCTION__, nullptr, object);
}

/**
 * get running application list
 *
 * #### Parameters
 *  - api : the api serving the request
 *  - object : return the details of appid
 *
 * #### Return
 *  0 : success
 *  1 : fail
 *
 */
int HS_AfmMainProxy::ps(afb_api_t api, struct json_object **object)
{
    return api_call_sync(api, _afm_main, "runners", nullptr, object);
}

/**
 * get details of application
 *
 * #### Parameters
 *  - api : the api serving the request
 *  - id : the id to get details,liked "dashboard@0.1"
 *  - object : return the details of application
 *
 * #### Return
 *  0 : success
 *  1 : fail
 *
 */
int HS_AfmMainProxy::detail(afb_api_t api, const std::string &id, struct json_object **object)
{
    struct json_object *args = json_object_new_string(id.c_str());
    return api_call_sync(api, _afm_main, __FUNCTION__, args, object);
}

/**
 * start application
 *
 * #### Parameters
 *  - request : the request
 *  - id : the application id liked "dashboard@0.1"
 *
 * #### Return
 *  None
 *
 */
void HS_AfmMainProxy::start(struct hs_instance *instance, afb_req_t request, const std::string &id)
{
    struct closure_data *cdata;

    /* tentatively store the client and client context, as the afb_req_t
     * request will no longer be available in the async callback handler. This
     * is similar to that is done showWindow(), handleRequest() in
     * homescreen.cpp, but allows to fake the subscription here as well to
     * avoid clients create/install dummy event handlers as to 'register' (or
     * to keep track of applications started).
     *
     * In case api_callback() does return an error we'll remove then the client
     * and client context there. We pass the closure_data with the client context
     * and the application id to remove it.
     */
    if (!instance || id.empty())
	    return;

    cdata = static_cast<struct closure_data *>(calloc(1, sizeof(*cdata)));
    cdata->hs_instance = instance;
    cdata->appid = id;

    struct HS_ClientManager *clientManager = instance->client_manager;
    if (!clientManager) {
	    return;
    }

    clientManager->addClient(request, id);
    api_call(request->api, _afm_main, __FUNCTION__, json_object_new_string(id.c_str()), cdata);
}
