#ifndef UNICODE
#define UNICODE
#endif

#include <assert.h>
#include <node_api.h>
#include <Windows.h>
#include <wlanapi.h>
#include <windot11.h>
#include <stdio.h>
#include <stdlib.h>
#include <comdef.h>

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
    SetEvent(callbackInfo->handleEvent);
  }
  if ((WlanNotificationData->NotificationCode == wlan_notification_acm_connection_complete) ||
      (WlanNotificationData->NotificationCode == wlan_notification_acm_connection_attempt_fail))
  {
    callbackInfo->callbackReason = WlanNotificationData->NotificationCode;
    SetEvent(callbackInfo->handleEvent);
  }
  return;
}

napi_value WlanInit(napi_env env, napi_callback_info info)
{
  napi_status status;
  DWORD dwResult = 0;
  DWORD dwMaxClient = 2;
  DWORD dwCurVersion = 0;
  dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
  if (dwResult != ERROR_SUCCESS)
  {
    wprintf(L"WlanOpenHandle failed with error: %u\n", dwResult);
  }
  dwResult = WlanRegisterNotification(hClient, WLAN_NOTIFICATION_SOURCE_ALL, TRUE, (WLAN_NOTIFICATION_CALLBACK)wlanCallback, (PVOID)&callbackInfo, NULL, NULL);
  if (dwResult != ERROR_SUCCESS)
  {
    wprintf(L"WlanRegisterNotification failed with error: %u\n", dwResult);
  }
  initflag = TRUE;
  return nullptr;
}

napi_value Scan(napi_env env, napi_callback_info info)
{
  if (!initflag)
  {
    napi_value uninit;
    napi_create_string_utf8(env, "You should call init() first", 30, &uninit);
    return uninit;
  }
  napi_status status;
  napi_value scan_result_arr = NULL;
  status = napi_create_array(env, &scan_result_arr);
  DWORD dwResult = 0;
  DWORD dwRetVal = 0;
  GUID interfaceGuid;
  DWORD dwPrevNotifType = 0;
  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  assert(status == napi_ok);
  napi_value cb = args[0];
  napi_value argv[1];
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
    DWORD waitResult = WaitForSingleObject(callbackInfo.handleEvent, 15000);
    if (waitResult == WAIT_OBJECT_0)
    {
      if (callbackInfo.callbackReason == wlan_notification_acm_scan_complete)
      {
        printf("ok\n");
        for (ifaceNum = 0; ifaceNum < (int)pIfList->dwNumberOfItems; ifaceNum++)
        {
          pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[ifaceNum];
          printf("WlanShowNetworksList... ");
          if (WlanGetNetworkBssList(hClient, &pIfInfo->InterfaceGuid, pDotSSid, dot11_BSS_type_infrastructure, FALSE, NULL, &WlanBssList) == ERROR_SUCCESS)
          {
            printf("ok\n");
            uint32_t correct_counter = 0;
            for (int c = 0; c < WlanBssList->dwNumberOfItems; c++)
            {
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
      }
    }
    assert(status == napi_ok);
    CloseHandle(callbackInfo.handleEvent);
    argv[0] = scan_result_arr;
    assert(status == napi_ok);

    napi_value global;
    status = napi_get_global(env, &global);
    assert(status == napi_ok);

    napi_value result;
    status = napi_call_function(env, global, cb, 1, argv, &result);
    assert(status == napi_ok);
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
  return nullptr;
}

napi_value GetInfo(napi_env env, napi_callback_info info)
{
  if (!initflag)
  {
    napi_value uninit;
    napi_create_string_utf8(env, "You should call init() first", 30, &uninit);
    return uninit;
  }
  napi_status status;
  napi_value argv[1];
  napi_value result;
  size_t argc = 1;
  PWLAN_INTERFACE_INFO_LIST plist = NULL;
  PWLAN_INTERFACE_INFO pInfo = NULL;
  PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
  WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
  DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
  status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  assert(status == napi_ok);
  if (WlanEnumInterfaces(hClient, NULL, &plist) == ERROR_SUCCESS)
  {
    for (int i = 0; i < (int)plist->dwNumberOfItems; i++)
    {
      napi_value iface, guid, description, connection;
      pInfo = (WLAN_INTERFACE_INFO *)&plist->InterfaceInfo[i];
      WCHAR GuidString[39] = {0};
      if (StringFromGUID2(pInfo->InterfaceGuid, (LPOLESTR)&GuidString, sizeof(GuidString) / sizeof(*GuidString)) != 0)
      {
        _bstr_t b(GuidString);
        char *guidStr = b;
        status = napi_create_string_utf8(env, guidStr, 39, &guid);
        assert(status == napi_ok);
        status = napi_create_object(env, &iface);
        assert(status == napi_ok);
        status = napi_set_named_property(env, iface, "guid", guid);
      }
      else
      {
        const char *error = "GetGuidtoStringFailed";
        status = napi_create_string_utf8(env, error, 22, &result);
        goto end;
      }
      _bstr_t b_desc(pInfo->strInterfaceDescription);
      status = napi_create_string_utf8(env, b_desc, 256, &description);
      assert(status == napi_ok);
      status = napi_set_named_property(env, iface, "description", description);
      assert(status == napi_ok);
      switch (pInfo->isState)
      {
      case wlan_interface_state_not_ready:
        status = napi_create_string_utf8(env, "not ready", 10, &connection);
        break;
      case wlan_interface_state_connected:
        status = napi_create_string_utf8(env, "connected", 10, &connection);
        break;
      case wlan_interface_state_ad_hoc_network_formed:
        status = napi_create_string_utf8(env, "ad_hoc", 7, &connection);
        break;
      case wlan_interface_state_disconnecting:
        status = napi_create_string_utf8(env, "disconnecting", 14, &connection);
        break;
      case wlan_interface_state_disconnected:
        status = napi_create_string_utf8(env, "disconnected", 13, &connection);
        break;
      case wlan_interface_state_associating:
        status = napi_create_string_utf8(env, "connecting", 11, &connection);
        break;
      case wlan_interface_state_discovering:
        status = napi_create_string_utf8(env, "discovering", 12, &connection);
        break;
      case wlan_interface_state_authenticating:
        status = napi_create_string_utf8(env, "authenticating", 15, &connection);
        break;
      default:
        status = napi_create_string_utf8(env, "unknown", 8, &connection);
        break;
      }
      assert(status == napi_ok);
      status = napi_set_named_property(env, iface, "connection", connection);
      assert(status == napi_ok);

      if (pInfo->isState == wlan_interface_state_connected)
      {
        if (WlanQueryInterface(hClient, &pInfo->InterfaceGuid, wlan_intf_opcode_current_connection, NULL, &connectInfoSize, (PVOID *)&pConnectInfo, &opCode) == ERROR_SUCCESS)
        {
          napi_value mode, profile_name;
          switch (pConnectInfo->wlanConnectionMode)
          {
          case wlan_connection_mode_profile:
            status = napi_create_string_utf8(env, "profile", 8, &mode);
            break;
          case wlan_connection_mode_temporary_profile:
            status = napi_create_string_utf8(env, "temporary_profile", 18, &mode);
            break;
          case wlan_connection_mode_discovery_secure:
            status = napi_create_string_utf8(env, "secure_discovery", 17, &mode);
            break;
          case wlan_connection_mode_discovery_unsecure:
            status = napi_create_string_utf8(env, "unsecure_discovery", 19, &mode);
            break;
          case wlan_connection_mode_auto:
            status = napi_create_string_utf8(env, "persistent_profile", 19, &mode);
            break;
          case wlan_connection_mode_invalid:
            status = napi_create_string_utf8(env, "invalid", 8, &mode);
            break;
          default:
            status = napi_create_string_utf8(env, "unknown", 8, &mode);
            break;
          }
          assert(status == napi_ok);
          status = napi_set_named_property(env, iface, "mode", mode);
          assert(status == napi_ok);
          _bstr_t profileName(pConnectInfo->strProfileName);
          status = napi_create_string_utf8(env, profileName, 256, &profile_name);
          assert(status == napi_ok);
          status = napi_set_named_property(env, iface, "profile_name", profile_name);
          assert(status == napi_ok);
          // TODO: get ssid, mac, phy_type, bssid_type, singal level... 
          //       refer: https://docs.microsoft.com/en-us/windows/win32/api/wlanapi/nf-wlanapi-wlanqueryinterface 
          
        }
        else
        {
          const char *error = "WlanQueryInterface failed";
          napi_create_string_utf8(env, error, 26, &result);
          goto end;
        }
      }
    }
  }
  else
  {
    const char *error = "WlanEnumInterfaces failed";
    napi_create_string_utf8(env, error, 26, &result);
  }

end:
  napi_value cb = argv[0];
  napi_value global;
  status = napi_get_global(env, &global);
  assert(status == napi_ok);
  napi_value result;
  status = napi_call_function(env, global, cb, 1, argv, &result);
}

napi_value Connect(napi_env env, napi_callback_info info)
{
  napi_status status;
  napi_value args[4];
  napi_value argv[1];
  unsigned int connectionFlag = 0;
  size_t argc = 4;
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

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  assert(status == napi_ok);
  if (argc < 4)
  {
    napi_throw_type_error(env, nullptr, "Wrong number of arguments");
    return nullptr;
  }
  else
  {
    wprintf(L"Get arguments\n");
  }

  napi_valuetype valuetype0;
  status = napi_typeof(env, args[0], &valuetype0);
  assert(status == napi_ok);

  napi_valuetype valuetype1;
  status = napi_typeof(env, args[1], &valuetype1);
  assert(status == napi_ok);

  napi_valuetype valuetype2;
  status = napi_typeof(env, args[2], &valuetype2);
  assert(status == napi_ok);

  napi_valuetype valuetype3;
  status = napi_typeof(env, args[3], &valuetype3);
  assert(status == napi_ok);

  if (valuetype0 != napi_string ||
      valuetype1 != napi_string ||
      valuetype2 != napi_string ||
      valuetype3 != napi_function)
  {
    napi_throw_type_error(env, nullptr, "Wrong arguments");
    return nullptr;
  }
  else
  {
    wprintf(L"arguments type are all correct\n");
  }

  if (!initflag)
  {
    goto end;
  }
  // get guid
  printf("Copy GUID... ");
  size_t strSize = 40;
  status = napi_get_value_string_utf8(env, args[0], GuidString, strSize, &strSize);
  assert(status == napi_ok);
  mbstowcs(GuidWString, GuidString, 40);
  dwResult = CLSIDFromString(GuidWString, &guid_for_wlan);
  if (dwResult != NOERROR)
  {
    wprintf(L"CLSIDFromString failed with error: %u\n", dwResult);
  }
  printf("ok\n");

  // get profile content
  printf("Copy profile contents... ");
  size_t profileLen;
  status = napi_get_value_string_utf8(env, args[1], NULL, NULL, &profileLen);
  assert(status == napi_ok);
  profile = (char *)malloc(sizeof(char) * profileLen);
  status = napi_get_value_string_utf8(env, args[1], profile, profileLen + 1, &profileLen);
  assert(status == napi_ok);
  printf("ok\n");

  // get ssid
  printf("Copy profileName... ");
  size_t ssidLen;
  status = napi_get_value_string_utf8(env, args[2], NULL, NULL, &ssidLen);
  assert(status == napi_ok);
  ssid = (char *)malloc(sizeof(char) * ssidLen);
  status = napi_get_value_string_utf8(env, args[2], ssid, ssidLen + 1, &ssidLen);
  assert(status == napi_ok);
  profileName = (WCHAR *)malloc(sizeof(WCHAR) * ssidLen);
  mbstowcs(profileName, ssid, ssidLen + 1);
  Wprofile = (WCHAR *)malloc(sizeof(WCHAR) * profileLen);
  mbstowcs(Wprofile, profile, profileLen + 1);
  printf("ok\n");

  printf("Copy ssid... ");
  ap_ssid.uSSIDLength = (ULONG)ssidLen;
  memcpy(ap_ssid.ucSSID, (UCHAR *)ssid, ssidLen);
  printf("ok\n");

  printf("Set wlan_connection_params... ");
  connectionParams.pDesiredBssidList = NULL;
  connectionParams.strProfile = profileName;
  connectionParams.dwFlags = 0;
  connectionParams.dot11BssType = dot11_BSS_type_infrastructure;
  connectionParams.wlanConnectionMode = wlan_connection_mode_profile;
  connectionParams.pDot11Ssid = &ap_ssid;
  printf("ok\n");

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

  callbackInfo.interfaceGUID = guid_for_wlan;
  callbackInfo.handleEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  printf("wlanconnect... ");
  dwResult = WlanConnect(hClient, &guid_for_wlan, &connectionParams, NULL);
  if (dwResult != ERROR_SUCCESS)
  {
    wprintf(L"WlanConnect failed with error: %u\n", dwResult);
  }

  DWORD waitConnectesult = WaitForSingleObject(callbackInfo.handleEvent, 15000);
  if (waitConnectesult == WAIT_OBJECT_0)
  {
    if (callbackInfo.callbackReason == wlan_notification_acm_connection_complete)
    {
      printf("ok\n");
      wprintf(L"wlan_notification_acm_connection_complete\n");
      connectionFlag = 1;
    }
    else if (callbackInfo.callbackReason == wlan_notification_acm_connection_attempt_fail)
    {
      wprintf(L"wlan_notification_acm_connection_attempt_fail\n");
    }
  }
end:
  //FIXME: FREE MEMORY
  if (connectionFlag == 1)
  { // Successful
    status = napi_create_int32(env, 1, argv);
  }
  else
  { // Failed
    status = napi_create_int32(env, 0, argv);
  }
  if (callbackInfo.handleEvent)
  {
    CloseHandle(callbackInfo.handleEvent);
  }
  printf("free memory...");
  free_memory(profile);
  free_memory(Wprofile);
  free_memory(ssid);
  free_memory(profileName);
  printf("ok\n");
  assert(status == napi_ok);
  napi_value cb = args[3];
  napi_value global;
  status = napi_get_global(env, &global);
  assert(status == napi_ok);
  napi_value result;
  status = napi_call_function(env, global, cb, 1, argv, &result);
  assert(status == napi_ok);
  return nullptr;
}

napi_value Wlanfree(napi_env env, napi_callback_info info)
{
  if (hClient != NULL)
  {
    WlanCloseHandle(hClient, NULL);
    hClient = NULL;
  }
  return nullptr;
}

#define DECLARE_NAPI_METHOD(name, func)     \
  {                                         \
    name, 0, func, 0, 0, 0, napi_default, 0 \
  }

napi_value Init(napi_env env, napi_value exports)
{
  napi_status status;
  napi_property_descriptor scanDesc = DECLARE_NAPI_METHOD("wlanScan", Scan);
  napi_property_descriptor initDesc = DECLARE_NAPI_METHOD("wlanInit", WlanInit);
  napi_property_descriptor freeDesc = DECLARE_NAPI_METHOD("wlanFree", Wlanfree);
  napi_property_descriptor connectDesc = DECLARE_NAPI_METHOD("wlanConnect", Connect);
  status = napi_define_properties(env, exports, 1, &scanDesc);
  status = napi_define_properties(env, exports, 1, &initDesc);
  status = napi_define_properties(env, exports, 1, &freeDesc);
  status = napi_define_properties(env, exports, 1, &connectDesc);
  assert(status == napi_ok);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
