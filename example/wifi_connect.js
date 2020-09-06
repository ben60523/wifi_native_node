var wifi_native = require("../index");

wifi_native.init();

let ap = {
    ssid: "HowsCoffee-2.4G",
    password: "87328292"
}
setTimeout(() => {
    wifi_native.scan().then((res) => {
        console.log(res);
        wifi_native.connect(ap, "Wi-Fi").then(() => {
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