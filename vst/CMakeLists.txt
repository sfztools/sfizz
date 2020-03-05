set (VSTPLUGIN_PRJ_NAME "${PROJECT_NAME}_vst3")
set (VSTPLUGIN_BUNDLE_NAME "${PROJECT_NAME}.vst3")

set (VST3SDK_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/external/VST_SDK/VST3_SDK")
set (VST3SDK_ARCHIVE "vst-sdk_3.6.14_build-24_2019-11-29.zip")

if (NOT EXISTS "${VST3SDK_BASEDIR}")
    message (STATUS "VST3 SDK is not found, downloading")

    execute_process (
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/download")

    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/download/${VST3SDK_ARCHIVE}")
        file (DOWNLOAD
            "https://download.steinberg.net/sdk_downloads/${VST3SDK_ARCHIVE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/download/${VST3SDK_ARCHIVE}"
            SHOW_PROGRESS)
    endif()

    execute_process (
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/external"
        COMMAND "${CMAKE_COMMAND}" -E tar xvf "../download/${VST3SDK_ARCHIVE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/external")
endif()

# stop trying to include this atrocity.. build it ourselves
#add_subdirectory("${VST3SDK_BASEDIR}" EXCLUDE_FROM_ALL)

# VST plugin specific settings
include (VSTConfig)

configure_file (VstPluginDefs.h.in "${CMAKE_CURRENT_BINARY_DIR}/VstPluginDefs.h")

# Build VST3 SDK
include("cmake/Vst3.cmake")

# Build the plugin
add_library(${VSTPLUGIN_PRJ_NAME} MODULE
    SfizzVstProcessor.cpp
    SfizzVstController.cpp
    SfizzVstEditor.cpp
    SfizzVstState.cpp
    VstPluginFactory.cpp)
target_link_libraries(${VSTPLUGIN_PRJ_NAME}
    PRIVATE ${PROJECT_NAME}::${PROJECT_NAME})
target_include_directories(${VSTPLUGIN_PRJ_NAME}
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}"
    PREFIX "")

plugin_add_vst3sdk(${VSTPLUGIN_PRJ_NAME})
plugin_add_vstgui(${VSTPLUGIN_PRJ_NAME})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(${VSTPLUGIN_PRJ_NAME} PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/vst3.version")
endif()

# Create the bundle (see "VST 3 Locations / Format")
if(WIN32)
    set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/${VST3_PACKAGE_ARCHITECTURE}-win")
    # TODO: make desktop.ini, Plugin.ico
elseif(APPLE)
    set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/MacOS")
    # TODO: make Info.plist, PkgInfo
else()
    set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/${VST3_PACKAGE_ARCHITECTURE}-linux")
endif()

# To help debugging the link only
if (FALSE)
    target_link_options(${VSTPLUGIN_PRJ_NAME} PRIVATE "-Wl,-no-undefined")
endif()
