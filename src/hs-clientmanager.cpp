/*
 * Copyright (c) 2018 TOYOTA MOTOR CORPORATION
 * Copyright (C) 2020 Konsulko Group
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
#include <algorithm>
#include <cassert>
#include "hs-proxy.h"
#include "hs-clientmanager.h"

static const char _homescreen[] = "homescreen";

HS_ClientManager* HS_ClientManager::me = nullptr;

static void cbRemoveClientCtxt(void *data)
{
    HS_ClientManager::instance()->removeClientCtxt(data);
}

/**
 * HS_ClientManager construction function
 *
 * #### Parameters
 *  - Nothing
 *
 * #### Return
 * None
 *
 */
HS_ClientManager::HS_ClientManager()
{
}

/**
 * get instance
 *
 * #### Parameters
 *  - Nothing
 *
 * #### Return
 * HS_ClientManager instance pointer
 *
 */
HS_ClientManager* HS_ClientManager::instance(void)
{
    if(me == nullptr)
        me = new HS_ClientManager();

    return me;
}

/**
 * HS_ClientManager init function
 *
 * #### Parameters
 *  - Nothing
 *
 * #### Return
 * init result
 *
 */
int HS_ClientManager::init(void)
{
    return 0;
}

/**
 * create client's afb_req_context
 *
 * #### Parameters
 *  - appid: app's id
 *
 * #### Return
 * HS_ClientCtxt pointer
 *
 */
HS_ClientCtxt* HS_ClientManager::createClientCtxt(afb_req_t req, std::string appid)
{
    HS_ClientCtxt *ctxt = (HS_ClientCtxt *)afb_req_context_get(req);
    if (!ctxt)
    {
        AFB_INFO( "create new session for %s", appid.c_str());
        ctxt = new HS_ClientCtxt(appid);
        afb_req_session_set_LOA(req, 1);
        afb_req_context_set(req, ctxt, cbRemoveClientCtxt);

	appid2ctxt[appid] = ctxt;
    }

    return ctxt;
}

/**
 * add Client
 *
 * #### Parameters
 *  - ctxt: app's id
 *
 * #### Return
 * HS_Client pointer
 *
 */
HS_Client* HS_ClientManager::addClient(afb_req_t req, std::string appid)
{
    return (client_list[appid] = new HS_Client(req, appid));
}

/**
 * remove Client
 *
 * #### Parameters
 *  - appid: app's id
 *
 * #### Return
 * None
 *
 */
void HS_ClientManager::removeClient(std::string appid)
{
    delete client_list[appid];
    client_list.erase(appid);
}

/**
 * remove Client from list
 *
 * #### Parameters
 *  - data: HS_ClientCtxt pointer
 *
 * #### Return
 * None
 *
 */
void HS_ClientManager::removeClientCtxt(void *data)
{
    HS_ClientCtxt *ctxt = (HS_ClientCtxt *)data;
    if(ctxt == nullptr)
    {
        AFB_WARNING( "data is nullptr");
        return;
    }

    AFB_INFO( "remove app %s", ctxt->id.c_str());
    std::lock_guard<std::mutex> lock(this->mtx);
    removeClient(ctxt->id);
    delete appid2ctxt[ctxt->id];
    appid2ctxt.erase(ctxt->id);
}

static int
is_application_running(std::string appid, std::unordered_map<std::string, HS_Client*> client_list)
{
    bool app_still_running = false;

    std::string id(appid);
    auto ip = client_list.find(id);

    // this will always be case as the removeClientCtxt() is never called as
    // clients do not handle the subscribe at all at this point. Not only that
    // but is redudant but we keep at as it to highlight the fact that we're
    // missing a feature to check if applications died or not (legitimate or not).
    //
    // Using ps (HS_AfmMainProxy::ps -- a sync version of checking if running
    // applications) seem to block in afm-system-daemon (see SPEC-3902), and
    // doing w/ it with an async version of ps doesn't work because we don't
    // have a valid clientCtx (required, and only possible if the application
    // subscribed themselves, which no longer happens)
    //
    // FIXME: We need another way of handling this would be necessary to correctly
    // handle the case where the app died.
    if (ip != client_list.end()) {
        app_still_running = true;
    }

    // > 0 means err
    return app_still_running != true;
}

/**
 * handle homescreen request
 *
 * #### Parameters
 *  - request : the request
 *  - verb : the verb name
 *  - appid : to which application
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_ClientManager::handleRequest(afb_req_t request, const char *verb, const char *appid)
{
    AFB_INFO("verb=[%s],appid=[%s].", verb, appid);
    int ret = 0;
    std::lock_guard<std::mutex> lock(this->mtx);
    if(appid == nullptr) {
        for(auto m : client_list) {
            m.second->handleRequest(request, verb);
        }
    }
    else {
        std::string id(appid);
        auto ip = client_list.find(id);
	if(ip != client_list.end()) {
	    // for showWindow verb we need to verify if the app is (still)
	    // running, and return the appropriate value to attempt to start it
	    // again. This 'problem' is avoided if the application itself
	    // subscribes and with that process, to install a callback that
	    // automatically removes the application from client_list.
	    // That is exactly how "subscribe" verb is handled below.
            if (strcasecmp(verb, "showWindow") == 0) {
                ret = is_application_running(id, client_list);
                if (ret == AFB_REQ_NOT_STARTED_APPLICATION) {
                    AFB_INFO("%s is not running. Will attempt to start it", appid);
                    return ret;
                }
            }
            AFB_INFO("%s found to be running. Forwarding request to the client", appid);
            ret = ip->second->handleRequest(request, verb);
        }
        else {
            if(!strcasecmp(verb, "subscribe")) {
                createClientCtxt(request, id);
                HS_Client* client = addClient(request, id);
                ret = client->handleRequest(request, "subscribe");
            }
            else {
                AFB_NOTICE("not exist session");
                ret = AFB_REQ_NOT_STARTED_APPLICATION;
            }
        }
    }
    return ret;
}

/**
 * push event
 *
 * #### Parameters
 *  - event : the event want to push
 *  - param : the parameter contents of event
 *  - appid : the destination application's id
 *
 * #### Return
 * 0 : success
 * others : fail
 *
 */
int HS_ClientManager::pushEvent(const char *event, struct json_object *param, std::string appid)
{
    if(event == nullptr) {
        AFB_WARNING("event name is null.");
        return -1;
    }

    std::lock_guard<std::mutex> lock(this->mtx);
    if(appid.empty()) { // broadcast event to clients who subscribed this event
        for(auto m : client_list) {
            m.second->pushEvent(event, param);
        }
    }
    else {  // push event to specific client
        auto ip = client_list.find(appid);
        if(ip != client_list.end()) {
            ip->second->pushEvent(event, param);
        }
    }

    return 0;
}
