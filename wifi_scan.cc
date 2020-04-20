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
PWLAN_BSS_LIST WlanBssList;
PWLAN_INTERFACE_INFO pIfInfo = NULL;
PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
PDOT11_SSID pDotSSid = NULL;

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
  if ((WlanNotificationData->NotificationCode == wlan_notification_acm_scan_complete) ||
      (WlanNotificationData->NotificationCode == wlan_notification_acm_scan_fail))
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
DWORD StringWToSsid(__in LPCWSTR strSsid, __out PDOT11_SSID pSsid)
{
  DWORD dwRetCode = ERROR_SUCCESS;
  BYTE pbSsid[DOT11_SSID_MAX_LENGTH + 1] = {0};
  if (strSsid == NULL || pSsid == NULL)
  {
    dwRetCode = ERROR_INVALID_PARAMETER;
  }
  else
  {
    pSsid->uSSIDLength = WideCharToMultiByte(CP_UTF8,
                                             0,
                                             strSsid,
                                             -1,
                                             (LPSTR)pbSsid,
                                             sizeof(pbSsid),
                                             NULL,
                                             NULL);
    pSsid->uSSIDLength--;
    memcpy(&pSsid->ucSSID, pbSsid, pSsid->uSSIDLength);
  }
  return dwRetCode;
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
  return nullptr;
}

napi_value Scan(napi_env env, napi_callback_info info)
{
  if (hClient == NULL)
  {
    napi_throw_error(env, nullptr, "Error: you should call init() first...\n");
    return nullptr;
  }
  napi_status status;
  napi_value scan_result_arr = NULL;
  status = napi_create_array(env, &scan_result_arr);
  DWORD dwResult = 0;
  DWORD dwRetVal = 0;
  GUID interfaceGuid;
  DWORD dwPrevNotifType = 0;

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
      callbackInfo.handleEvent = CreateEvent(
          nullptr,
          FALSE,
          FALSE,
          nullptr);
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
        for (ifaceNum = 0; ifaceNum < (int)pIfList->dwNumberOfItems; ifaceNum++)
        {
          pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[ifaceNum];
          if (WlanGetNetworkBssList(hClient, &pIfInfo->InterfaceGuid, pDotSSid, dot11_BSS_type_independent, FALSE, NULL, &WlanBssList) == ERROR_SUCCESS)
          {
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
    return scan_result_arr;
  }
}

napi_value Connect(napi_env env, napi_callback_info info)
{
  napi_status status;
  napi_value args[4];
  napi_value argv[1];
  size_t argc = 4;
  char GuidString[40] = {0};
  WCHAR GuidWString[40] = {0};
  DWORD dwResult;
  DWORD profileReasonCode;
  WLAN_CONNECTION_PARAMETERS connectionParams;
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

  if (valuetype0 != napi_string || valuetype1 != napi_string || valuetype2 != napi_string || valuetype3 != napi_function)
  {
    napi_throw_type_error(env, nullptr, "Wrong arguments");
    return nullptr;
  }
  else
  {
    wprintf(L"arguments type are all correct\n");
  }
  // get guid
  size_t strSize = 40;
  status = napi_get_value_string_utf8(env, args[0], GuidString, strSize, &strSize);
  assert(status == napi_ok);
  GUID guid_for_wlan;
  mbstowcs(GuidWString, GuidString, 40);
  wprintf(L"GuidWstring: %ls\n", GuidWString);
  dwResult = CLSIDFromString(GuidWString, &guid_for_wlan);
  if (dwResult != NOERROR) {
    wprintf(L"CLSIDFromString failed with error: %u\n", dwResult);
  }
  printf("InterfaceGUID: %s\n", GuidString);
  WCHAR GuidString2[40] = {0};
  StringFromGUID2(guid_for_wlan, (LPOLESTR)&GuidString2, 39);
  wprintf(L"===========%ws=============\n", GuidString2);

  // get profile
  size_t profileLen;
  status = napi_get_value_string_utf8(env, args[1], NULL, NULL, &profileLen);
  assert(status == napi_ok);
  char *profile = (char *)malloc(sizeof(char) * profileLen);
  status = napi_get_value_string_utf8(env, args[1], profile, profileLen + 1, &profileLen);
  assert(status == napi_ok);
  printf("%s\n", profile);

  // get ssid
  size_t ssidLen;
  status = napi_get_value_string_utf8(env, args[2], NULL, NULL, &ssidLen);
  assert(status == napi_ok);
  char *ssid = (char *)malloc(sizeof(char) * ssidLen);
  status = napi_get_value_string_utf8(env, args[2], ssid, ssidLen + 1, &ssidLen);
  assert(status == napi_ok);
  printf("%s\n", ssid);

  napi_value cb = args[3];

  connectionParams.pDesiredBssidList = NULL;
  WCHAR *Wprofile = (WCHAR *)malloc(sizeof(WCHAR) * profileLen);
  mbstowcs(Wprofile, profile, profileLen + 1);
  connectionParams.strProfile = Wprofile;
  wprintf(L"Wprofile: %ls\n", Wprofile);
  connectionParams.dwFlags = 0;
  connectionParams.dot11BssType = dot11_BSS_type_any;
  connectionParams.wlanConnectionMode = wlan_connection_mode_profile;

  PDOT11_SSID ap_ssid;
  if (!WlanBssList)
  {
    wprintf(L"need to scan first\n");
    goto end;
  }
  for (int i = 0; i < WlanBssList->dwNumberOfItems; i++)
  {
    if (strstr(ssid, (char *)WlanBssList->wlanBssEntries[i].dot11Ssid.ucSSID))
    {
      ap_ssid = &WlanBssList->wlanBssEntries[i].dot11Ssid;
      wprintf(L"found ap\n");
      break;
    }
  }
  wprintf(L"SSID: %hs\n", ap_ssid->ucSSID);
  connectionParams.pDot11Ssid = ap_ssid;

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
    napi_create_int32(env, 0, argv);
    goto end;
  }
  else
  {
    wprintf(L"preparing to connect\n");
    callbackInfo.interfaceGUID = guid_for_wlan;
    callbackInfo.handleEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    dwResult = WlanConnect(hClient, &guid_for_wlan, &connectionParams, NULL);
    if (dwResult != ERROR_SUCCESS)
    {
      wprintf(L"WlanConnect failed with error: %u\n", dwResult);
      napi_create_int32(env, 0, argv);
      goto end;
    }

    DWORD waitResult = WaitForSingleObject(callbackInfo.handleEvent, 15000);
    if (waitResult == WAIT_OBJECT_0)
    {
      if (callbackInfo.callbackReason == wlan_notification_acm_connection_complete)
      {
        napi_create_int32(env, 1, argv);
      }
      else if (callbackInfo.callbackReason == wlan_notification_acm_connection_attempt_fail)
      {
        napi_create_int32(env, 0, argv);
      }
    }
  }

end:
  napi_value global;
  status = napi_get_global(env, &global);
  assert(status == napi_ok);
  napi_value result;
  free(Wprofile);
  free(profile);
  free(ssid);
  status = napi_call_function(env, global, cb, 1, argv, &result);
  assert(status == napi_ok);
  return nullptr;
}

napi_value Wlanfree(napi_env env, napi_callback_info info)
{
  if (pIfList != NULL)
  {
    WlanFreeMemory(pIfList);
  }
  if (pDotSSid != NULL)
  {
    WlanFreeMemory(pDotSSid);
  }
  if (WlanBssList != NULL)
  {
    WlanFreeMemory(WlanBssList);
  }
  if (hClient != NULL)
  {
    WlanCloseHandle(hClient, NULL);
  }
  return nullptr;
}

napi_value getScanCB(napi_env env, napi_callback_info info)
{
  size_t argc = 1;
  napi_status status;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  assert(status == napi_ok);

  napi_value cb = args[0];

  napi_value argv[1];
  argv[0] = Scan(env, info);
  assert(status == napi_ok);

  napi_value global;
  status = napi_get_global(env, &global);
  assert(status == napi_ok);

  napi_value result;
  status = napi_call_function(env, global, cb, 1, argv, &result);
  assert(status == napi_ok);

  return nullptr;
}

napi_value getConnectCB(napi_env env, napi_callback_info info)
{
  size_t argc = 1;
  napi_status status;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  assert(status == napi_ok);

  napi_value cb = args[0];

  napi_value argv[1];
  argv[0] = Connect(env, info);
  assert(status == napi_ok);

  napi_value global;
  status = napi_get_global(env, &global);
  assert(status == napi_ok);

  napi_value result;
  status = napi_call_function(env, global, cb, 1, argv, &result);
  assert(status == napi_ok);

  return nullptr;
}

#define DECLARE_NAPI_METHOD(name, func)     \
  {                                         \
    name, 0, func, 0, 0, 0, napi_default, 0 \
  }

napi_value Init(napi_env env, napi_value exports)
{
  napi_status status;
  napi_property_descriptor scanDesc = DECLARE_NAPI_METHOD("wlanScan", getScanCB);
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
