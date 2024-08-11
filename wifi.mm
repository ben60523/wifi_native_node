#include <CoreWLAN/CoreWLAN.h>
#import <CoreLocation/CoreLocation.h>
#include <functional>
#include <objc/NSObject.h>
#include <Foundation/Foundation.h>
#include <cstring>
#include <napi.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <string>
#include <vector>

using namespace Napi;

typedef struct NetworkItem {
  char *ssid;
  int rssi;
} NetworkItem;

class WifiScanWorker : public AsyncWorker {
public:
  WifiScanWorker(Function callback) : AsyncWorker(callback) {}
  ~WifiScanWorker() {}
  void Execute() override { scan_networks(); }
  void OnOK() override {
    HandleScope scope(Env());
    NSLog(@"Trigger CB");
    Napi::Array res = Napi::Array::New(Env());
    for (size_t i = 0; i < networks.size(); i++) {
      NetworkItem item = networks.at(i);
      Napi::Object obj = Napi::Object::New(Env());
      obj.Set(Napi::String::New(Env(), "ssid"),
              Napi::String::New(Env(), item.ssid));
      obj.Set(Napi::String::New(Env(), "rssi"),
              Napi::Number::New(Env(), item.rssi));
      res.Set(i, obj);
    }
    NSLog(@"Ready to enter callback func");
    Callback().Call(Env().Global(), {res});
  }

private:
  Napi::Array _networks;
  std::vector<NetworkItem> networks;
  void scan_networks() {
    CWInterface *wifiInterface = [[CWWiFiClient sharedWiFiClient] interface];
    if (!wifiInterface) {
      NSLog(@"No Wi-Fi interface found");
    }
    NSLog(@"Interface %@ found.", [wifiInterface interfaceName]);
    NSError *err = nil;
    NSSet *network_list = [wifiInterface scanForNetworksWithName:nil
                                                           error:&err];
    if (err) {
      const char *msg = [[err localizedDescription] UTF8String];
      NSLog(@"ERROR::%s", msg);
    }
    NSLog(@"Scanning is over, ready to output the list[%zd]",
          network_list.count);
    for (CWNetwork *network in network_list) {
      // NSLog(@"===============================");
      // NSLog(@"SSID: %@", network.ssid ?: @"(null)");
      // NSLog(@"BSSID: %@", network.bssid);
      // NSLog(@"RSSI: %ld", network.rssiValue);
      // NSLog(@"Noise: %ld", network.noiseMeasurement);
      // NSLog(@"Channel: %@", network.wlanChannel);
      if (network.ssid) {
        NetworkItem element = NetworkItem();
        element.ssid = (char *)[[network ssid] UTF8String];
        element.rssi = network.rssiValue;
        networks.push_back(element);
      }
    }
    NSLog(@"The list can be used in callback");
  }
};

class WifiConnectWorker : public AsyncWorker {
public:
  WifiConnectWorker(const char *ssid, const char *password, Function callback)
      : AsyncWorker(callback), ssid(ssid), password(password) {}
  ~WifiConnectWorker() {}
  void Execute() override { connect_network(); }
  void OnOK() override {
    HandleScope scope(Env());
    Napi::Boolean ret = Napi::Boolean::New(Env(), res);
    Callback().Call(Env().Global(), {ret});
  }

private:
  bool res;
  const char *ssid;
  const char *password;
  void connect_network() {
    CWInterface *wifiInterface = [[CWWiFiClient sharedWiFiClient] interface];
    if (!wifiInterface) {
      NSLog(@"No Wi-Fi interface found");
    }

    NSLog(@"Ineterface %@ is found", [wifiInterface interfaceName]);
    NSError *err = nil;
    NSString *ns_ssid = [NSString stringWithUTF8String:ssid];
    NSString *ns_pass = [NSString stringWithUTF8String:password];

    bool apFound = false;
    for (CWNetwork *network in [wifiInterface cachedScanResults]) {
      if ([network ssid] && [[network ssid] isEqualToString:ns_ssid]) {
        apFound = true;
        res = [wifiInterface associateToNetwork:network
                                       password:ns_pass
                                            error:&err];
        if (!res) {
          const char *msg = [[err localizedDescription] UTF8String];
          NSLog(@"Connecting error %s", msg);
        }
        break;
      }
    }
    if (!apFound) {
      NSLog(@"%s is not found", ssid);
    }
  }
};

Value scan(const CallbackInfo &info) {
  Function cb = info[0].As<Function>();
  WifiScanWorker *worker = new WifiScanWorker(cb);
  worker->Queue();
  return info.Env().Undefined();
}

Value connectAp(const CallbackInfo &info) {
  std::string ssid_str = info[0].As<Napi::String>();
  std::string pass_str = info[1].As<Napi::String>();
  const char *ssid = ssid_str.c_str();;
  const char *pass = pass_str.c_str();
  Function cb = info[2].As<Function>();
  WifiConnectWorker *worker = new WifiConnectWorker(ssid, pass, cb);
  worker->Queue();
  return info.Env().Undefined();
}

Value getWifiStatus(const CallbackInfo &info) {
  Env env = info.Env();
  CWWiFiClient *wifiClient = [CWWiFiClient sharedWiFiClient];
  NSArray<CWInterface *> *interfaces = [wifiClient interfaces];
  Array res = Array::New(env);
  int count = 0;
  for (CWInterface *interface in interfaces) {
    bool isConnected = false;
    Napi::Object obj = Napi::Object::New(env);
    const char *ssid = [[interface ssid] UTF8String];
    const char *bssid = [[interface bssid] UTF8String];
    bool isOn = [interface powerOn];
    const char *name = [[interface interfaceName] UTF8String];
    obj.Set(Napi::String::New(env, "ip"), Napi::String::New(env, ""));
    struct ifaddrs *ifa, *ifaddr;
    char host[NI_MAXHOST];
    if (getifaddrs(&ifaddr) == -1) {
      NSLog(@"Error getting ip address");
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr == NULL) {
        continue;
      }
      int family = ifa->ifa_addr->sa_family;
      if (family == AF_INET || family == AF_INET6) {
        if (strcmp(ifa->ifa_name, name) == 0) {
          size_t sockaddr_in = (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
          int s = getnameinfo(ifa->ifa_addr, sockaddr_in, host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
          if (s != 0) {
            NSLog(@"getnameinfo() failed: %s", gai_strerror(s));
            obj.Set(Napi::String::New(env, "ip"), Napi::String::New(env, ""));
          }
          if (family == AF_INET) {
            obj.Set(Napi::String::New(env, "ip"), Napi::String::New(env, host));
            isConnected = true;
          }
        }
      }
    }
    freeifaddrs(ifaddr);
    obj.Set(Napi::String::New(env, "ssid"), Napi::String::New(env, ssid ?: ""));
    obj.Set(Napi::String::New(env, "bssid"), Napi::String::New(env, bssid ?: ""));
    obj.Set(Napi::String::New(env, "isOn"), Napi::Boolean::New(env, isOn));
    obj.Set(Napi::String::New(env, "name"), Napi::String::New(env, name));
    obj.Set(Napi::String::New(env, "connection"), Napi::String::New(env, isConnected ? "connected" : "disconnected"));
    res.Set(count, obj);
    count ++;
  }
  return res;
}

Object Init(Env env, Object exports) {
  exports.Set(Napi::String::New(env, "scan"), Napi::Function::New(env, scan));
  exports.Set(Napi::String::New(env, "connect"), Napi::Function::New(env, connectAp));
  exports.Set(Napi::String::New(env, "getWifiStatus"), Napi::Function::New(env, getWifiStatus));
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init);
