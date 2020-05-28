# Configuration for this plugin
# TODO: generate version from git
set (LV2PLUGIN_VERSION_MINOR   4)
set (LV2PLUGIN_VERSION_MICRO   0)
set (LV2PLUGIN_NAME            "sfizz")
set (LV2PLUGIN_COMMENT         "SFZ sampler")
set (LV2PLUGIN_URI             "http://sfztools.github.io/sfizz")
set (LV2PLUGIN_REPOSITORY      "https://github.com/sfztools/sfizz")
set (LV2PLUGIN_AUTHOR          "Paul Ferrand")
set (LV2PLUGIN_EMAIL           "paul@ferrand.cc")
if (SFIZZ_USE_VCPKG)
    set (LV2PLUGIN_SPDX_LICENSE_ID "LGPL-3.0-only")
else()
    set (LV2PLUGIN_SPDX_LICENSE_ID "ISC")
endif()

if(SFIZZ_LV2_UI)
    set(LV2PLUGIN_IF_ENABLE_UI "")
else()
    set(LV2PLUGIN_IF_ENABLE_UI "#")
endif()

if(WIN32)
    set(LV2_UI_TYPE "WindowsUI")
elseif(APPLE)
    set(LV2_UI_TYPE "CocoaUI")
elseif(HAIKU)
    set(LV2_UI_TYPE "BeUI")
else()
    set(LV2_UI_TYPE "X11UI")
endif()

if (MSVC)
    set (LV2PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lv2" CACHE STRING
    "Install destination for LV2 bundle [default: ${CMAKE_INSTALL_PREFIX}/lv2}]")
else()
    set (LV2PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/lv2" CACHE STRING
    "Install destination for LV2 bundle [default: ${CMAKE_INSTALL_PREFIX}/lib/lv2}]")
endif()
