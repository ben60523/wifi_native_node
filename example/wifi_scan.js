var wifiNative = require("../index");
wifiNative.init();

setTimeout(() => {
    wifiNative.scanAsync().then((list) => {
        // console.log(list);
        wifiNative.free();
    }).catch((e) => {
        console.log("scan err: ", e);
        wifiNative.free();
    })
}, 1000)

