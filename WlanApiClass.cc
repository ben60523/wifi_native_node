#include "WlanApiClass.h"
#include <wlanapi.h>
#include <stdio.h>

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
    initflag = TRUE;
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
}

void WlanApiClass::scanAsync(void *cb())
{
    WlanApiClass::scan(FALSE);
    if (cb)
    {
        cb();
    }
}

void WlanApiClass::connect()
{
}

void WlanApiClass::disconnect()
{
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