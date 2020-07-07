var wifi_native = require("../index");

wifi_native.init();

let ap = {
    ssid: "your_ap_ssid",
    password: "your_ap_password"
}
setTimeout(() => {
    wifi_native.scan().then((res) => {
        console.log(res);
        wifi_native.connect(ap, "Wi-Fi 2").then(() => {
            console.log("connection success");
            wifi_native.free();
        }).catch(() => {
            console.log("connection failed");
            wifi_native.free();
        });
    }).catch((res) => {
        console.log(res)
        console.log("scan failed");
    })
}, 1000);