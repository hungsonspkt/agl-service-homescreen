#ifndef HOMESCREEN_H
#define HOMESCREEN_H

#include <memory>
#include <algorithm>
#include <unordered_map>
#include <list>

#include "hs-helper.h"
#include "hs-clientmanager.h"
#include "hs-appinfo.h"

struct hs_instance {
	HS_ClientManager *client_manager;   // the connection session manager
	HS_AppInfo *app_info;               // application info

	hs_instance() : client_manager(HS_ClientManager::instance()), app_info(HS_AppInfo::instance()) {}
	int init(afb_api_t api);
	void setEventHook(const char *event, const event_hook_func f);
	void onEvent(afb_api_t api, const char *event, struct json_object *object);
private:
	std::unordered_map<std::string, std::list<event_hook_func>> event_hook_list;
};

#endif
