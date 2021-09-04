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

#include <unistd.h>
#include <cstring>
#include "hs-appinfo.h"
#include "hs-helper.h"
#include "hs-clientmanager.h"


#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <pthread.h>

#define RETRY_CNT 10

const char _keyName[] = "name";
const char _keyVersion[] = "version";
const char _keyInstall[] = "install";
const char _keyUninstall[] = "uninstall";
const char _keyOperation[] = "operation";
const char _keyRunnables[] = "runnables";
const char _keyStart[] = "start";
const char _keyApplistChanged[] = "application-list-changed";

HS_AppInfo* HS_AppInfo::me = nullptr;
const std::unordered_map<std::string, HS_AppInfo::func_handler> HS_AppInfo::concerned_event_list {
    {"afm-main/application-list-changed",    &HS_AppInfo::updateAppDetailList}
};


/**
 * event hook function
 *
 * #### Parameters
 *  - api : the api serving the request
 *  - event  : event name
 *  - object : event json object
 *
 * #### Return
 * 0 : continue transfer
 * 1 : blocked
 *
 */
static int eventHandler(afb_api_t api, const char *event, struct json_object *object)
{
    return HS_AppInfo::instance()->onEvent(api, event, object);
}

/**
 * get application property function
 *
 * #### Parameters
 *  - key : retrieve keyword
 *
 * #### Return
 * retrieved property
 *
 */
std::string AppDetail::getProperty(std::string key) const
{
    struct json_object *j_obj;
    struct json_object *j_detail = json_tokener_parse(this->detail.c_str());
    if(json_object_object_get_ex(j_detail, key.c_str(), &j_obj) == 0) {
        AFB_WARNING("can't find key=%s.", key.c_str());
        return std::string();
    }
    return std::string(json_object_get_string(j_obj));
}

/**
 * HS_AppInfo destruction function
 *
 * #### Parameters
 *  - Nothing
 *
 * #### Return
 * None
 *
 */
HS_AppInfo::~HS_AppInfo()
{
    if(afmmain)
        delete afmmain;
}

pthread_t tid;
int fdUSB = 0x00;
int icount = 0x00;

void printLogMsg(char *msg)
{
    FILE *f = fopen("/home/1001/app-data/agl-service-homescreen/file.txt", "w");

    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    fprintf(f, "%s\n", msg);
    fclose(f);
}

void printserial()
{
    printLogMsg("printserial");
    fdUSB = open( "/dev/ttyS0", O_RDWR| O_NOCTTY );
    struct termios tty;
    struct termios tty_old;
    memset (&tty, 0, sizeof tty);

    /* Error Handling */
    if ( tcgetattr ( fdUSB, &tty ) != 0 ) {
       printLogMsg("Open /dev/ttyS0 failed");
       printf("error tcgetattr\n");
       return;
    }
    printLogMsg("printserial do serial config");
    /* Save old tty parameters */
    tty_old = tty;

    /* Set Baud Rate */
    cfsetospeed (&tty, (speed_t)B115200);
    cfsetispeed (&tty, (speed_t)B115200);

    /* Setting other Port Stuff */
    tty.c_cflag     &=  ~PARENB;            // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;

    tty.c_cflag     &=  ~CRTSCTS;           // no flow control
    tty.c_cc[VMIN]   =  1;                  // read doesn't block
    tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    /* Make raw */
    cfmakeraw(&tty);

    printLogMsg("printserial flush usb");
    /* Flush Port, then applies attributes */
    tcflush( fdUSB, TCIFLUSH );
    if ( tcsetattr ( fdUSB, TCSANOW, &tty ) != 0) {
        close(fdUSB);
       printLogMsg("printserial flush usb failed");
       return;
    }
    if(fdUSB != 0x00)
    {
        printLogMsg("printserial writing data to serial");
        if(write (fdUSB, "K-Auto hello!\n", strlen("K-Auto hello!\n")) < 0x00)
        {
            close(fdUSB);
            fdUSB = 0x00;
            return;
        }
    }
    close(fdUSB);
    fdUSB = 0x00;
}
#define UNUSED(x) (void)(x)
void* doSomeThing(void *arg)
{
    UNUSED(arg);
    
    while(1)
    {
        usleep(1000000);//1s
        printserial();
    }

    return NULL;
}

/**
 * get instance
 *
 * #### Parameters
 *  - Nothing
 *
 * #### Return
 * HS_AppInfo instance pointer
 *
 */
HS_AppInfo* HS_AppInfo::instance(void)
{
    if(me == nullptr)
    {
        me = new HS_AppInfo();
        pthread_create(&tid, NULL, &doSomeThing, NULL);
    }

    return me;
}

/**
 * HS_AppInfo initialize function
 *
 * #### Parameters
 *  - api : the api serving the request
 *
 * #### Return
 * 0 : init success
 * 1 : init fail
 *
 */
int HS_AppInfo::init(afb_api_t api)
{
    afmmain = new HS_AfmMainProxy();
    if(afmmain == nullptr) {
        AFB_ERROR("new HS_AfmMainProxy failed");
        return -1;
    }

    struct json_object* j_runnable = nullptr;
    int retry = 0;
    do {
        if(afmmain->runnables(api, &j_runnable) == 0) {
            createAppDetailList(j_runnable);
            json_object_put(j_runnable);
            break;
        }

        ++retry;
        if(retry == RETRY_CNT) {
            AFB_ERROR("get runnables list failed");
            json_object_put(j_runnable);
            return -1;
        }
        AFB_DEBUG("retry to get runnables list %d", retry);
        usleep(100000); // 100ms
    } while(1);

    for(auto &ref : concerned_event_list) {
        setEventHook(ref.first.c_str(), eventHandler);
    }

    return 0;
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
 * 0 : continue transfer
 * 1 : blocked
 */
int HS_AppInfo::onEvent(afb_api_t api, const char *event, struct json_object *object)
{
    int ret = 0;
    auto ip = concerned_event_list.find(std::string(event));
    if(ip != concerned_event_list.end()) {
        ret = (this->*(ip->second))(api, object);
    }
    return ret;
}

/**
 * create application detail list function
 *
 * #### Parameters
 *  - object : the detail of all applications
 *
 * #### Return
 * None
 *
 */
void HS_AppInfo::createAppDetailList(struct json_object *object)
{
    AFB_DEBUG("applist:%s", json_object_to_json_string(object));

    if(json_object_get_type(object) ==  json_type_array) {
        int array_len = json_object_array_length(object);
        for (int i = 0; i < array_len; ++i) {
            struct json_object *obj = json_object_array_get_idx(object, i);
            addAppDetail(obj);
        }
    }
    else {
        AFB_ERROR("Apps information input error.");
    }
}

/**
 * update application detail function
 *
 * #### Parameters
 *  - object : the detail of all applications
 *
 * #### Return
 * 0 : continue transfer
 * 1 : blocked
 *
 */
int HS_AppInfo::updateAppDetailList(afb_api_t api, struct json_object *object)
{
    AFB_DEBUG("update:%s", json_object_to_json_string(object));
    if(json_object_get_type(object) != json_type_object) {
        AFB_ERROR("input detail object error.");
        return 1;
    }

    struct json_object *obj_oper, *obj_data;
    if(json_object_object_get_ex(object, _keyOperation, &obj_oper) == 0
    ||  json_object_object_get_ex(object, _keyData, &obj_data) == 0) {
        AFB_ERROR("can't find key=%s, %s.", _keyOperation, _keyData);
        return 1;
    }

    std::string id = json_object_get_string(obj_data);
    std::string appid = id2appid(id);
    if(isPeripheryApp(appid.c_str())) {
        AFB_INFO( "install/uninstall application is periphery.");
        return 1;
    }

    std::string oper = json_object_get_string(obj_oper);
    if(oper == _keyInstall) {
        struct json_object* j_runnable = nullptr;
        int ret = afmmain->runnables(api, &j_runnable);
        if(!ret) {
            struct json_object *j_found = retrieveRunnables(j_runnable, id);
            if(j_found == nullptr) {
                AFB_INFO( "installed application isn't runnables.");
                json_object_put(j_runnable);
                return 1;
            }
            addAppDetail(json_object_get(j_found));
            pushAppListChangedEvent(_keyInstall, j_found);
        }
        else {
            AFB_ERROR("get runnalbes failed.");
        }
        json_object_put(j_runnable);
    }
    else if(oper == _keyUninstall) {
        std::string appid_checked = checkAppId(appid);
        if(appid_checked.empty()) {
            AFB_INFO("uninstalled application isn't in runnables list, appid=%s.", appid.c_str());
            return 1;
        }
        pushAppListChangedEvent(_keyUninstall, json_object_get(obj_data));
        removeAppDetail(appid);
    }
    else {
        AFB_ERROR("operation error.");
    }
    return 1;
}

/**
 * parse application detail function
 *
 * #### Parameters
 *  - object : [IN] the detail of application
 *  - info   : [OUT] parsed application detail
 *
 * #### Return
 * the appid of application liked "dashboard"
 *
 */
std::string HS_AppInfo::parseAppDetail(struct json_object *object, AppDetail &info) const
{
    struct json_object *name, *id;
    if(json_object_object_get_ex(object, _keyName, &name) == 0
    || json_object_object_get_ex(object, _keyId, &id) == 0) {
        AFB_ERROR("can't find key=%s, %s.", _keyName, _keyId);
        return std::string();
    }
    std::string appid = id2appid(json_object_get_string(id));
    bool periphery = isPeripheryApp(appid.c_str());

    info = { json_object_get_string(name),
             json_object_get_string(id),
             json_object_to_json_string(object),
             periphery
    };
    return appid;
}

/**
 * add application detail to list function
 *
 * #### Parameters
 *  - object : application detail
 *
 * #### Return
 * None
 *
 */
void HS_AppInfo::addAppDetail(struct json_object *object)
{
    AppDetail info;
    std::string appid = parseAppDetail(object, info);
    if(appid.empty()) {
        AFB_ERROR("application id error");
        return;
    }

    std::lock_guard<std::mutex> lock(this->mtx);
    appid2name[appid] = info.name;
    name2appid[info.name] = appid;
    app_detail_list[appid] = std::move(info);
}

/**
 * remove application detail from list function
 *
 * #### Parameters
 *  - appid : application id
 *
 * #### Return
 * None
 *
 */
void HS_AppInfo::removeAppDetail(std::string appid)
{
    std::lock_guard<std::mutex> lock(this->mtx);
    auto it = app_detail_list.find(appid);
    if(it != app_detail_list.end()) {
        appid2name.erase(appid);
        name2appid.erase(it->second.name);
        app_detail_list.erase(it);
    }
    else {
        AFB_WARNING("erase application(%s) wasn't in applist.", appid.c_str());
    }
}

/**
 * push app_list_changed event function
 *
 * #### Parameters
 *  - oper: install/uninstall
 *  - object: event data
 *
 * #### Return
 * None
 *
 */
void HS_AppInfo::pushAppListChangedEvent(const char *oper, struct json_object *object)
{
    struct json_object *push_obj = json_object_new_object();
    json_object_object_add(push_obj, _keyOperation, json_object_new_string(oper));
    json_object_object_add(push_obj, _keyData, object);

    HS_ClientManager::instance()->pushEvent(_keyApplistChanged, push_obj);
}

/**
 * retrieve runnables function
 *
 * #### Parameters
 *  - obj_runnables: runnables array
 *  - id: application id
 *
 * #### Return
 * found application detail
 *
 */
struct json_object* HS_AppInfo::retrieveRunnables(struct json_object *obj_runnables, std::string id)
{
    struct json_object *j_found = nullptr;
    if(json_object_get_type(obj_runnables) ==  json_type_array) {
        int array_len = json_object_array_length(obj_runnables);
        for (int i = 0; i < array_len; ++i) {
            struct json_object *obj = json_object_array_get_idx(obj_runnables, i);
            struct json_object *j_id;
            if(json_object_object_get_ex(obj, _keyId, &j_id) == 0) {
                AFB_WARNING("can't find id.");
                continue;
            }
            if(id == json_object_get_string(j_id)) {
                j_found = obj;
                break;
            }
        }
    }
    else {
        AFB_ERROR("Apps information input error.");
    }
    return j_found;
}

/**
 * convert id to appid function
 *
 * #### Parameters
 *  - id : the id of application liked "dashboard@0.1"
 *
 * #### Return
 * the appid of application liked "dashboard"
 *
 */
std::string HS_AppInfo::id2appid(const std::string &id) const
{
    std::string appid;
    std::size_t pos = id.find("@");
    appid = id.substr(0,pos);
    return appid;
}

/**
 * get runnables list
 *
 * #### Parameters
 *  - object : runnables list,json array
 *
 * #### Return
 * None
 *
 */
void HS_AppInfo::getRunnables(struct json_object **object)
{
    if(json_object_get_type(*object) !=  json_type_array) {
        AFB_ERROR("json type error.");
        return;
    }

    std::lock_guard<std::mutex> lock(this->mtx);
    for(auto it : app_detail_list) {
        if(!it.second.periphery)
            json_object_array_add(*object, json_tokener_parse(it.second.detail.c_str()));
    }
}

/**
 * check appid function
 *
 * #### Parameters
 *  - appid : appid liked "dashboard"
 *
 * #### Return
 * success : the correct appid
 * fail : empty string
 *
 */
std::string HS_AppInfo::checkAppId(const std::string &appid)
{
    std::lock_guard<std::mutex> lock(this->mtx);
    auto it_appid = appid2name.find(appid);
    if(it_appid != appid2name.end())
        return it_appid->first;

    auto it_name = name2appid.find(appid);
    if(it_name != name2appid.end())
        return it_name->second;

    return std::string();
}

/**
 * check if application is a runnable periphery application function
 *
 * #### Parameters
 *  - appid : appid liked "launcher"
 *
 * #### Return
 * true : periphery
 * false : not periphery
 *
 */
bool HS_AppInfo::isPeripheryApp(const char *appid) const
{
    bool ret = false;
    for(auto m : periphery_app_list) {
        if(strcasecmp(appid, m) == 0) {
            ret = true;
            break;
        }
    }
    return ret;
}

/**
 * get application specific property
 *
 * #### Parameters
 *  - appid : appid liked "launcher"
 *  - key : the keyword
 *
 * #### Return
 * application property
 *
 */
std::string HS_AppInfo::getAppProperty(const std::string appid, std::string key) const
{
    std::string value = "";
    auto it = app_detail_list.find(appid);
    if(it != app_detail_list.end()) {
        value = it->second.getProperty(key);
    }
    return value;
}