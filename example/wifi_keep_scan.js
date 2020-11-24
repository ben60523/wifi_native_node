var wifiNative = require("../index");
wifiNative.init();

wifiNative.scan_keep();

setInterval(() => {
    wifiNative.getNetworkList().then((MediCamNetWorks) => {
        console.log(MediCamNetWorks);
    })
}, 7000)