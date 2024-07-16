var wifiNative = require("../index");
var os = require("os");

if (os.platform() === "win32") {
  wifiNative.init();

  setTimeout(() => {
    wifiNative
      .scanAsync()
      .then((list) => {
        // console.log(list);
        wifiNative.free();
      })
      .catch((e) => {
        console.log("scan err: ", e);
        wifiNative.free();
      });
  }, 1000);
} else if (os.platform() === "darwin") {
  wifiNative
    .scanDarwin()
    .then((list) => {
      console.log(list);
    })
    .catch((e) => {
      console.log(e);
    });
}
