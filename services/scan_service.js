var wifi_native = require("bindings")("wifi_native");
var { parentPort } = require("worker_threads");

wifi_native.wlanScan((status) => {
    if (status == 1) {
        wifi_native.wlanGetNetworkList((MediCamNetWorks) => {
            if (Array.isArray(MediCamNetWorks)) {
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
                parentPort.postMessage(MediCamNetWorks);
            }
            else
                parentPort.postMessage(MediCamNetWorks);
        })
    } else {
        parentPort.postMessage("Scan Failed");
    }
});