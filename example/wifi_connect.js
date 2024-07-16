var os = require("os");
var wifi_native = require("../index");

let ap = {
  ssid: "Sharedfield",
  password: "25175089",
};

if (os.platform() === "win32") {
  wifi_native.init();
  setTimeout(() => {
    wifi_native
      .scanSync()
      .then((res) => {
        console.log(res);
        wifi_native
          .connect(ap, "Wi-Fi 2")
          .then(() => {
            console.log("connection success");
            wifi_native.free();
          })
          .catch(() => {
            console.log("connection failed");
            wifi_native.free();
          });
      })
      .catch((res) => {
        console.log(res);
        console.log("scan failed");
      });
  }, 1000);
} else {
  wifi_native
    .connectDarwin(ap.ssid, ap.password)
    .then(() => {
      console.log("Connection success");
    })
    .catch(() => {
      console.log("Connection failed");
    });
}
