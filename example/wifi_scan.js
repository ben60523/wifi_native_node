var wlanNative = require("../index");

wlanNative.init();

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