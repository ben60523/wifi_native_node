var wifiNative = require("../index");
wifiNative.init();
wifiNative.scanSync().then(() => {
    console.log("Scan Complete");
});
console.log("Running...");
process.on("exit", () => {
    wifiNative.free();
})