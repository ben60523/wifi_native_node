var wlanNative = require("../index");

wlanNative.init();
setTimeout(() => {
    wlanNative.scan().then((results) => {
        console.log("MediCamNetworks: ", results);
        setTimeout(() => {
            wlanNative.free();
        }, 1000);
    }).catch((results) => {
        console.log("scan failed");
        setTimeout(() => {
            wlanNative.free();
        }, 1000);
    })
}, 1000);
