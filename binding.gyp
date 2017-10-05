{
  "includes": [
    "common.gypi",
    "config.gypi"
  ],

  "variables": {
      # gyp does not appear to let you test for undefined variables, so define
      # lldb_build_dir as empty so we can test it later.
      "lldb_build_dir%": ""
  },

  "targets": [
    {
      "target_name": "llnode_module",
      "sources": [
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
            "libraries": ["<(lldb_lib_dir)/lib/liblldb.so.1" ],
            "ldflags": [
              "-Wl,-rpath,<(lldb_lib_dir)/lib",
              "-L<(lldb_lib_dir)/lib",
              "-l<(lldb_lib)"
            ]
          },
          # lldb_builddir != ""
          {
            "libraries": ["<(lldb_build_dir)/lib/liblldb.so.1" ],
            "ldflags": [
              "-Wl,-rpath,<(lldb_build_dir)/lib",
              "-L<(lldb_build_dir)/lib",
              "-l<(lldb_lib)"
            ]
          }]
        ]
      }]
    ],
      # "conditions": [
      #   ["OS=='mac'", {
      #     "make_global_settings": [
      #       ["CXX","/usr/bin/clang++"],
      #       ["LINK","/usr/bin/clang++"],
      #     ],
      #     "xcode_settings": {
      #       "OTHER_CPLUSPLUSFLAGS" : ["-stdlib=libc++"],
      #      },
      #      # cflags as recommended by 'llvm-config --cxxflags'
      #      "cflags": [
      #        "--enable-cxx11",
      #        "--enable-libcpp",
      #        "-std=c++11",
      #        "-stdlib=libc++",
      #        "-I/usr/local/opt/llvm36/lib/llvm-3.6/include",
      #        "-DNDEBUG -D_GNU_SOURCE",
      #        "-D__STDC_CONSTANT_MACROS",
      #        "-D__STDC_FORMAT_MACROS",
      #        "-D__STDC_LIMIT_MACROS"
      #        "-g",
      #        "-O2",
      #        "-fomit-frame-pointer"
      #        "-fvisibility-inlines-hidden",
      #        "-fno-exceptions",
      #        "-fPIC",
      #        "-ffunction-sections",
      #        "-fdata-sections",
      #        "-Wcast-qual",
      #       ],
      #      # ldflags as recommended by 'llvm-config --ldflags'
      #      "ldflags": [ "-L/usr/local/opt/llvm36/lib/llvm-3.6/lib -llldb-3.6", ],
      #   }],
      #   ["OS=='linux'", {
      #     "libraries": ["/usr/lib/llvm-3.9/lib/liblldb.so.1" ],
      #      # cflags as recommended by 'llvm-config --cxxflags'
      #      "cflags": [
      #        "-I/usr/lib/llvm-3.9/include",
      #        "-DNDEBUG -D_GNU_SOURCE",
      #        "-D__STDC_CONSTANT_MACROS",
      #        "-D__STDC_FORMAT_MACROS",
      #        "-D__STDC_LIMIT_MACROS",
      #        "-g",
      #        "-O2",
      #        "-fomit-frame-pointer",
      #        "-fvisibility-inlines-hidden",
      #        "-fno-exceptions",
      #        "-fPIC",
      #        "-ffunction-sections",
      #        "-fdata-sections",
      #        "-Wcast-qual",
      #       ],
      #      # ldflags as recommended by 'llvm-config --ldflags'
      #      "ldflags": [ "-L/usr/lib/llvm-3.9/lib -llldb-3.9", ],
      #   }],
      # ],
    },
    {
      "target_name": "install",
      "type":"none",
      "dependencies" : [ "llnode_module" ],
      "copies": [
        {
          "destination": "<(module_root_dir)",
          "files": ["<(module_root_dir)/build/Release/llnode_module.node"]
        }]
    },
  ],
}

