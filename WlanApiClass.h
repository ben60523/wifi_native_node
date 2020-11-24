#include <Windows.h>
#include <wlanapi.h>
#include <mutex>

typedef struct _WLAN_CALLBACK_INFO
{
    GUID interfaceGUID;
    HANDLE handleEvent;
    DWORD callbackReason;
} WLAN_CALLBACK_INFO;

void wlanCallback(WLAN_NOTIFICATION_DATA *WlanNotificationData, PVOID myContext);
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
    void scanSync(void);
    void scanAsync(void *cb());
    void get_network_list(void);
    void get_interface_info(void);
    void connect(void);
    void disconnect(void);
};
