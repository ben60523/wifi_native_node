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

struct ScanResult
{
  UCHAR *ssid;
  long iRssi;
};

napi_value Scan(napi_env env, napi_callback_info info)
{
  napi_status status;
  napi_value scan_result_arr = NULL;
  status = napi_create_array(env, &scan_result_arr);
  DWORD dwMaxClient = 2;
  DWORD dwCurVersion = 0;
  DWORD dwResult = 0;
  DWORD dwRetVal = 0;
  GUID interfaceGuid;
  DWORD dwPrevNotifType = 0;
  PWLAN_BSS_LIST WlanBssList;
  HANDLE hClient = NULL;
  PWLAN_INTERFACE_INFO pIfInfo = NULL;
  PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
  PDOT11_SSID pDotSSid = NULL;
  unsigned int ifaceNum = 0;

  dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
  if (dwResult != ERROR_SUCCESS)
  {
    wprintf(L"WlanOpenHandle failed with error: %u\n", dwResult);
  }
  dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
  if (dwResult != ERROR_SUCCESS)
  {
    wprintf(L"WlanEnumInterfaces failed with error: %u\n", dwResult);
  }
  else
  {
    wprintf(L"Num Entries: %lu\n", pIfList->dwNumberOfItems);
    wprintf(L"Current Index: %lu\n", pIfList->dwIndex);
    for (ifaceNum = 0; ifaceNum < (int)pIfList->dwNumberOfItems; ifaceNum++)
    {
      pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[ifaceNum];
      WLAN_RAW_DATA WlanRawData;
      wprintf(L"%ls Scanning...\n", pIfInfo->strInterfaceDescription);
      dwResult = WlanScan(hClient, &pIfInfo->InterfaceGuid, pDotSSid, &WlanRawData, NULL);
      if (dwResult != ERROR_SUCCESS)
      {
        wprintf(L"WlanScan failed with error: %u\n", dwResult);
      }
    }
    Sleep(4000);
    for (ifaceNum = 0; ifaceNum < (int)pIfList->dwNumberOfItems; ifaceNum++)
    {
      pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[ifaceNum];
      PWLAN_BSS_LIST WlanBssList;
      if (WlanGetNetworkBssList(hClient, &pIfInfo->InterfaceGuid, pDotSSid, dot11_BSS_type_independent, FALSE, NULL, &WlanBssList) == ERROR_SUCCESS)
      {
        WCHAR GuidString[40] = {0};
        uint32_t correct_counter = 0;
        StringFromGUID2(pIfInfo->InterfaceGuid, (LPOLESTR)&GuidString, 39);
        wprintf(L"===========%ws=============\n", GuidString);
        for (int c = 0; c < WlanBssList->dwNumberOfItems; c++)
        {
          wprintf(L"SSID: %hs\n", WlanBssList->wlanBssEntries[c].dot11Ssid.ucSSID);
          wprintf(L"MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  WlanBssList->wlanBssEntries[c].dot11Bssid[0],
                  WlanBssList->wlanBssEntries[c].dot11Bssid[1],
                  WlanBssList->wlanBssEntries[c].dot11Bssid[2],
                  WlanBssList->wlanBssEntries[c].dot11Bssid[3],
                  WlanBssList->wlanBssEntries[c].dot11Bssid[4],
                  WlanBssList->wlanBssEntries[c].dot11Bssid[5]);
          wprintf(L"RSSI: %ld\n", WlanBssList->wlanBssEntries[c].lRssi);
          wprintf(L"---------------------------\n");
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
    assert(status == napi_ok);
    return scan_result_arr;
  }
}

napi_value Method(napi_env env, napi_callback_info info)
{
  napi_status status;
  napi_value world;
  status = napi_create_string_utf8(env, "world", 5, &world);
  assert(status == napi_ok);
  return world;
}

#define DECLARE_NAPI_METHOD(name, func)     \
  {                                         \
    name, 0, func, 0, 0, 0, napi_default, 0 \
  }

napi_value Init(napi_env env, napi_value exports)
{
  napi_status status;
  napi_property_descriptor desc = DECLARE_NAPI_METHOD("hello", Method);
  napi_property_descriptor Scandesc = DECLARE_NAPI_METHOD("wifiscan", Scan);
  status = napi_define_properties(env, exports, 1, &desc);
  status = napi_define_properties(env, exports, 1, &Scandesc);
  assert(status == napi_ok);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
