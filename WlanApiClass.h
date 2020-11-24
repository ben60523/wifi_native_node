#include <Windows.h>
#include <wlanapi.h>
#include <mutex>
#include <thread>

typedef struct _WLAN_CALLBACK_INFO
{
    GUID interfaceGUID;
    HANDLE handleEvent;
    DWORD callbackReason;
} WLAN_CALLBACK_INFO;

void wlanCallback(WLAN_NOTIFICATION_DATA *WlanNotificationData, PVOID myContext);
void free_memory(void *p);
class WlanApiClass
{
private:
    /* data */
public:
    WlanApiClass(/* args */);
    ~WlanApiClass();

    WLAN_CALLBACK_INFO callbackInfo = {0};
    HANDLE hClient = NULL;
    BOOL initflag = FALSE;
    std::mutex Scan_Lock;

    void scan(BOOL isSync);
    void get_network_list(void);
    void get_interface_info(void);
    HRESULT WlanApiClass::connect(GUID guid_for_wlan, WCHAR *WProfile, char *ssid, ULONG ssidLen);
    HRESULT disconnect(GUID guid_for_wlan);
    /**
     * scanSync: It will stop and wait wlan_callback_info comes
     *           Create a new thread to avoid stucking event loop
    */
    void scanSync(void);
    /**
     * scanAsync: It need to get wlan_callback _info manually
     *            cb() will be executed after WlanScan() is done
    */
    void scanAsync(void *cb());
};
