# wifi_native
* Use wlanpai.h in javascript with n-api
## Environment Setup
```ps
npm install --global windows-build-tools # To set up MSVS Build tools enviornment if needed
npm install --global node-gyp # To use node-gyp globally
```

## Build Project
```ps
# More info https://github.com/nodejs/node-gyp
node-gyp build
# create and build MSVS project automatically
```
## Project structure

```ps
wifi_native
     |
     |___ index.js # Module entry
     |___ binding.gyp # Binding info
     |___ wifi_native.cc # Interface between C/C++ and Javascript
     |___ src # The baseclass 
           |___ WlanClassApi.cc
           |___ WlanClassApi.h 
```

## Methods
### `init()`
* Call this method to initialize before any function in this project
### `scanAsync()`
* Get ssid and signal intense of nearby APs. __Note that this api won't wait the response of Wi-Fi adapter__
### `scanSync()`
* Get ssid and signal intense of nearby APs. __Note that this api will wait the response of Wi-Fi adapter__
### `connect()`
* To connect selected AP 
    * resolve if connect command is executed successfully
```javascript
let ap = {
    ssid: "your_ap_ssid",
    password: "your_ap_password" | NULL
}
let wifi_interface_name = "Wi-Fi"
wifi_native.connect(ap, wifi_interface_name)
    .then(() => {
        console.log("connection success");
    })
    .catch(() => {
        console.log("connection failed");
    });
```
### `disconnect()`
* To disconnect the connection between AP and certain Wi-Fi interface

```javascript
wifi_native.disconnect(wifi_interface_name)
    .then(() => {
        console.log("disconnected")
    })
    .catch(() => {
        console.log("failed")
    })
```

### `getNetworkList()`
* get APs which is detected last time
```javascript
wifiNative.getNetworkList()
    .then((list) => {
        console.log(list);
    })
    .catch(() => {
        console.log("getting list failed");
    })
```
### `getIfaceState()`
* get information of Wi-Fi interfaces
```javascript
let info = wlanNative.getIfaceStateNative()
```
* output
```javascript
[
  {
    guid: '{DCF5C285-4B1F-419B-A0D0-8BCEC4D24E59}',
    description: 'Edimax AC600 Wireless LAN USB Adapter',
    connection: 'disconnected'
  },
  {
    guid: '{DC94B46C-AE8C-4078-B15F-D55CD47AC640}',
    description: 'Intel(R) Dual Band Wireless-AC 8260',
    connection: 'connected',
    mode: 'profile',
    profile_name: 'UNICORNWORKING-5G',
    ssid: 'UNICORNWORKING-5G',
    bssid_type: 'infrastructure',
    AP_MAC: 'b8-ec-a3-f7-fc-06'
  }
]
```

### `free()`
* Free memory if all the methods will not be used
