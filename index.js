var wifi_native = require('bindings')('wifi_native');
var fs = require("fs");

var scan_loop = null;

var win32WirelessProfileBuilder = function (ssid, security, key) {
    var profile_content;
    if (security == null) {
        security = false;
    }
    if (key == null) {
        key = null;
    }
    profile_content = "<?xml version=\"1.0\"?> <WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\"> <name>" + ssid.plaintext + "</name> <SSIDConfig> <SSID> <hex>" + ssid.hex + "</hex> <name>" + ssid.plaintext + "</name> </SSID> </SSIDConfig>";
    switch (security) {
        case "wpa":
            profile_content += "<connectionType>ESS</connectionType> <connectionMode>auto</connectionMode> <autoSwitch>true</autoSwitch> <MSM> <security> <authEncryption> <authentication>WPAPSK</authentication> <encryption>TKIP</encryption> <useOneX>false</useOneX> </authEncryption> <sharedKey> <keyType>passPhrase</keyType> <protected>false</protected> <keyMaterial>" + key + "</keyMaterial> </sharedKey> </security> </MSM>";
            break;
        case "wpa2":
            profile_content += "<connectionType>ESS</connectionType> <connectionMode>auto</connectionMode> <autoSwitch>true</autoSwitch> <MSM> <security> <authEncryption> <authentication>WPA2PSK</authentication> <encryption>AES</encryption> <useOneX>false</useOneX> </authEncryption> <sharedKey> <keyType>passPhrase</keyType> <protected>false</protected> <keyMaterial>" + key + "</keyMaterial> </sharedKey> </security> </MSM>";
            break;
        default:
            profile_content += "<connectionType>ESS</connectionType> <connectionMode>manual</connectionMode> <MSM> <security> <authEncryption> <authentication>open</authentication> <encryption>none</encryption> <useOneX>false</useOneX> </authEncryption> </security> </MSM>";
    }
    profile_content += "</WLANProfile>";
    return profile_content;
};

var getIfaceState = function () {
    var KEY, VALUE, connectionData, j, k, len, ln, ln_trim, parsedLine, ref;
    connectionData = require("child_process").execSync("chcp 65001&netsh wlan show interface").toString("utf8");
    ref = connectionData.split('\n');
    var adapterName = "";
    let ifaceList = getIfaceStateNative();
    for (k = j = 0, len = ref.length; j < len; k = ++j) {
        ln = ref[k];
        try {
            ln_trim = ln.trim();
            if (ln_trim === "Software Off") {
                break;
            } else {
                parsedLine = new RegExp(/([^:]+): (.+)/).exec(ln_trim);
                KEY = parsedLine[1].trim();
                VALUE = parsedLine[2].trim();
            }
        } catch (error1) {
            error = error1;
            continue;
        }
        if (VALUE.startsWith("Wi-Fi")) {
            adapterName = VALUE;
        }
        switch (KEY) {
            case "GUID":
                ifaceList.forEach(iface => {
                    if (iface.guid.toLowerCase().includes(VALUE.toLowerCase())) {
                        iface.adapterName = adapterName;
                    }
                });
                break;
            default:
                break;
        }
    }
    // console.log(ifaceList);
    return ifaceList;

}

var getIfaceStateNative = function () {
    let ifaceState;
    wifi_native.wlanGetIfaceInfo((ifaceList) => {
        ifaceState = ifaceList;
    })
    return ifaceState;
}

var writeProfile = function (_ap) {
    let ssid, profileName;
    ssid = {
        plaintext: _ap.ssid,
        hex: ""
    };
    for (i = j = 0, ref = ssid.plaintext.length - 1; 0 <= ref ? j <= ref : j >= ref; i = 0 <= ref ? ++j : --j) {
        ssid.hex += ssid.plaintext.charCodeAt(i).toString(16);
    }
    xmlContent = null;
    if (_ap.password.length) {
        xmlContent = win32WirelessProfileBuilder(ssid, "wpa2", _ap.password);
    } else {
        xmlContent = win32WirelessProfileBuilder(ssid);
    }
    try {
        fs.writeFileSync(`${require('os').homedir}\\${_ap.ssid}.xml`, xmlContent)
        profileName = `${require('os').homedir}\\${_ap.ssid}.xml`;
    } catch (error2) {
    }
    return profileName;
}

var init = async function () {
    var initialize = function () {
        return new Promise((resolve, reject) => {
            if (wifi_native.wlanInit() == 0) {
                return true;
            }
            else {
                return false;
            }
        });
    };
    let resp = await initialize();
}

var getNetworkList = function () {
    return new Promise((resolve, reject) => {
        console.log("GetWlanApList")
        wifi_native.wlanGetNetworkList((MediCamNetWorks) => {
            if (MediCamNetWorks) {
                for (let j = MediCamNetWorks.length - 1; j >= 0; j--) {
                    for (let i = MediCamNetWorks.length - 1; i >= 0; i--) {
                        if (MediCamNetWorks[i] && MediCamNetWorks[j]) {
                            if (MediCamNetWorks[j].ssid == MediCamNetWorks[i].ssid && i != j) {
                                if (MediCamNetWorks[j].rssi >= MediCamNetWorks[i].rssi) {
                                    MediCamNetWorks.splice(i, 1);
                                } else {
                                    MediCamNetWorks.splice(j, 1);
                                }
                            }
                        }
                    }
                }
                console.log(MediCamNetWorks);
                resolve(MediCamNetWorks);
            } else {
                reject();
            }
        })
    });
}

var scanSync = function () {
    return new Promise((resolve, reject) => {
        wifi_native.wlanScanSync(() => {
            getNetworkList().then((list) => {
                resolve(list);
            })
        })
    });
}

var kill_scan_keep = function () {
    if (scan_loop) {
        clearInterval(scan_loop);
        scan_loop = null;
    }
}

var scanAsync = function () {
    return new Promise((resolve, reject) => {
        wifi_native.wlanScanAsync((flag) => {
            while (1) {
                if (wifi_native.wlanCheckScanFinish()) {
                    getNetworkList().then((list) => {
                        resolve(list);
                    })
                    break;
                }
            }
        })
    });
};

var free = function () {
    wifi_native.wlanFree()
}

var listenWiFiEvent = function (timeout, EventCode) {
    let counter = timeout / 200;
    return new Promise((resolve, reject) => {
        let failedCount = 0;
        let interval = setInterval(() => {
            let ret = wifi_native.wlanListener();
            if (failedCount > counter) {
                clearInterval(interval);
                console.log("Cannot get expected callback event", ret);
                reject();
            }
            if (ret == EventCode) {
                clearInterval(interval);
                resolve();
            } else {
                failedCount++;
            }
        }, 200)
    });
}

var connect = function (_ap, adapter) {
    return new Promise((resolve, reject) => {
        let iface = getIfaceState();
        let guid, profile;
        for (let ifaceNum = 0; ifaceNum < iface.length; ifaceNum++) {
            if (iface[ifaceNum].adapterName == adapter) {
                guid = iface[ifaceNum].guid;
                guid = guid;
                break;
            }
        }
        if (!guid) {
            console.log("Cannot find wlan interface");
            reject();
            return;
        }
        profile = writeProfile(_ap);
        let profileContent = fs.readFileSync(profile, { encoding: 'utf8' });
        wifi_native.wlanConnect(guid, profileContent, _ap.ssid, (result) => {
            if (result == 1) {
                fs.unlinkSync(profile);
                let failedCount = 0
                let interval = setInterval(() => {
                    let ifStates = getIfaceState();
                    let ifState = ifStates.find(interface => interface.adapterName === adapter)
                    if (ifState.connection === "connected" || ifState.connection === "disconnected") {
                        if (failedCount > 20) {
                            console.log("Failed: ")
                            console.log(ifStates)
                            clearInterval(interval);
                            reject();
                        }
                        if (ifState.ssid === _ap.ssid) {
                            failedCount = 0;
                            clearInterval(interval)
                            resolve();
                        }
                        failedCount++;
                    }
                }, 250)

            }
            else {
                fs.unlinkSync(profile);
                reject();
            }
        })
    })
}

var disconnect = function (adapter) {
    return new Promise((resolve, reject) => {
        let iface = getIfaceState();
        let guid;
        for (let ifaceNum = 0; ifaceNum < iface.length; ifaceNum++) {
            if (iface[ifaceNum].adapterName == adapter) {
                if (iface[ifaceNum].connection == "connected") {
                    guid = iface[ifaceNum].guid;
                    // guid = "{" + guid + "}";
                    break;
                } else {
                    console.log("This interface hasn't connect any AP")
                    reject();
                    return;
                }
            }
        }
        if (!guid) {
            console.log("Cannot find wlan interface");
            reject();
            return;
        }
        wifi_native.wlanDisconnect(guid, (status) => {
            if (status == 1) {
                console.log("wlan disconnected");
                resolve();
            } else {
                console.log("disconnect failed");
                reject();
            }
        })
    });

}

module.exports = { init, scanAsync, getNetworkList, connect, disconnect, getIfaceStateNative, free, getIfaceState, scanSync, kill_scan_keep };