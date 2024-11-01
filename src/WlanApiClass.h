#include <Windows.h>
#include <wlanapi.h>
#include <mutex>
#include <string>
#include <vector>

using namespace std;
typedef struct _WLAN_CALLBACK_INFO
{
    GUID interfaceGUID;
    HANDLE handleEvent;
    DWORD callbackReason;
} WLAN_CALLBACK_INFO;

typedef struct _SCAN_RESULT
{
    string ssid;
    LONG rssi;

} SCAN_RESULT;

typedef struct _INTERFACE_INFO
{
    string guid;
    string description;
    string connection;
    string mode;
    string profile_name;
    string ssid;
    string bssid_type;
    string AP_MAC;
} INTERFACE_INFO;

void wlanCallback(WLAN_NOTIFICATION_DATA *WlanNotificationData, PVOID myContext);
void free_memory(void *p);
class WlanApiClass
{
private:
    /* data */
    BOOL is_scan_finish;

public:
    WlanApiClass(/* args */);
    ~WlanApiClass();

    WLAN_CALLBACK_INFO callbackInfo;
    HANDLE hClient;
    std::mutex Scan_Lock;

    HRESULT init();
    HRESULT scan(BOOL isSync);
    vector<SCAN_RESULT> get_network_list(void);
    vector<INTERFACE_INFO> get_interface_info(void);
    HRESULT connect(GUID guid_for_wlan, WCHAR *WProfile, WCHAR *profileName, char *ssid, ULONG ssidLen);
    HRESULT disconnect(GUID guid_for_wlan);
    /**
     * scanSync: It will stop and wait wlan_callback_info comes
    */
    HRESULT scanSync(void);
    /**
     * scanAsync: It won't stop and wait wlan_callback_info comes
    */
    HRESULT scanAsync();

    BOOL scan_finish()
    {
        return is_scan_finish;
    };
};
