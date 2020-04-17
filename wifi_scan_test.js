var wifi_scan = require("./index");

wifi_scan().then((results) => {
    console.log("MediCamNetworks: ", results);
}).catch((results) => {
    console.log("scan failed");
})