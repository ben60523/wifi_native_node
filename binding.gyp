{
  "targets": [
    {
      "target_name": "wifi_native",
      "sources": [
        "src/WlanApiClass.cc",
        "wifi_native.cc" 
        ],
      "libraries": [
        "-ldbghelp.lib",
        "-lWS2_32.lib",
        "-ladvapi32.lib",
        "-luser32.lib",
        "-lIPHLPAPI.lib",
        "-lPSAPI.lib",
        "-lUSERENV.lib",
        "-lCRYPT32.lib",
        "-lbcrypt.lib",
        "-lkernel32.lib",
        "-lWINMM.lib"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "-L<$(HOMEPATH)AppData\\Local\\node-gyp\\Cache\\12.13.0\\include\\node",
        "src"
      ],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ]
    }
  ]
}
