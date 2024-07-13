#include <CoreWLAN/CoreWLAN.h>
#include <Foundation/Foundation.h>
#include <MacTypes.h>
#include <cstddef>
#include <napi.h>
#include <objc/objc.h>
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
      NSLog(@"===============================");
      NSLog(@"SSID: %@", network.ssid ?: @"(null)");
      NSLog(@"BSSID: %@", network.bssid);
      NSLog(@"RSSI: %ld", network.rssiValue);
      NSLog(@"Noise: %ld", network.noiseMeasurement);
      NSLog(@"Channel: %@", network.wlanChannel);
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
    NSError *err = nil;
    NSString *ns_ssid = [NSString stringWithUTF8String:ssid];
    NSString *ns_pass = [NSString stringWithUTF8String:password];

    for (CWNetwork *network in [wifiInterface cachedScanResults]) {
      if ([network ssid] && [[network ssid] isEqualToString:ns_ssid]) {
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
  }
};

Value scan(const CallbackInfo &info) {
  Function cb = info[0].As<Function>();
  WifiScanWorker *worker = new WifiScanWorker(cb);
  worker->Queue();
  return info.Env().Undefined();
}

Value connect(const CallbackInfo &info) {
  const char *ssid = info[0].As<String>().Utf8Value().c_str();
  const char *pass = info[1].As<String>().Utf8Value().c_str();
  Function cb = info[2].As<Function>();
  WifiConnectWorker *worker = new WifiConnectWorker(ssid, pass, cb);
  worker->Queue();
  return info.Env().Undefined();
}

Object Init(Env env, Object exports) {
  exports.Set(Napi::String::New(env, "scan"), Napi::Function::New(env, scan));
  exports.Set(Napi::String::New(env, "connect"),
              Napi::Function::New(env, connect));
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init);
