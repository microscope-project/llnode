{
  "includes": [
    "common.gypi",
    "options.gypi"
  ],

  "variables": {
    # gyp does not appear to let you test for undefined variables, so define
    # lldb_build_dir as empty so we can test it later.
    # this variable is used when we don't link with frameworks on Macos
    "lldb_build_dir%": ""
  },

  "targets": [{
    "target_name": "addon",
    "sources": [
      "src/addon.cc",
      "src/llnode_module.cc",
      "src/llnode_api.cc",
      "src/llv8.cc",
      "src/llv8-constants.cc",
      "src/llscan.cc"
    ],
    "include_dirs": [
      ".",
      "<(lldb_dir)/include",
      "<!(node -e \"require('nan')\")"
    ],
    "conditions": [
      [ "OS == 'mac'", {
        "conditions": [
          [ "lldb_build_dir == ''", {
            "variables": {
              "mac_shared_frameworks": "/Applications/Xcode.app/Contents/SharedFrameworks",
            },
            "xcode_settings": {
              "OTHER_LDFLAGS": [
                "-F<(mac_shared_frameworks)",
              "-Wl,-rpath,<(mac_shared_frameworks)",
              "-framework LLDB",
              ],
            },
          },
          # lldb_builddir != ""
          {
            "xcode_settings": {
              "OTHER_LDFLAGS": [
                "-Wl,-rpath,<(lldb_build_dir)/lib",
                "-L<(lldb_build_dir)/lib",
                "-l<(lldb_lib)",
              ],
            },
          }],
        ],
      }],
      [ "OS=='linux'", {
        "conditions": [
          [ "lldb_build_dir == ''", {
            "libraries": ["<(lldb_dir)/lib/liblldb.so.1" ],
            "ldflags": [
              "-Wl,-rpath,<(lldb_dir)/lib",
              "-L<(lldb_dir)/lib",
              "-l<(lldb_lib)",
            ]
          },
          # lldb_builddir != ""
          {
            "libraries": ["<(lldb_build_dir)/lib/liblldb.so.1" ],
            "ldflags": [
              "-Wl,-rpath,<(lldb_build_dir)/lib",
              "-L<(lldb_build_dir)/lib",
              "-l<(lldb_lib)",
            ]
          }
        ],
      ]
     }]
    ]
  },
  {
    "target_name": "install",
    "type":"none",
    "dependencies" : [ "addon" ],
    "copies": [
    {
      "destination": "<(module_root_dir)",
      "files": ["<(module_root_dir)/build/Release/addon.node"]
    }]
  },
  ],
}

