var wifi_native = require('bindings')('wifi_scan');

var scan = function () {
    return new Promise((resolve, reject) => {
        wifi_native.wifiscanCb((MediCamNetWorks) => {
            if (MediCamNetWorks)
                resolve(MediCamNetWorks);
            else 
                reject(MediCamNetWorks);
        });
    });
};

module.exports = scan;

