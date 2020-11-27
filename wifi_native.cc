#ifndef UNICODE
#define UNICODE
#endif

#include <assert.h>
#include <node_api.h>
#include <napi.h>
#include <Windows.h>
#include <wlanapi.h>
#include <windot11.h>
#include <stdio.h>
#include <stdlib.h>
#include <comdef.h>
#include <string>
#include <thread>
#include <mutex>
#include <future>
#include "WlanApiClass.h"

using namespace std;

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")

BOOL initflag = FALSE;
WlanApiClass wlanApi;
BOOL background_scan_finished = FALSE;
/**
 * ============== Internal Function ==============
*/

int listener()
{
  if (wlanApi.callbackInfo.callbackReason == wlan_notification_acm_scan_complete)
  {
    printf("ok scan\n");
    return 0;
  }
  else if (wlanApi.callbackInfo.callbackReason == wlan_notification_acm_connection_complete)
  {
    printf("ok connnect\n");
    wprintf(L"wlan_notification_acm_connection_complete\n");
    return 1;
  }
  else if (wlanApi.callbackInfo.callbackReason == wlan_notification_acm_connection_attempt_fail)
  {
    wprintf(L"wlan_notification_acm_connection_attempt_fail\n");
    return 2;
  }
  else
  {
    // others we don't really care
    return wlanApi.callbackInfo.callbackReason;
  }
}

/**
 * ========== These functions need to export ===========
*/
Napi::Value callEvents(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  return Napi::Number::New(env, listener());
}

napi_value wlan_init(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  napi_value result;
  if (wlanApi.init() == S_OK)
  {
    initflag = TRUE;
    napi_create_int32(env, 0, &result);
    return result;
  }
  else
  {
    napi_create_int32(env, 1, &result);
    return result;
  }
}

Napi::Value wlan_scan_async(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (!initflag)
  {
    return Napi::String::New(env, "You should call init() first");
  }
  if (wlanApi.scanAsync() == S_OK)
  {
    Napi::Function cb = info[0].As<Napi::Function>();
    cb.Call(env.Global(), {Napi::Number::New(env, 0)});
  }
  else
  {
    Napi::Function cb = info[0].As<Napi::Function>();
    cb.Call(env.Global(), {Napi::Number::New(env, 1)});
  }
  return Napi::String::New(env, "scan");
}

Napi::Value wlan_scan_sync(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (!initflag)
  {
    return Napi::String::New(env, "You should call init() first");
  }
  if (wlanApi.scanSync() == S_OK)
  {
    Napi::Function cb = info[0].As<Napi::Function>();
    cb.Call(env.Global(), {Napi::Number::New(env, 0)});
  }
  else
  {
    Napi::Function cb = info[0].As<Napi::Function>();
    cb.Call(env.Global(), {Napi::Number::New(env, 1)});
  }
  return Napi::String::New(env, "scan");
}

Napi::Value get_network_list(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (!initflag)
  {
    return Napi::String::New(env, "You should call init() first");
  }
  napi_status status;
  napi_value scan_result_arr = NULL;
  status = napi_create_array(env, &scan_result_arr);

  vector<SCAN_RESULT> scan_result_list = wlanApi.get_network_list();
  vector<SCAN_RESULT>::iterator begin = scan_result_list.begin();
  vector<SCAN_RESULT>::iterator end = scan_result_list.end();
  vector<SCAN_RESULT>::iterator scan_result_it;

  int nb_AP = 0;
  for (scan_result_it = begin; scan_result_it != end; scan_result_it++)
  {
    napi_value scan_result;
    status = napi_create_object(env, &scan_result);
    status = napi_set_named_property(env, scan_result, "ssid", Napi::String::New(env, scan_result_it->ssid));
    status = napi_set_named_property(env, scan_result, "rssi", Napi::Number::New(env, scan_result_it->rssi));
    status = napi_set_element(env, scan_result_arr, nb_AP, scan_result);
    nb_AP++;
  }

  Napi::Function cb = info[0].As<Napi::Function>();
  cb.Call(env.Global(), {scan_result_arr});
  return Napi::String::New(env, "getList");
}

Napi::Value wlan_get_info(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (!initflag)
  {
    return Napi::String::New(env, "You should call init() first");
  }

  napi_status status;
  napi_value ifaceList;
  status = napi_create_array(env, &ifaceList);

  vector<INTERFACE_INFO> interface_info_list = wlanApi.get_interface_info();
  vector<INTERFACE_INFO>::iterator begin = interface_info_list.begin();
  vector<INTERFACE_INFO>::iterator end = interface_info_list.end();
  vector<INTERFACE_INFO>::iterator interface_info_it;

  int nb_interface = 0;
  for (interface_info_it = begin; interface_info_it != end; interface_info_it++)
  {
    napi_value ifaceInfo;
    status = napi_create_object(env, &ifaceInfo);
    status = napi_set_named_property(env, ifaceInfo, "guid", Napi::String::New(env, interface_info_it->guid));
    status = napi_set_named_property(env, ifaceInfo, "description", Napi::String::New(env, interface_info_it->description));
    status = napi_set_named_property(env, ifaceInfo, "connection", Napi::String::New(env, interface_info_it->connection));
    if (interface_info_it->connection == "connected")
    {
      status = napi_set_named_property(env, ifaceInfo, "mode", Napi::String::New(env, interface_info_it->mode));
      status = napi_set_named_property(env, ifaceInfo, "profile_name", Napi::String::New(env, interface_info_it->profile_name));
      status = napi_set_named_property(env, ifaceInfo, "ssid", Napi::String::New(env, interface_info_it->ssid));
      status = napi_set_named_property(env, ifaceInfo, "bssid_type", Napi::String::New(env, interface_info_it->bssid_type));
      status = napi_set_named_property(env, ifaceInfo, "AP_MAC", Napi::String::New(env, interface_info_it->AP_MAC));
    }
    status = napi_set_element(env, ifaceList, nb_interface, ifaceInfo);
    nb_interface++;
  }

  Napi::Function cb = info[0].As<Napi::Function>();
  cb.Call(env.Global(), {ifaceList});
  return Napi::String::New(env, "get info");
}

Napi::Value wlan_connect(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  char GuidString[40] = {0};
  WCHAR GuidWString[40] = {0};
  DWORD dwResult;
  GUID guid_for_wlan;
  char *profile = NULL;
  char *ssid = NULL;
  WCHAR *profileName = NULL;
  WCHAR *Wprofile = NULL;
  DOT11_SSID ap_ssid;

  if (!initflag)
  {
    printf("free memory...");
    free_memory(profile);
    free_memory(Wprofile);
    free_memory(ssid);
    free_memory(profileName);
    printf("ok\n");
    return Napi::String::New(env, "Un-initialization");
  }
  else
  {
    printf("Copy GUID... ");
    size_t strSize = 40;
    std::string guid_arg = info[0].As<Napi::String>();
    strcpy(GuidString, guid_arg.c_str());
    // To wchar
    mbstowcs(GuidWString, GuidString, 40);
    // To GUID
    dwResult = CLSIDFromString(GuidWString, &guid_for_wlan);
    if (dwResult != NOERROR)
    {
      wprintf(L"CLSIDFromString failed with error: %u\n", dwResult);
    }
    printf("ok\n");

    // get profile content
    printf("Copy profile contents... ");
    std::string profile_arg = info[1].As<Napi::String>();
    size_t profileLen = profile_arg.length();
    profile = (char *)malloc(sizeof(char) * profileLen);
    strcpy(profile, profile_arg.c_str());
    Wprofile = (WCHAR *)malloc(sizeof(WCHAR) * profileLen);
    mbstowcs(Wprofile, profile, profileLen + 1);
    printf("ok\n");

    // get filename of wi_fi profile for WlanSetProfile()
    printf("Copy profileName... ");
    std::string ssid_arg = info[2].As<Napi::String>();
    size_t ssidLen = ssid_arg.length();
    ssid = (char *)malloc(sizeof(char) * ssidLen);
    strcpy(ssid, ssid_arg.c_str());
    profileName = (WCHAR *)malloc(sizeof(WCHAR) * ssidLen);
    mbstowcs(profileName, ssid, ssidLen + 1);
    printf("ok\n");

    // get ssid
    printf("Copy ssid... ");
    ap_ssid.uSSIDLength = (ULONG)ssidLen;
    memcpy(ap_ssid.ucSSID, (UCHAR *)ssid, ssidLen);
    printf("ok\n");

    if (wlanApi.connect(guid_for_wlan, Wprofile, profileName, ssid, ssidLen) == S_OK)
    {
      printf("free memory...");
      free_memory(profile);
      free_memory(Wprofile);
      free_memory(ssid);
      free_memory(profileName);
      printf("ok\n");
      Napi::Function cb = info[3].As<Napi::Function>();
      cb.Call(env.Global(), {Napi::Number::New(env, 1)});
    }
    else
    {
      printf("free memory...");
      free_memory(profile);
      free_memory(Wprofile);
      free_memory(ssid);
      free_memory(profileName);
      printf("ok\n");
      Napi::Function cb = info[3].As<Napi::Function>();
      cb.Call(env.Global(), {Napi::Number::New(env, 0)});
    }
  }
  return Napi::String::New(env, "connect");
}

Napi::Value wlan_disconnect(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  char GuidString[40] = {0};
  WCHAR GuidWString[40] = {0};
  DWORD dwResult;
  GUID guid_for_wlan;

  if (!initflag)
  {
    return Napi::String::New(env, "You should call init() first");
  }

  // get guid
  printf("Copy GUID... ");
  size_t strSize = 40;
  std::string guid_arg = info[0].As<Napi::String>();
  strcpy(GuidString, guid_arg.c_str());
  // To wchar
  mbstowcs(GuidWString, GuidString, 40);
  // To GUID
  dwResult = CLSIDFromString(GuidWString, &guid_for_wlan);
  if (dwResult != NOERROR)
  {
    wprintf(L"CLSIDFromString failed with error: %u\n", dwResult);
  }
  printf("ok\n");

  // disconnect
  if (wlanApi.disconnect(guid_for_wlan) == S_OK)
  {
    Napi::Function cb = info[1].As<Napi::Function>();
    cb.Call(env.Global(), {Napi::Number::New(env, 1)});
  }
  else
  {
    wprintf(L"WlanDisconnect failed with error: %u\n", dwResult);
    Napi::Function cb = info[1].As<Napi::Function>();
    cb.Call(env.Global(), {Napi::Number::New(env, 0)});
  }
  return Napi::String::New(env, "disconnect");
}

Napi::Value wlan_free(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  wlanApi.~WlanApiClass();
  initflag = FALSE;
  return Napi::String::New(env, "free");
}

Napi::Value check_scan_finish(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, wlanApi.scan_finish());
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "wlanListener"), Napi::Function::New(env, callEvents));
  exports.Set(Napi::String::New(env, "wlanInit"), Napi::Function::New(env, wlan_init));
  exports.Set(Napi::String::New(env, "wlanScanAsync"), Napi::Function::New(env, wlan_scan_async));
  exports.Set(Napi::String::New(env, "wlanScanSync"), Napi::Function::New(env, wlan_scan_sync));
  exports.Set(Napi::String::New(env, "wlanFree"), Napi::Function::New(env, wlan_free));
  exports.Set(Napi::String::New(env, "wlanConnect"), Napi::Function::New(env, wlan_connect));
  exports.Set(Napi::String::New(env, "wlanGetNetworkList"), Napi::Function::New(env, get_network_list));
  exports.Set(Napi::String::New(env, "wlanDisconnect"), Napi::Function::New(env, wlan_disconnect));
  exports.Set(Napi::String::New(env, "wlanGetIfaceInfo"), Napi::Function::New(env, wlan_get_info));
  exports.Set(Napi::String::New(env, "wlanCheckScanFinish"), Napi::Function::New(env, check_scan_finish));
  return exports;
}
NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init);
