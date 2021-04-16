set(VST3SDK_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/external/VST_SDK/VST3_SDK")
#set(AUWRAPPER_BASEDIR "${VST3SDK_BASEDIR}/public.sdk/source/vst/auwrapper")
set(AUWRAPPER_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/external/sfzt_auwrapper")

# VST plugin specific settings
include(VSTConfig)

configure_file(VstPluginDefs.h.in "${CMAKE_CURRENT_BINARY_DIR}/VstPluginDefs.h")

# Build VST3 SDK
include("cmake/Vst3.cmake")

# Add the ring buffer
set(RINGBUFFER_HEADERS
    "external/ring_buffer/ring_buffer/ring_buffer.h"
    "external/ring_buffer/ring_buffer/ring_buffer.tcc")
add_library(sfizz-ring-buffer STATIC EXCLUDE_FROM_ALL
    "external/ring_buffer/ring_buffer/ring_buffer.cpp"
    ${RINGBUFFER_HEADERS})
source_group("Header Files" FILES ${RINGBUFFER_HEADERS})
target_include_directories(sfizz-ring-buffer PUBLIC "external/ring_buffer")
add_library(sfizz::ring-buffer ALIAS sfizz-ring-buffer)

# Build the VST shared code
add_library(sfizz-vst3-core STATIC EXCLUDE_FROM_ALL
    SfizzVstProcessor.cpp
    SfizzVstProcessor.h
    SfizzVstController.cpp
    SfizzVstController.h
    SfizzVstEditor.cpp
    SfizzVstEditor.h
    SfizzVstState.cpp
    SfizzVstState.h
    SfizzVstParameters.h
    SfizzVstUpdates.h
    SfizzVstUpdates.cpp
    SfizzVstIDs.h
    OrderedEventProcessor.h
    OrderedEventProcessor.hpp
    IdleUpdateHandler.h
    X11RunLoop.h
    X11RunLoop.cpp)
target_include_directories(sfizz-vst3-core
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(sfizz-vst3-core
    PRIVATE sfizz::sfizz
    PRIVATE sfizz::import
    PRIVATE sfizz::editor
    PRIVATE sfizz::plugins-common
    PRIVATE sfizz::pugixml sfizz::filesystem
    PRIVATE sfizz::ring-buffer
    PRIVATE absl::strings absl::optional absl::container_common
    vst3sdk
    vst3sdk_vstgui)
gw_target_warn(sfizz-vst3-core PRIVATE
    "-Wno-ignored-qualifiers"
    "-Wno-unused-parameter")
add_library(sfizz::vst3-core ALIAS sfizz-vst3-core)

# --- VST3 plugin --- #

if(SFIZZ_VST)
    set(VSTPLUGIN_PRJ_NAME "${PROJECT_NAME}_vst3")
    set(VSTPLUGIN_BUNDLE_NAME "${PROJECT_NAME}.vst3")

    add_library(${VSTPLUGIN_PRJ_NAME} MODULE "VstPluginFactory.cpp")
    if(WIN32)
        target_sources(${VSTPLUGIN_PRJ_NAME} PRIVATE vst3.def)
    endif()
    target_include_directories(${VSTPLUGIN_PRJ_NAME}
        PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(${VSTPLUGIN_PRJ_NAME}
        PRIVATE sfizz-vst3-core)
    set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES
        OUTPUT_NAME "${PROJECT_NAME}"
        PREFIX "")

    plugin_add_vst3sdk(${VSTPLUGIN_PRJ_NAME})
    plugin_add_vstgui(${VSTPLUGIN_PRJ_NAME})

    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_link_libraries(${VSTPLUGIN_PRJ_NAME} PRIVATE
            "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/vst3.version")
    endif()
    sfizz_enable_lto_if_needed(${VSTPLUGIN_PRJ_NAME})
    if(MINGW)
        set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES LINK_FLAGS "-static")
    endif()

    # Create the bundle(see "VST 3 Locations / Format")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/Resources")
    copy_editor_resources(
        ${VSTPLUGIN_PRJ_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/../editor/resources"
        "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/Resources")
    if(WIN32)
        set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES
            SUFFIX ".vst3"
            LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/${VST3_PACKAGE_ARCHITECTURE}-win/$<0:>")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/win/Plugin.ico"
            "${CMAKE_CURRENT_SOURCE_DIR}/win/desktop.ini"
            DESTINATION "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}")
    elseif(APPLE)
        set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES
            SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/MacOS/$<0:>")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mac/PkgInfo"
            DESTINATION "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents")
        set(SFIZZ_VST3_BUNDLE_EXECUTABLE "${PROJECT_NAME}")
        set(SFIZZ_VST3_BUNDLE_VERSION "${PROJECT_VERSION}")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.vst3.plist"
            "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/Info.plist" @ONLY)
    else()
        set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/${VST3_PACKAGE_ARCHITECTURE}-linux/$<0:>")
    endif()

    # Copy the license
    if(APPLE)
        # on macOS, files are not permitted at the bundle root, during code signing
        file(COPY "gpl-3.0.txt"
            DESTINATION "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/SharedSupport/License")
    else()
        file(COPY "gpl-3.0.txt"
            DESTINATION "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}")
    endif()

    # To help debugging the link only
    if(FALSE)
        target_link_options(${VSTPLUGIN_PRJ_NAME} PRIVATE "-Wl,-no-undefined")
    endif()

    # Installation
    if(NOT MSVC)
        install(DIRECTORY "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}"
            DESTINATION "${VSTPLUGIN_INSTALL_DIR}"
            COMPONENT "vst")
        bundle_dylibs(vst
            "${VSTPLUGIN_INSTALL_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/MacOS/sfizz"
            COMPONENT "vst")
    endif()
endif()

# --- Audio Unit wrapper --- #

if(SFIZZ_AU AND NOT APPLE)
    message(WARNING "Audio Unit is available only for macOS builds")
elseif(SFIZZ_AU)
    set(AUPLUGIN_PRJ_NAME "${PROJECT_NAME}_au")
    set(AUPLUGIN_BUNDLE_NAME "${PROJECT_NAME}.component")

    add_library(${AUPLUGIN_PRJ_NAME} MODULE
        "VstPluginFactory.cpp"
        "${AUWRAPPER_BASEDIR}/aucarbonview.mm"
        "${AUWRAPPER_BASEDIR}/aucocoaview.mm"
        "${AUWRAPPER_BASEDIR}/ausdk.mm"
        "${AUWRAPPER_BASEDIR}/auwrapper.mm"
        "${AUWRAPPER_BASEDIR}/NSDataIBStream.mm")
    target_include_directories(${AUPLUGIN_PRJ_NAME} PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${VST3SDK_BASEDIR}")
    target_link_libraries(${AUPLUGIN_PRJ_NAME} PRIVATE
        "${APPLE_FOUNDATION_LIBRARY}"
        "${APPLE_COCOA_LIBRARY}"
        "${APPLE_CARBON_LIBRARY}"
        "${APPLE_AUDIOTOOLBOX_LIBRARY}"
        "${APPLE_AUDIOUNIT_LIBRARY}"
        "${APPLE_COREAUDIO_LIBRARY}"
        "${APPLE_COREMIDI_LIBRARY}")

    target_link_libraries(${AUPLUGIN_PRJ_NAME}
        PRIVATE sfizz-vst3-core)
    set_target_properties(${AUPLUGIN_PRJ_NAME} PROPERTIES
        OUTPUT_NAME "${PROJECT_NAME}"
        PREFIX "")

    plugin_add_vst3sdk(${AUPLUGIN_PRJ_NAME})
    plugin_add_vstgui(${AUPLUGIN_PRJ_NAME})

    # Get Core Audio utility classes if missing
    set(CA_UTILITY_BASEDIR
        "${CMAKE_CURRENT_SOURCE_DIR}/external/CoreAudioUtilityClasses")
    if(EXISTS "${CA_UTILITY_BASEDIR}")
        message(STATUS "The CoreAudioUtilityClasses are available locally")
    else()
        message(STATUS "The CoreAudioUtilityClasses are not available locally")

        set(CA_UTILITY_VERSION "1.1")
        set(CA_UTILITY_ARCHIVE "CoreAudioUtilityClasses-${CA_UTILITY_VERSION}.tar.gz")
        set(CA_UTILITY_DOWNLOAD_URL "https://github.com/sfztools/CoreAudioUtilityClasses/releases/download/v${CA_UTILITY_VERSION}/${CA_UTILITY_ARCHIVE}")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/download/${CA_UTILITY_ARCHIVE}")
            message(STATUS "The CoreAudioUtilityClasses archive is available")
        else()
            message(STATUS "The CoreAudioUtilityClasses archive is missing")
            message(STATUS "Downloading: ${CA_UTILITY_DOWNLOAD_URL}")

            file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/download")
            file(DOWNLOAD "${CA_UTILITY_DOWNLOAD_URL}"
                "${CMAKE_CURRENT_SOURCE_DIR}/download/${CA_UTILITY_ARCHIVE}")
        endif()
        message(STATUS "Extracting: ${CA_UTILITY_ARCHIVE}")
        execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "tar" "xvf"
            "${CMAKE_CURRENT_SOURCE_DIR}/download/${CA_UTILITY_ARCHIVE}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/external")
    endif()

    # Add Core Audio utility classes
    target_include_directories(${AUPLUGIN_PRJ_NAME} PRIVATE
        "${CA_UTILITY_BASEDIR}/CoreAudio"
        "${CA_UTILITY_BASEDIR}/CoreAudio/AudioUnits"
        "${CA_UTILITY_BASEDIR}/CoreAudio/AudioUnits/AUPublic/AUBase"
        "${CA_UTILITY_BASEDIR}/CoreAudio/AudioUnits/AUPublic/Utility"
        "${CA_UTILITY_BASEDIR}/CoreAudio/PublicUtility")

    # Add VST hosting classes
    target_link_libraries(${AUPLUGIN_PRJ_NAME} PRIVATE vst3sdk_hosting)

    # Add generated source
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
    target_include_directories(${AUPLUGIN_PRJ_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include")
    string(TIMESTAMP SFIZZ_AU_CLASS_PREFIX_NUMBER "%s" UTC)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/aucocoaclassprefix.h"
        "#define SMTG_AU_NAMESPACE SMTGAUCocoa${SFIZZ_AU_CLASS_PREFIX_NUMBER}_")

    sfizz_enable_lto_if_needed(${AUPLUGIN_PRJ_NAME})

    # Create the bundle
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/Resources")
    copy_editor_resources(
        ${AUPLUGIN_PRJ_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/../editor/resources"
        "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/Resources")
    set_target_properties(${AUPLUGIN_PRJ_NAME} PROPERTIES
        SUFFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/MacOS/$<0:>")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mac/PkgInfo"
        DESTINATION "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents")
    set(SFIZZ_AU_BUNDLE_EXECUTABLE "${PROJECT_NAME}")
    set(SFIZZ_AU_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(SFIZZ_AU_BUNDLE_IDENTIFIER "tools.sfz.sfizz.au")
    set(SFIZZ_AU_BUNDLE_TYPE "aumu")
    set(SFIZZ_AU_BUNDLE_SUBTYPE "samp")
    set(SFIZZ_AU_BUNDLE_MANUFACTURER "Sfzt")
    math(EXPR SFIZZ_AU_DECIMAL_VERSION
        "${PROJECT_VERSION_MAJOR}*10000 + ${PROJECT_VERSION_MINOR}*100 + ${PROJECT_VERSION_PATCH}")
    execute_process(
        COMMAND "sh" "-c" "echo 'obase=16;${SFIZZ_AU_DECIMAL_VERSION}' | bc"
        OUTPUT_VARIABLE SFIZZ_AU_HEXADECIMAL_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.au.plist"
        "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/Info.plist" @ONLY)

    file(COPY "gpl-3.0.txt"
        DESTINATION "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/SharedSupport/License")

    # Add the resource fork
    if(FALSE)
        execute_process(COMMAND "xcrun" "--find" "Rez"
            OUTPUT_VARIABLE OSX_REZ_COMMAND OUTPUT_STRIP_TRAILING_WHITESPACE)
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/mac/audiounitconfig.h.in"
            "${CMAKE_CURRENT_BINARY_DIR}/include/audiounitconfig.h" @ONLY)
        add_custom_command(TARGET ${AUPLUGIN_PRJ_NAME} POST_BUILD COMMAND
            "${OSX_REZ_COMMAND}"
            "-d" "SystemSevenOrLater=1"
            "-script" "Roman"
            "-d" "i386_YES"
            "-d" "x86_64_YES"
            "-is" "${CMAKE_OSX_SYSROOT}"
            "-I" "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/A/Headers"
            "-I" "/System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/A/Headers"
            "-I" "/System/Library/Frameworks/AudioUnit.framework/Versions/A/Headers/"
            "-I" "${CA_UTILITY_BASEDIR}/CoreAudio/AudioUnits/AUPublic/AUBase"
            "-I" "${CMAKE_CURRENT_BINARY_DIR}/include" # generated audiounitconfig.h
            "-o" "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/Resources/${PROJECT_NAME}.rsrc"
            "-useDF"
            "${AUWRAPPER_BASEDIR}/auresource.r")
    endif()

    # Installation
    if(AUPLUGIN_INSTALL_DIR)
        install(DIRECTORY "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}"
            DESTINATION "${AUPLUGIN_INSTALL_DIR}"
            COMPONENT "au")
        bundle_dylibs(au
            "${AUPLUGIN_INSTALL_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/MacOS/sfizz"
            COMPONENT "au")
    endif()
endif()

# --- VST2 wrapper --- #

if(SFIZZ_VST2)
    set(VST2PLUGIN_PRJ_NAME "${PROJECT_NAME}_vst2")
    set(VST2PLUGIN_BUNDLE_NAME "${PROJECT_NAME}.vst2")

    set(VST2SDK_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/external/vstsdk2.4")
    set(VST2WRAPPER_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/external/VST_SDK/VST3_SDK/public.sdk/source/vst/vst2wrapper")

    if(NOT EXISTS "${VST2SDK_BASEDIR}/pluginterfaces/vst2.x/aeffect.h")
        message(FATAL_ERROR "VST SDK 2.4 is missing. Make it available in the following folder: ${VST2SDK_BASEDIR}")
    endif()

    add_library(${VST2PLUGIN_PRJ_NAME} MODULE
        "VstPluginFactory.cpp"
        "Vst2PluginFactory.cpp"
        "Vst2PluginEntry.c"
        "${VST2WRAPPER_BASEDIR}/vst2wrapper.cpp"
        "${VST2WRAPPER_BASEDIR}/../basewrapper/basewrapper.cpp"
        "${VST2SDK_BASEDIR}/public.sdk/source/vst2.x/audioeffect.cpp"
        "${VST2SDK_BASEDIR}/public.sdk/source/vst2.x/audioeffectx.cpp")
    target_include_directories(${VST2PLUGIN_PRJ_NAME} PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${VST2SDK_BASEDIR}")

    if(NOT WIN32)
        target_compile_definitions(${VST2PLUGIN_PRJ_NAME} PRIVATE "__cdecl=")
    endif()

    target_link_libraries(${VST2PLUGIN_PRJ_NAME}
        PRIVATE sfizz-vst3-core)
    set_target_properties(${VST2PLUGIN_PRJ_NAME} PROPERTIES
        OUTPUT_NAME "${PROJECT_NAME}"
        PREFIX "")

    plugin_add_vst3sdk(${VST2PLUGIN_PRJ_NAME})
    plugin_add_vstgui(${VST2PLUGIN_PRJ_NAME})

    # Add VST hosting classes
    target_link_libraries(${VST2PLUGIN_PRJ_NAME} PRIVATE vst3sdk_hosting)

    sfizz_enable_lto_if_needed(${VST2PLUGIN_PRJ_NAME})

    # Create the bundle
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/${VST2PLUGIN_BUNDLE_NAME}/Contents/Resources")
    copy_editor_resources(
        ${VST2PLUGIN_PRJ_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/../editor/resources"
        "${PROJECT_BINARY_DIR}/${VST2PLUGIN_BUNDLE_NAME}/Contents/Resources")
    set_target_properties(${VST2PLUGIN_PRJ_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${VST2PLUGIN_BUNDLE_NAME}/Contents/Binary/$<0:>")

    file(COPY "gpl-3.0.txt"
        DESTINATION "${PROJECT_BINARY_DIR}/${VST2PLUGIN_BUNDLE_NAME}")

    if(MINGW)
        set_target_properties(${VST2PLUGIN_PRJ_NAME} PROPERTIES LINK_FLAGS "-static")
    endif()

    # Installation
    if(VST2PLUGIN_INSTALL_DIR)
        install(DIRECTORY "${PROJECT_BINARY_DIR}/${VST2PLUGIN_BUNDLE_NAME}"
            DESTINATION "${VST2PLUGIN_INSTALL_DIR}"
            COMPONENT "vst2")
        if(APPLE)
            bundle_dylibs(vst2
                "${VST2PLUGIN_INSTALL_DIR}/${VST2PLUGIN_BUNDLE_NAME}/Contents/Binary/sfizz.${CMAKE_SHARED_MODULE_SUFFIX}"
                COMPONENT "vst2")
        endif()
    endif()
endif()
