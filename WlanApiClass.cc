#include "WlanApiClass.h"
#include <wlanapi.h>
#include <stdio.h>
#include <thread>
#include <functional>

WlanApiClass::WlanApiClass()
{
    DWORD dwResult = 0;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
    if (dwResult != ERROR_SUCCESS)
    {
        printf("WlanOpenHandle failed with error: %u\n", dwResult);
    }
    dwResult = WlanRegisterNotification(hClient, WLAN_NOTIFICATION_SOURCE_ALL, TRUE, (WLAN_NOTIFICATION_CALLBACK)wlanCallback, (PVOID)&callbackInfo, NULL, NULL);
    if (dwResult != ERROR_SUCCESS)
    {
        wprintf(L"WlanRegisterNotification failed with error: %u\n", dwResult);
    }
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

void WlanApiClass::scan(BOOL isSync)
{
    Scan_Lock.lock();
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
            if (isSync)
            {
                callbackInfo.handleEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            }
            else
            {
                callbackInfo.callbackReason = 8;
            }

            printf("WlanScan (%d)... ", ifaceNum);
            dwResult = WlanScan(hClient, &pIfInfo->InterfaceGuid, pDotSSid, WlanRawData, NULL);
            if (dwResult != ERROR_SUCCESS)
            {
                wprintf(L"WlanScan failed with error: %u\n", dwResult);
            }
            if (isSync)
            {
                DWORD waitResult = WaitForSingleObject(callbackInfo.handleEvent, 5000);
                if (waitResult == WAIT_OBJECT_0)
                {
                    if (callbackInfo.callbackReason == wlan_notification_acm_scan_complete)
                    {
                        printf("ok\n");
                        // status = napi_create_int32(env, 1, &scan_flag);
                        // pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[ifaceNum];
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
    if (isSync)
    {
        CloseHandle(callbackInfo.handleEvent);
    }
    Scan_Lock.unlock();
}

void WlanApiClass::scanSync()
{
    // scan is the member function of WlanApiClass, so you need to call it in new thread in compiler style.
    // That is, func(&this, ..arg) in the constructor of thread.
    // More Ref. https://stackoverflow.com/questions/10673585/start-thread-with-member-function
    std::thread Scan_Thread(&scan, this, true);
    Scan_Thread.detach();
}

void WlanApiClass::scanAsync(void *cb())
{
    WlanApiClass::scan(FALSE);
    if (cb)
    {
        cb();
    }
}

HRESULT WlanApiClass::connect(GUID guid_for_wlan, WCHAR *WProfile, char *ssid, ULONG ssidLen)
{
    // char GuidString[40] = {0};
    // WCHAR GuidWString[40] = {0};
    DWORD dwResult;
    DWORD profileReasonCode;
    WLAN_CONNECTION_PARAMETERS connectionParams;
    // GUID guid_for_wlan;
    char *profile = NULL;
    char *ssid = NULL;
    WCHAR *profileName = NULL;
    // WCHAR *WProfile = NULL;
    DOT11_SSID ap_ssid;

    printf("Copy GUID... ");
    size_t strSize = 40;
    // std::string guid_arg = info[0].As<Napi::String>();
    // strcpy(GuidString, guid_arg.c_str());
    // // To wchar
    // mbstowcs(GuidWString, GuidString, 40);
    // // To GUID
    // dwResult = CLSIDFromString(GuidWString, &guid_for_wlan);
    // if (dwResult != NOERROR)
    // {
    //     wprintf(L"CLSIDFromString failed with error: %u\n", dwResult);
    // }
    // printf("ok\n");

    // get profile content
    // printf("Copy profile contents... ");
    // std::string profile_arg = info[1].As<Napi::String>();
    // size_t profileLen = profile_arg.length();
    // profile = (char *)malloc(sizeof(char) * profileLen);
    // strcpy(profile, profile_arg.c_str());
    // WProfile = (WCHAR *)malloc(sizeof(WCHAR) * profileLen);
    // mbstowcs(WProfile, profile, profileLen + 1);
    // printf("ok\n");

    // get filename of wi_fi profile for WlanSetProfile()
    // printf("Copy profileName... ");
    // std::string ssid_arg = info[2].As<Napi::String>();
    // size_t ssidLen = ssid_arg.length();
    // ssid = (char *)malloc(sizeof(char) * ssidLen);
    // strcpy(ssid, ssid_arg.c_str());
    // profileName = (WCHAR *)malloc(sizeof(WCHAR) * ssidLen);
    // mbstowcs(profileName, ssid, ssidLen + 1);
    // printf("ok\n");

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
    //FIXME: FREE MEMORY
    printf("free memory...");
    free_memory(profile);
    // free_memory(WProfile);
    // free_memory(ssid);
    free_memory(profileName);
    printf("ok\n");
    if (dwResult != ERROR_SUCCESS)
    {
        printf("WlanConnect failed with error: %u\n", dwResult);
        return S_FALSE;
    }
    else
    {
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

void WlanApiClass::get_network_list()
{
}

void WlanApiClass::get_interface_info()
{
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