# Configuration for this plugin
# TODO: generate version from git
set (LV2PLUGIN_VERSION_MINOR   0)
set (LV2PLUGIN_VERSION_MICRO   1)
set (LV2PLUGIN_NAME            "sfizz")
set (LV2PLUGIN_COMMENT         "SFZ sampler")
set (LV2PLUGIN_URI             "http://sfztools.github.io/sfizz")
set (LV2PLUGIN_AUTHOR          "Paul Ferrand")
set (LV2PLUGIN_EMAIL           "paul at ferrand dot cc")
set (LV2PLUGIN_SPDX_LICENSE_ID "ISC")

if (WIN32)
    set (LV2PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lv2" CACHE STRING
    "Install destination for LV2 bundle [default: ${CMAKE_INSTALL_PREFIX}/lv2}]")
else()
    set (LV2PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/lv2" CACHE STRING
    "Install destination for LV2 bundle [default: ${CMAKE_INSTALL_PREFIX}/lib/lv2}]")
endif()
