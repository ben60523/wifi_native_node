var wifi_native = require("../index");

wifi_native.init();


setTimeout(() => {
    wifi_native.disconnect("Wi-Fi 2").then(() => {
        wifi_native.free();
        console.log("disconnected")
    }).catch(() => {
        wifi_native.free();
        console.log("failed")
    })
}, 1000)