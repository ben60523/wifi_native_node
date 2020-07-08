var wifi_native = require('bindings')('wifi_native');
var wifiControl = require("wifi-control");
var fs = require("fs");
var path = require("path");
var { Worker } = require("worker_threads");
var assert = require("assert")
let moduleDirname = path.dirname(module.filename);

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
        let options = {
            debug: true,
            connectionTimeout: 2000
        }
        if (wifiControl.init(options).interface == "wlan") {
            if (wifi_native.wlanInit() == 0) {
                return true;
            }
            else {
                return false;
            }
        } else {
            return false;
        }
    };
    let resp = await initialize();
    assert(resp == true);
}

var getNetworkList = function () {
    return new Promise((resolve, reject) => {
        wifi_native.wlanGetNetworkList((APs) => {
            if (APs) {
                resolve(APs);
            } else {
                reject();
            }
        })
    });
}

var scan = function () {
    return new Promise((resolve, reject) => {
        let scan_worker = new Worker(path.resolve(moduleDirname, "services/scan_service.js"));
        scan_worker.on("message", (result) => {
            resolve(result);
        });
        scan_worker.on("err", (err) => {
            reject(err);
        });
        scan_worker.on('exit', (code) => {
            if (code !== 0)
                reject(new Error(`Worker stopped with exit code ${code}`));
        })
    });
};

var free = function () {
    wifi_native.wlanFree()
}

var connect = function (_ap, adapter) {
    return new Promise((resolve, reject) => {
        let iface = wifiControl.getIfaceState();
        let guid, profile;
        for (let ifaceNum = 0; ifaceNum < iface.length; ifaceNum++) {
            if (iface[ifaceNum].adapterName == adapter) {
                guid = iface[ifaceNum].guid;
                guid = "{" + guid + "}";
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
        let connect_worker = new Worker(path.resolve(moduleDirname, "services/connect_service.js"), { workerData: { ap: _ap, GUID: guid, profileContent: profileContent } });
        let adapterName = adapter;
        connect_worker.on("message", (msg) => {
            if (msg == "ok") {
                fs.unlinkSync(profile);
                let failedCount = 0;
                let interval = setInterval(() => {
                    let ifStates = wifiControl.getIfaceState();
                    let ifState = ifStates.find(interface => interface.adapterName === adapterName)
                    if (ifStates.success && ((ifState.connection === "connected") || (ifState.connection === "disconnected"))) {
                        if (failedCount > 20) {
                            clearInterval(interval);
                            reject();
                        }
                        if (ifState.connection === "connected" && ifState.ssid === _ap.ssid) {
                            failedCount = 0;
                            clearInterval(interval)
                            resolve();
                        }
                        failedCount++;
                    }
                }, 250)
            } else {
                fs.unlinkSync(profile);
                reject();
            }
        })
    })
}

var disconnect = function (adapter) {
    return new Promise((resolve, reject) => {
        let iface = wifiControl.getIfaceState();
        let guid;
        for (let ifaceNum = 0; ifaceNum < iface.length; ifaceNum++) {
            if (iface[ifaceNum].adapterName == adapter) {
                if (iface[ifaceNum].connection == "connected") {
                    guid = iface[ifaceNum].guid;
                    guid = "{" + guid + "}";
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

module.exports = { init, scan, getNetworkList, connect, disconnect, free };