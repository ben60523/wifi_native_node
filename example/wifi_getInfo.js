var wlanNative = require("../index");

wlanNative.init();

let iface = wlanNative.getIfaceState()
console.log(iface)

wlanNative.free();