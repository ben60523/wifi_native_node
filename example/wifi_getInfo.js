const os = require('os')
var wlanNative = require("../index");
if (os.platform() === 'win32') {
  wlanNative.init();

  let iface = wlanNative.getIfaceState()
  console.log(iface)
  
  wlanNative.free();
} else if (os.platform() === 'darwin') {
  console.log(wlanNative.getWifiStatus());
}
