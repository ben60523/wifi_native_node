var { workerData, parentPort } = require("worker_threads");
var wifi_native = require("bindings")("wifi_native");

let _ap = workerData.ap;
let guid = workerData.GUID;
let profileContent = workerData.profileContent;

wifi_native.wlanConnect(guid, profileContent, _ap.ssid, (result) => {
    if (result == 1) {
        parentPort.postMessage("ok");
    } else {
        parentPort.postMessage("failed");
    }
});