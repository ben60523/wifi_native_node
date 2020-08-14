var wlanNative = require("../index");

wlanNative.init();

wlanNative.getIfaceStateNative().then((ifaceList) => {
    console.log("info", ifaceList);
    wlanNative.free();
}).catch((e) => {
    console.error("error", e);
    wlanNative.free();
})