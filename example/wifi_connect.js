var wifi_native = require("../index");

wifi_native.init();

let ap = {
    ssid: "ap_ssid",
    password: "ap_pw"
}
setTimeout(() => {
    wifi_native.scan().then((res) => {
        console.log(res);
        wifi_native.connect(ap, "Wi-Fi 5").then(() => {
            console.log("connection success");
            wifi_native.free();
        }).catch(() => {
            console.log("connection failed");
            wifi_native.free();
        });
    }).catch(() => {
        console.log("scan failed");
    })
}, 1000);