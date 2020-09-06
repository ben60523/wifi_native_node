var wifiNative = require("../index");
wifiNative.init();

setTimeout(() => {
    wifiNative.scan().then((list) => {
        console.log(list);
        wifiNative.free();
    }).catch((e) => {
        console.log("scan err: ", e);
        wifiNative.free();
    })
}, 1000)

