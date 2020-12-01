#include "WlanApiClass.h"
#include <iostream>
#include <wlanapi.h>
#include <stdio.h>
#include <thread>
#include <functional>
#include <vector>
#include <future>
#include <comdef.h>

WlanApiClass::WlanApiClass()
{
    callbackInfo = {0};
    hClient = NULL;
    is_scan_finish = FALSE;
    // initflag = TRUE;
}

WlanApiClass::~WlanApiClass()
{
    if (hClient != NULL)
    {
        WlanCloseHandle(hClient, NULL);
        hClient = NULL;
    }
}

HRESULT WlanApiClass::init()
{
    DWORD dwResult = 0;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
    if (dwResult != ERROR_SUCCESS)
    {
        printf("WlanOpenHandle failed with error: %u\n", dwResult);
        return S_FALSE;
    }
    dwResult = WlanRegisterNotification(hClient, WLAN_NOTIFICATION_SOURCE_ALL, TRUE, (WLAN_NOTIFICATION_CALLBACK)wlanCallback, (PVOID)&callbackInfo, NULL, NULL);
    if (dwResult != ERROR_SUCCESS)
    {
        wprintf(L"WlanRegisterNotification failed with error: %u\n", dwResult);
        return S_FALSE;
    }
    return S_OK;
}

HRESULT WlanApiClass::scan(BOOL isSync)
{
    WlanApiClass::Scan_Lock.lock();
    is_scan_finish = FALSE;
    cout << "WlanScan";
    DWORD dwResult = 0;
    PWLAN_INTERFACE_INFO pIfInfo = NULL;
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    PDOT11_SSID pDotSSid = NULL;

    unsigned int ifaceNum = 0;
    dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
    if (dwResult != ERROR_SUCCESS)
    {
        wprintf(L"WlanEnumInterfaces failed with error: %u\n", dwResult);
        Sleep(1000);
        is_scan_finish = TRUE;
        WlanApiClass::Scan_Lock.unlock();
        return S_FALSE;
    }
    else
    {
        uint32_t correct_counter = 0;
        for (ifaceNum = 0; ifaceNum < (int)pIfList->dwNumberOfItems; ifaceNum++)
        {
            pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[ifaceNum];
            PWLAN_RAW_DATA WlanRawData = NULL;
            callbackInfo.interfaceGUID = pIfInfo->InterfaceGuid;
            if (isSync)
            {
                callbackInfo.handleEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            }
            else
            {
                callbackInfo.callbackReason = 8;
            }

            // printf("WlanScan (%d)... ", ifaceNum);
            dwResult = WlanScan(hClient, &pIfInfo->InterfaceGuid, pDotSSid, WlanRawData, NULL);
            if (dwResult != ERROR_SUCCESS)
            {
                wprintf(L"WlanScan failed with error: %u\n", dwResult);
                Sleep(1000);
                is_scan_finish = TRUE;
                WlanApiClass::Scan_Lock.unlock();
                return S_FALSE;
            }
            if (isSync)
            {
                DWORD waitResult = MsgWaitForMultipleObjects(1, &callbackInfo.handleEvent, 0, 10000, QS_ALLINPUT);
                if (waitResult == WAIT_OBJECT_0 + 1)
                {
                    if (callbackInfo.callbackReason == wlan_notification_acm_scan_complete)
                    {
                        // printf("ok\n");
                        goto end;
                    }
                }
            }
        }
    }
end:
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
    if (isSync)
    {
        CloseHandle(callbackInfo.handleEvent);
    }
    WlanApiClass::Scan_Lock.unlock();
    cout << " ok" << endl;
    is_scan_finish = TRUE;
    return S_OK;
}

HRESULT WlanApiClass::scanSync()
{
    if (scan(TRUE) == S_OK)
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

HRESULT WlanApiClass::scanAsync()
{
    if (scan(FALSE) == S_OK)
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

HRESULT WlanApiClass::connect(GUID guid_for_wlan, WCHAR *WProfile, WCHAR *profileName, char *ssid, ULONG ssidLen)
{
    DWORD dwResult;
    DWORD profileReasonCode;
    WLAN_CONNECTION_PARAMETERS connectionParams;
    DOT11_SSID ap_ssid;

    size_t strSize = 40;
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
    dwResult = WlanSetProfile(hClient, &guid_for_wlan, 0, WProfile, NULL, TRUE, NULL, &profileReasonCode);
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
        printf("WlanConnect failed with error: %u\n", dwResult);
        return S_FALSE;
    }
    else
    {
        printf("ok\n");
        return S_OK;
    }
}

HRESULT WlanApiClass::disconnect(GUID guid_for_wlan)
{
    DWORD dwResult;
    dwResult = WlanDisconnect(hClient, &guid_for_wlan, NULL);
    if (dwResult == ERROR_SUCCESS)
    {
        return S_OK;
    }
    else
    {
        wprintf(L"WlanDisconnect failed with error: %u\n", dwResult);
        return S_FALSE;
    }
}

vector<SCAN_RESULT> WlanApiClass::get_network_list()
{
    DWORD dwResult = 0;
    PWLAN_BSS_LIST WlanBssList = NULL;
    PWLAN_INTERFACE_INFO pIfInfo = NULL;
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    PDOT11_SSID pDotSSid = NULL;

    unsigned int ifaceNum = 0;
    vector<SCAN_RESULT> scan_result_list;
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

            // printf("WlanShowNetworksList... ");
            if (WlanGetNetworkBssList(hClient, &pIfInfo->InterfaceGuid, pDotSSid, dot11_BSS_type_infrastructure, FALSE, NULL, &WlanBssList) == ERROR_SUCCESS)
            {
                // printf("ok\n");
                for (unsigned int c = 0; c < WlanBssList->dwNumberOfItems; c++)
                {
                    if (strstr((char *)WlanBssList->wlanBssEntries[c].dot11Ssid.ucSSID, "MediCam_"))
                    {
                        SCAN_RESULT scan_result;
                        // wprintf(L"ssid: %hs, rssi: %d\n", WlanBssList->wlanBssEntries[c].dot11Ssid.ucSSID, WlanBssList->wlanBssEntries[c].lRssi);
                        scan_result.ssid = std::string((char *)WlanBssList->wlanBssEntries[c].dot11Ssid.ucSSID, WlanBssList->wlanBssEntries[c].dot11Ssid.uSSIDLength);
                        scan_result.rssi = WlanBssList->wlanBssEntries[c].lRssi;
                        scan_result_list.push_back(scan_result);
                    }
                }
            }
        }
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
    return scan_result_list;
}

vector<INTERFACE_INFO> WlanApiClass::get_interface_info()
{
    PWLAN_INTERFACE_INFO_LIST plist = NULL;
    PWLAN_INTERFACE_INFO pInfo = NULL;
    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
    WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
    DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
    vector<INTERFACE_INFO> interface_info_list;
    BOOL error = FALSE;
    if (WlanEnumInterfaces(hClient, NULL, &plist) == ERROR_SUCCESS)
    {
        // status = napi_create_array(env, &ifaceList);
        for (int i = 0; i < (int)plist->dwNumberOfItems; i++)
        {
            INTERFACE_INFO interface_info;
            pInfo = (WLAN_INTERFACE_INFO *)&plist->InterfaceInfo[i];
            WCHAR GuidString[39] = {0};
            if (StringFromGUID2(pInfo->InterfaceGuid, (LPOLESTR)&GuidString, sizeof(GuidString) / sizeof(*GuidString)) != 0)
            {
                _bstr_t b(GuidString);
                interface_info.guid = b;
            }
            else
            {
                printf("StringFromGUID2 error");
                error = TRUE;
                goto end;
            }
            _bstr_t b_desc(pInfo->strInterfaceDescription);
            interface_info.description = b_desc;

            switch (pInfo->isState)
            {
            case wlan_interface_state_not_ready:
                interface_info.connection = "not ready";
                break;
            case wlan_interface_state_connected:
                interface_info.connection = "connected";
                break;
            case wlan_interface_state_ad_hoc_network_formed:
                interface_info.connection = "ad_hoc";
                break;
            case wlan_interface_state_disconnecting:
                interface_info.connection = "disconnection";
                break;
            case wlan_interface_state_disconnected:
                interface_info.connection = "disconnected";
                break;
            case wlan_interface_state_associating:
                interface_info.connection = "connecting";
                break;
            case wlan_interface_state_discovering:
                interface_info.connection = "discovering";
                break;
            case wlan_interface_state_authenticating:
                interface_info.connection = "authenticating";
                break;
            default:
                interface_info.connection = "unknown";
                break;
            }

            if (pInfo->isState == wlan_interface_state_connected)
            {
                if (WlanQueryInterface(hClient, &pInfo->InterfaceGuid, wlan_intf_opcode_current_connection, NULL, &connectInfoSize, (PVOID *)&pConnectInfo, &opCode) == ERROR_SUCCESS)
                {
                    switch (pConnectInfo->wlanConnectionMode)
                    {
                    case wlan_connection_mode_profile:
                        interface_info.mode = "profile";
                        break;
                    case wlan_connection_mode_temporary_profile:
                        interface_info.mode = "temporary_profile";
                        break;
                    case wlan_connection_mode_discovery_secure:
                        interface_info.mode = "secure_discovery";
                        break;
                    case wlan_connection_mode_discovery_unsecure:
                        interface_info.mode = "un-secure_discovery";
                        break;
                    case wlan_connection_mode_auto:
                        interface_info.mode = "persistent_profile";
                        break;
                    case wlan_connection_mode_invalid:
                        interface_info.mode = "invalid";
                        break;
                    default:
                        interface_info.mode = "unknown";
                        break;
                    }

                    _bstr_t profileName(pConnectInfo->strProfileName);
                    interface_info.profile_name = profileName;

                    // SSID
                    if (pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength != 0)
                    {
                        interface_info.ssid = std::string((char *)pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID, pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength);
                    }

                    // physical type
                    switch (pConnectInfo->wlanAssociationAttributes.dot11BssType)
                    {
                    case dot11_BSS_type_infrastructure:
                        interface_info.bssid_type = "infrastructure";
                        break;
                    case dot11_BSS_type_independent:
                        interface_info.bssid_type = "independent";
                        break;
                    case dot11_BSS_type_any:
                        interface_info.bssid_type = "others";
                        break;
                    }

                    // MAC
                    char mac[17];
                    sprintf(mac, "%02x-%02x-%02x-%02x-%02x-%02x",
                            pConnectInfo->wlanAssociationAttributes.dot11Bssid[0],
                            pConnectInfo->wlanAssociationAttributes.dot11Bssid[1],
                            pConnectInfo->wlanAssociationAttributes.dot11Bssid[2],
                            pConnectInfo->wlanAssociationAttributes.dot11Bssid[3],
                            pConnectInfo->wlanAssociationAttributes.dot11Bssid[4],
                            pConnectInfo->wlanAssociationAttributes.dot11Bssid[5]);
                    interface_info.AP_MAC = mac;
                }
                else
                {
                    printf("WlanQueryInterface failed");
                    error = TRUE;
                    goto end;
                }
            }
            interface_info_list.push_back(interface_info);
        }
    }
    else
    {
        printf("WlanEnumInterfaces failed");
        error = TRUE;
        goto end;
    }

end:
    return interface_info_list;
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
    }
    return;
}

void free_memory(void *p)
{
    if (p)
    {
        free(p);
        p = NULL;
    }
}