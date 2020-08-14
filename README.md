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
     |___ example # Example of the project
     |      |____ wifi_connect.js
     |      |____ wifi_disconnect.js
     |      |____ wifi_scan.js
     |
     |___ services # Long task, run with new thread
     |      |____ connect_service.js
     |      |____ scan_service.js
     |
     |___ index.js # Module entry
     |___ binding.gyp # Binding info
     |___ wifi_native.cc # Library of the project
                         # which is written by c
```

## Packing project with electron-builder
*  Starting worker thread from ASAR file is not support for now (ref: https://stackoverflow.com/questions/59630103/using-worker-thread-in-electron).
   1. You have to let wifi_native unpacked by add `extraResources` and `asarUnpack` of `build` in package.json
    ```json
    {
        "build": {
            "extraResources": [
                {
                    "from": "./node_modules/wifi_native",
                    "to": "./extra_res/module/wifi_native"
                }
            ]
        }
    }
    
    ```
   2. Add `bindings` module in your project
   3. Require `wifi_native` with the correct path in non-development environment


    ```javascript
        let isDev = "is_Development_Environment";
        if (isDev) {
            let wifiNative = require("wifi_native");
        } else {
            let wifiNative = require("../../../extra_res/module/wifi_native");
        }
    ```

## Methods
### `init()`
* Call this method to initialize before any function in this project

### `scan()`
* Get ssid and signal intense of nearby APs
``` javascript
    wlanNative.scan()
        .then((results) => {
        /*
            result = [
                {
                    ssid: String
                    rssi: Number
                },
                
                .
                .
                .
            ]
        */
        })
        .catch((results) => {
            console.log("scan failed");
        })

```
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
### `getIfaceStateNative()`
* get information of Wi-Fi interfaces
```javascript
wlanNative.getIfaceStateNative().then((ifaceList) => {
    console.log("info", ifaceList);
}).catch((e) => {
    console.error("error", e);
})
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
