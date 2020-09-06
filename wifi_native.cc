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
#include <thread>
#include <future>
#include <mutex>
#include <string>

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")

typedef struct _WLAN_CALLBACK_INFO
{
  GUID interfaceGUID;
  HANDLE handleEvent;
  DWORD callbackReason;
} WLAN_CALLBACK_INFO;

HANDLE hClient = NULL;
WLAN_CALLBACK_INFO callbackInfo = {0};
BOOL initflag = FALSE;
void free_memory(void *p)
{
  if (p)
  {
    free(p);
    p = NULL;
  }
}

int listener()
{
  if (callbackInfo.callbackReason == wlan_notification_acm_scan_complete)
  {
    printf("ok scan\n");
    return 0;
  }
  else if (callbackInfo.callbackReason == wlan_notification_acm_connection_complete)
  {
    printf("ok connnect\n");
    wprintf(L"wlan_notification_acm_connection_complete\n");
    return 1;
  }
  else if (callbackInfo.callbackReason == wlan_notification_acm_connection_attempt_fail)
  {
    wprintf(L"wlan_notification_acm_connection_attempt_fail\n");
    return 2;
  }
  else
  {
    // others we don't really care
    return 99;
  }
}

Napi::Value callEvents(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  return Napi::Number::New(env, listener());
}

void wlanCallback(WLAN_NOTIFICATION_DATA *WlanNotificationData, PVOID myContext)
{
  WLAN_CALLBACK_INFO *callbackInfo = (WLAN_CALLBACK_INFO *)myContext;
  if (callbackInfo == nullptr)
  {
    return;
  }
  if (memcmp(&callbackInfo->interfaceGUID, &WlanNotificationData->InterfaceGuid, sizeof(GUID)) != 0)
  {
    return;
  }
  if (WlanNotificationData->NotificationCode == wlan_notification_acm_scan_complete)
  {
    callbackInfo->callbackReason = WlanNotificationData->NotificationCode;
  }
  if ((WlanNotificationData->NotificationCode == wlan_notification_acm_connection_complete) ||
      (WlanNotificationData->NotificationCode == wlan_notification_acm_connection_attempt_fail))
  {
    callbackInfo->callbackReason = WlanNotificationData->NotificationCode;
  }
  return;
}

napi_value WlanInit(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  napi_value result;
  DWORD dwResult = 0;
  DWORD dwMaxClient = 2;
  DWORD dwCurVersion = 0;
  dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
  if (dwResult != ERROR_SUCCESS)
  {
    wprintf(L"WlanOpenHandle failed with error: %u\n", dwResult);
    napi_create_int32(env, 1, &result);
    return result;
  }
  dwResult = WlanRegisterNotification(hClient, WLAN_NOTIFICATION_SOURCE_ALL, TRUE, (WLAN_NOTIFICATION_CALLBACK)wlanCallback, (PVOID)&callbackInfo, NULL, NULL);
  if (dwResult != ERROR_SUCCESS)
  {
    wprintf(L"WlanRegisterNotification failed with error: %u\n", dwResult);
    napi_create_int32(env, 1, &result);
    return result;
  }
  initflag = TRUE;
  napi_create_int32(env, 0, &result);
  return result;
}

Napi::Value Scan(Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (!initflag)
  {
    return Napi::String::New(env, "You should call init() first");
  }
  Napi::Value scan_flag;
  DWORD dwResult = 0;
  PWLAN_INTERFACE_INFO pIfInfo = NULL;
  PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
  PDOT11_SSID pDotSSid = NULL;

  unsigned int ifaceNum = 0;
  dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
  if (dwResult != ERROR_SUCCESS)
  {
    wprintf(L"WlanEnumInterfaces failed with error: %u\n", dwResult);
  }
  else
  {
    uint32_t correct_counter = 0;
    for (ifaceNum = 0; ifaceNum < (int)pIfList->dwNumberOfItems; ifaceNum++)
    {
      pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[ifaceNum];
      PWLAN_RAW_DATA WlanRawData = NULL;
      callbackInfo.interfaceGUID = pIfInfo->InterfaceGuid;
      callbackInfo.handleEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
      printf("WlanScan... ");
      dwResult = WlanScan(hClient, &pIfInfo->InterfaceGuid, pDotSSid, WlanRawData, NULL);
      if (dwResult != ERROR_SUCCESS)
      {
        wprintf(L"WlanScan failed with error: %u\n", dwResult);
      }
    }
    Napi::Function cb = info[0].As<Napi::Function>();
    cb.Call(env.Global(), {Napi::Number::New(env, 0)});
  }
  if (pIfList != NULL)
  {
    WlanFreeMemory(pIfList);
    pIfList = NULL;
  }
  if (pDotSSid != NULL)
  {
    WlanFreeMemory(pDotSSid);
    pDotSSid = NULL;
  }
  return Napi::String::New(env, "scan");
}

Napi::Value GetNetworkList(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (!initflag)
  {
    return Napi::String::New(env, "You should call init() first");
  }
  napi_status status;
  napi_value scan_result_arr = NULL;
  status = napi_create_array(env, &scan_result_arr);
  DWORD dwResult = 0;
  PWLAN_BSS_LIST WlanBssList = NULL;
  PWLAN_INTERFACE_INFO pIfInfo = NULL;
  PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
  PDOT11_SSID pDotSSid = NULL;

  unsigned int ifaceNum = 0;
  dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
  if (dwResult != ERROR_SUCCESS)
  {
    wprintf(L"WlanEnumInterfaces failed with error: %u\n", dwResult);
  }
  else
  {
    uint32_t correct_counter = 0;
    for (ifaceNum = 0; ifaceNum < (int)pIfList->dwNumberOfItems; ifaceNum++)
    {
      pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[ifaceNum];

      printf("WlanShowNetworksList... ");
      if (WlanGetNetworkBssList(hClient, &pIfInfo->InterfaceGuid, pDotSSid, dot11_BSS_type_infrastructure, FALSE, NULL, &WlanBssList) == ERROR_SUCCESS)
      {
        printf("ok\n");
        for (unsigned int c = 0; c < WlanBssList->dwNumberOfItems; c++)
        {
          wprintf(L"ssid: %hs, rssi: %d\n", WlanBssList->wlanBssEntries[c].dot11Ssid.ucSSID, WlanBssList->wlanBssEntries[c].lRssi);
          if (strstr((char *)WlanBssList->wlanBssEntries[c].dot11Ssid.ucSSID, "MediCam_"))
          {
            napi_value ssid, rssi, scan_result;
            status = napi_create_object(env, &scan_result);
            status = napi_create_string_utf8(env, (char *)WlanBssList->wlanBssEntries[c].dot11Ssid.ucSSID, (size_t)WlanBssList->wlanBssEntries[c].dot11Ssid.uSSIDLength, &ssid);
            status = napi_create_int64(env, WlanBssList->wlanBssEntries[c].lRssi, &rssi);
            status = napi_set_named_property(env, scan_result, "ssid", ssid);
            status = napi_set_named_property(env, scan_result, "rssi", rssi);
            status = napi_set_element(env, scan_result_arr, correct_counter, scan_result);
            correct_counter++;
          }
        }
      }
    }
    Napi::Function cb = info[0].As<Napi::Function>();
    cb.Call(env.Global(), {scan_result_arr});
  }
  if (pIfList != NULL)
  {
    WlanFreeMemory(pIfList);
    pIfList = NULL;
  }
  if (pDotSSid != NULL)
  {
    WlanFreeMemory(pDotSSid);
    pDotSSid = NULL;
  }
  if (WlanBssList != NULL)
  {
    WlanFreeMemory(WlanBssList);
    WlanBssList = NULL;
  }
  return Napi::String::New(env, "getList");
}

Napi::Value GetInfo(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (!initflag)
  {
    return Napi::String::New(env, "You should call init() first");
  }
  napi_status status;
  napi_value ifaceList;
  PWLAN_INTERFACE_INFO_LIST plist = NULL;
  PWLAN_INTERFACE_INFO pInfo = NULL;
  PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
  WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
  DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
  if (WlanEnumInterfaces(hClient, NULL, &plist) == ERROR_SUCCESS)
  {
    status = napi_create_array(env, &ifaceList);

    for (int i = 0; i < (int)plist->dwNumberOfItems; i++)
    {
      napi_value iface, guid, description, connection;
      pInfo = (WLAN_INTERFACE_INFO *)&plist->InterfaceInfo[i];
      WCHAR GuidString[39] = {0};
      if (StringFromGUID2(pInfo->InterfaceGuid, (LPOLESTR)&GuidString, sizeof(GuidString) / sizeof(*GuidString)) != 0)
      {
        _bstr_t b(GuidString);
        char *guidStr = b;
        status = napi_create_string_utf8(env, guidStr, 38, &guid);

        status = napi_create_object(env, &iface);

        status = napi_set_named_property(env, iface, "guid", guid);
      }
      else
      {
        const char *error = "GetGuidtoStringFailed";
        status = napi_create_string_utf8(env, error, 22, &ifaceList);
        goto end;
      }
      _bstr_t b_desc(pInfo->strInterfaceDescription);
      status = napi_create_string_utf8(env, b_desc, b_desc.length(), &description);

      status = napi_set_named_property(env, iface, "description", description);

      switch (pInfo->isState)
      {
      case wlan_interface_state_not_ready:
        status = napi_create_string_utf8(env, "not ready", 9, &connection);
        break;
      case wlan_interface_state_connected:
        status = napi_create_string_utf8(env, "connected", 9, &connection);
        break;
      case wlan_interface_state_ad_hoc_network_formed:
        status = napi_create_string_utf8(env, "ad_hoc", 6, &connection);
        break;
      case wlan_interface_state_disconnecting:
        status = napi_create_string_utf8(env, "disconnecting", 13, &connection);
        break;
      case wlan_interface_state_disconnected:
        status = napi_create_string_utf8(env, "disconnected", 12, &connection);
        break;
      case wlan_interface_state_associating:
        status = napi_create_string_utf8(env, "connecting", 10, &connection);
        break;
      case wlan_interface_state_discovering:
        status = napi_create_string_utf8(env, "discovering", 11, &connection);
        break;
      case wlan_interface_state_authenticating:
        status = napi_create_string_utf8(env, "authenticating", 14, &connection);
        break;
      default:
        status = napi_create_string_utf8(env, "unknown", 7, &connection);
        break;
      }

      status = napi_set_named_property(env, iface, "connection", connection);

      if (pInfo->isState == wlan_interface_state_connected)
      {
        if (WlanQueryInterface(hClient, &pInfo->InterfaceGuid, wlan_intf_opcode_current_connection, NULL, &connectInfoSize, (PVOID *)&pConnectInfo, &opCode) == ERROR_SUCCESS)
        {
          napi_value mode, profile_name, ssid, phys_typ, phys_addr;
          switch (pConnectInfo->wlanConnectionMode)
          {
          case wlan_connection_mode_profile:
            status = napi_create_string_utf8(env, "profile", 7, &mode);
            break;
          case wlan_connection_mode_temporary_profile:
            status = napi_create_string_utf8(env, "temporary_profile", 17, &mode);
            break;
          case wlan_connection_mode_discovery_secure:
            status = napi_create_string_utf8(env, "secure_discovery", 16, &mode);
            break;
          case wlan_connection_mode_discovery_unsecure:
            status = napi_create_string_utf8(env, "unsecure_discovery", 18, &mode);
            break;
          case wlan_connection_mode_auto:
            status = napi_create_string_utf8(env, "persistent_profile", 18, &mode);
            break;
          case wlan_connection_mode_invalid:
            status = napi_create_string_utf8(env, "invalid", 7, &mode);
            break;
          default:
            status = napi_create_string_utf8(env, "unknown", 7, &mode);
            break;
          }

          status = napi_set_named_property(env, iface, "mode", mode);

          _bstr_t profileName(pConnectInfo->strProfileName);
          status = napi_create_string_utf8(env, profileName, profileName.length(), &profile_name);

          status = napi_set_named_property(env, iface, "profile_name", profile_name);

          // TODO: get ssid, mac, phy_type, bssid_type, singal level...
          //       refer: https://docs.microsoft.com/en-us/windows/win32/api/wlanapi/nf-wlanapi-wlanqueryinterface

          // SSID
          if (pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength != 0)
          {
            status = napi_create_string_utf8(env, (char *)pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID, (size_t)pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength, &ssid);

            status = napi_set_named_property(env, iface, "ssid", ssid);
          }

          // physical type
          switch (pConnectInfo->wlanAssociationAttributes.dot11BssType)
          {
          case dot11_BSS_type_infrastructure:
            status = napi_create_string_utf8(env, "infrastructure", 14, &phys_typ);
            break;
          case dot11_BSS_type_independent:
            status = napi_create_string_utf8(env, "independent", 11, &phys_typ);
            break;
          case dot11_BSS_type_any:
            status = napi_create_string_utf8(env, "others", 6, &phys_typ);
            break;
          }

          status = napi_set_named_property(env, iface, "bssid_type", phys_typ);

          // MAC
          char mac[17];
          sprintf(mac, "%02x-%02x-%02x-%02x-%02x-%02x",
                  pConnectInfo->wlanAssociationAttributes.dot11Bssid[0],
                  pConnectInfo->wlanAssociationAttributes.dot11Bssid[1],
                  pConnectInfo->wlanAssociationAttributes.dot11Bssid[2],
                  pConnectInfo->wlanAssociationAttributes.dot11Bssid[3],
                  pConnectInfo->wlanAssociationAttributes.dot11Bssid[4],
                  pConnectInfo->wlanAssociationAttributes.dot11Bssid[5]);
          status = napi_create_string_utf8(env, mac, 17, &phys_addr);

          status = napi_set_named_property(env, iface, "AP_MAC", phys_addr);
        }
        else
        {
          const char *error = "WlanQueryInterface failed";
          napi_create_string_utf8(env, error, 25, &ifaceList);
          goto end;
        }
      }
      status = napi_set_element(env, ifaceList, i, iface);
    }
  }
  else
  {
    const char *error = "WlanEnumInterfaces failed";
    napi_create_string_utf8(env, error, 25, &ifaceList);
    goto end;
  }

end:
  Napi::Function cb = info[0].As<Napi::Function>();
  cb.Call(env.Global(), {ifaceList});
  return Napi::String::New(env, "get info");
}

Napi::Value Connect(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  char GuidString[40] = {0};
  WCHAR GuidWString[40] = {0};
  DWORD dwResult;
  DWORD profileReasonCode;
  WLAN_CONNECTION_PARAMETERS connectionParams;
  GUID guid_for_wlan;
  char *profile = NULL;
  char *ssid = NULL;
  WCHAR *profileName = NULL;
  WCHAR *Wprofile = NULL;
  DOT11_SSID ap_ssid;

  if (!initflag)
  {
    goto end;
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

    // set wlan connection parameters for WlanConnect()
    printf("Set wlan_connection_params... ");
    connectionParams.pDesiredBssidList = NULL;
    connectionParams.strProfile = profileName;
    connectionParams.dwFlags = 0;
    connectionParams.dot11BssType = dot11_BSS_type_infrastructure;
    connectionParams.wlanConnectionMode = wlan_connection_mode_profile;
    connectionParams.pDot11Ssid = &ap_ssid;
    printf("ok\n");

    // set profile
    printf("WlanSetProfile... ");
    dwResult = WlanSetProfile(hClient, &guid_for_wlan, 0, Wprofile, NULL, TRUE, NULL, &profileReasonCode);
    if (dwResult != ERROR_SUCCESS)
    {
      wprintf(L"WlanSetProfile failed with error: %u\n", dwResult);
      if (dwResult == ERROR_BAD_PROFILE)
      {
        WCHAR reasonStr[256];
        dwResult = WlanReasonCodeToString(profileReasonCode, 256, reasonStr, NULL);
        if (dwResult == ERROR_SUCCESS)
        {
          wprintf(L"why: %ls\n", reasonStr);
        }
      }
    }
    printf("ok\n");

    //connect to ap
    printf("wlanconnect... ");
    callbackInfo.callbackReason = 8;
    dwResult = WlanConnect(hClient, &guid_for_wlan, &connectionParams, NULL);
    if (dwResult != ERROR_SUCCESS)
    {
      wprintf(L"WlanConnect failed with error: %u\n", dwResult);
      Napi::Function cb = info[3].As<Napi::Function>();
      cb.Call(env.Global(), {Napi::Number::New(env, 0)});
    }
    else
    {
      Napi::Function cb = info[3].As<Napi::Function>();
      cb.Call(env.Global(), {Napi::Number::New(env, 1)});
    }
  }

end:
  //FIXME: FREE MEMORY
  printf("free memory...");
  free_memory(profile);
  free_memory(Wprofile);
  free_memory(ssid);
  free_memory(profileName);
  printf("ok\n");
  return Napi::String::New(env, "connect");
}

Napi::Value Disconnect(const Napi::CallbackInfo &info)
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
  dwResult = WlanDisconnect(hClient, &guid_for_wlan, NULL);
  if (dwResult == ERROR_SUCCESS)
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

Napi::Value Wlanfree(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (hClient != NULL)
  {
    WlanCloseHandle(hClient, NULL);
    hClient = NULL;
  }
  return Napi::String::New(env, "free");
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "wlanListener"), Napi::Function::New(env, callEvents));
  exports.Set(Napi::String::New(env, "wlanInit"), Napi::Function::New(env, WlanInit));
  exports.Set(Napi::String::New(env, "wlanScan"), Napi::Function::New(env, Scan));
  exports.Set(Napi::String::New(env, "wlanFree"), Napi::Function::New(env, Wlanfree));
  exports.Set(Napi::String::New(env, "wlanConnect"), Napi::Function::New(env, Connect));
  exports.Set(Napi::String::New(env, "wlanGetNetworkList"), Napi::Function::New(env, GetNetworkList));
  exports.Set(Napi::String::New(env, "wlanDisconnect"), Napi::Function::New(env, Disconnect));
  exports.Set(Napi::String::New(env, "wlanGetIfaceInfo"), Napi::Function::New(env, GetInfo));
  return exports;
}
NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init);
