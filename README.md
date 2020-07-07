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

## Methods
### `init()`
* Call this method to initialize before any function in this project
```javascript
var wlanNative = require("../index");
wlanNative.init();
```
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
### `free()`
* Free memory if all the methods will not be used
