var wifi_native = require('bindings')('wifi_native');
var EventsEmitter = require("events").EventEmitter;
let event = new EventsEmitter();
console.log("start")
event.on("scan_ok", () => {
    console.log("ok~")
})

if (wifi_native.wlanInit() == 0) {
    wifi_native.wlanScan((flag) => {
        if (flag == 0) {
            console.log("good");
            wifi_native.wlanListener(event.emit.bind(event));
        } else
            console.log("not good")
    })
} else {
    console.log("Rrrrr")
}