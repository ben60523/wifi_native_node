var wifi_native = require('bindings')('wifi_scan');

var init = function () {
    wifi_native.wlanInit();
};

var scan = function () {
    return new Promise((resolve, reject) => {
        wifi_native.wlanScan((MediCamNetWorks) => {
            if (MediCamNetWorks)
                resolve(MediCamNetWorks);
            else
                reject(MediCamNetWorks);
        });
    });
};

var free = function () {
    wifi_native.wlanFree()
}

module.exports = { init, scan, free };