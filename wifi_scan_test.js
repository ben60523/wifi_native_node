var wifi_scan = require('bindings')('wifi_scan');

console.log(wifi_scan.hello()); // 'world'

console.log(wifi_scan.wifiscan());
