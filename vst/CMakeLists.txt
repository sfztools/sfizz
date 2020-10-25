set (VSTPLUGIN_PRJ_NAME "${PROJECT_NAME}_vst3")
set (VSTPLUGIN_BUNDLE_NAME "${PROJECT_NAME}.vst3")

set (VST3SDK_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/external/VST_SDK/VST3_SDK")

# VST plugin specific settings
include (VSTConfig)

configure_file (VstPluginDefs.h.in "${CMAKE_CURRENT_BINARY_DIR}/VstPluginDefs.h")

# Build VST3 SDK
include("cmake/Vst3.cmake")

# Build the plugin
set(VSTPLUGIN_SOURCES
    SfizzVstProcessor.cpp
    SfizzVstController.cpp
    SfizzVstEditor.cpp
    SfizzVstState.cpp
    VstPluginFactory.cpp
    X11RunLoop.cpp)

set(VSTPLUGIN_HEADERS
    SfizzVstProcessor.h
    SfizzVstController.h
    SfizzVstEditor.h
    SfizzVstState.h
    X11RunLoop.h)

add_library(${VSTPLUGIN_PRJ_NAME} MODULE
    ${VSTPLUGIN_HEADERS}
    ${VSTPLUGIN_SOURCES})

if(WIN32)
    target_sources(${VSTPLUGIN_PRJ_NAME} PRIVATE vst3.def)
endif()
target_link_libraries(${VSTPLUGIN_PRJ_NAME}
    PRIVATE ${PROJECT_NAME}::${PROJECT_NAME}
    PRIVATE sfizz_editor)
target_include_directories(${VSTPLUGIN_PRJ_NAME}
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}"
    PREFIX "")

plugin_add_vst3sdk(${VSTPLUGIN_PRJ_NAME})
plugin_add_vstgui(${VSTPLUGIN_PRJ_NAME})

# Add the ring buffer
set (RINGBUFFER_HEADERS
    "external/ring_buffer/ring_buffer/ring_buffer.h"
    "external/ring_buffer/ring_buffer/ring_buffer.tcc")
source_group ("Header Files" FILES ${RINGBUFFER_HEADERS})
target_include_directories(${VSTPLUGIN_PRJ_NAME} PRIVATE "external/ring_buffer")
target_sources(${VSTPLUGIN_PRJ_NAME} PRIVATE
    "external/ring_buffer/ring_buffer/ring_buffer.cpp"
    ${RINGBUFFER_HEADERS})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(${VSTPLUGIN_PRJ_NAME} PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/vst3.version")
endif()
sfizz_enable_lto_if_needed (${VSTPLUGIN_PRJ_NAME})
if (MINGW)
    set_target_properties (${VSTPLUGIN_PRJ_NAME} PROPERTIES LINK_FLAGS "-static")
endif()

# Create the bundle (see "VST 3 Locations / Format")
execute_process (
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/Resources")
copy_editor_resources(
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
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mac/Plugin.icns"
        DESTINATION "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/Resources")
else()
    set_target_properties(${VSTPLUGIN_PRJ_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/${VST3_PACKAGE_ARCHITECTURE}-linux/$<0:>")
endif()

file(COPY "gpl-3.0.txt"
    DESTINATION "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}")

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${VSTPLUGIN_PRJ_NAME} PRIVATE
        "-Wno-extra"
        "-Wno-multichar"
        "-Wno-reorder"
        "-Wno-class-memaccess"
        "-Wno-ignored-qualifiers"
        "-Wno-unknown-pragmas"
        "-Wno-unused-function"
        "-Wno-unused-parameter"
        "-Wno-unused-variable")
endif()

# To help debugging the link only
if (FALSE)
    target_link_options(${VSTPLUGIN_PRJ_NAME} PRIVATE "-Wl,-no-undefined")
endif()

# Installation
if (NOT MSVC)
    install(DIRECTORY "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}"
        DESTINATION "${VSTPLUGIN_INSTALL_DIR}"
        COMPONENT "vst")
    bundle_dylibs(vst
        "${VSTPLUGIN_INSTALL_DIR}/${VSTPLUGIN_BUNDLE_NAME}/Contents/MacOS/sfizz"
        COMPONENT "vst")
endif()

# --- Audio Unit wrapper --- #

if(SFIZZ_AU AND NOT APPLE)
    message(WARNING "Audio Unit is available only for macOS builds")
elseif(SFIZZ_AU)
    set(AUPLUGIN_PRJ_NAME "${PROJECT_NAME}_au")
    set(AUPLUGIN_BUNDLE_NAME "${PROJECT_NAME}.component")

    add_library(${AUPLUGIN_PRJ_NAME} MODULE
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/auwrapper/aucarbonview.mm"
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/auwrapper/aucocoaview.mm"
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/auwrapper/ausdk.mm"
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/auwrapper/auwrapper.mm"
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/auwrapper/NSDataIBStream.mm")
    target_include_directories(${AUPLUGIN_PRJ_NAME} PRIVATE
        "${VST3SDK_BASEDIR}")
    target_link_libraries(${AUPLUGIN_PRJ_NAME} PRIVATE
        "${APPLE_FOUNDATION_LIBRARY}"
        "${APPLE_COCOA_LIBRARY}"
        "${APPLE_CARBON_LIBRARY}"
        "${APPLE_AUDIOTOOLBOX_LIBRARY}"
        "${APPLE_AUDIOUNIT_LIBRARY}"
        "${APPLE_COREAUDIO_LIBRARY}"
        "${APPLE_COREMIDI_LIBRARY}")

    set_target_properties(${AUPLUGIN_PRJ_NAME} PROPERTIES
        OUTPUT_NAME "${PROJECT_NAME}"
        PREFIX "")

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

    # Add VST base classes
    target_sources(${AUPLUGIN_PRJ_NAME} PRIVATE
        "${VST3SDK_BASEDIR}/base/source/baseiids.cpp"
        "${VST3SDK_BASEDIR}/base/source/fbuffer.cpp"
        "${VST3SDK_BASEDIR}/base/source/fdebug.cpp"
        "${VST3SDK_BASEDIR}/base/source/fdynlib.cpp"
        "${VST3SDK_BASEDIR}/base/source/fobject.cpp"
        "${VST3SDK_BASEDIR}/base/source/fstreamer.cpp"
        "${VST3SDK_BASEDIR}/base/source/fstring.cpp"
        "${VST3SDK_BASEDIR}/base/source/timer.cpp"
        "${VST3SDK_BASEDIR}/base/source/updatehandler.cpp"
        "${VST3SDK_BASEDIR}/base/thread/source/fcondition.cpp"
        "${VST3SDK_BASEDIR}/base/thread/source/flock.cpp"
        "${VST3SDK_BASEDIR}/pluginterfaces/base/conststringtable.cpp"
        "${VST3SDK_BASEDIR}/pluginterfaces/base/coreiids.cpp"
        "${VST3SDK_BASEDIR}/pluginterfaces/base/funknown.cpp"
        "${VST3SDK_BASEDIR}/pluginterfaces/base/ustring.cpp"
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstinitiids.cpp")

    # Add VST hosting classes
    target_sources(${AUPLUGIN_PRJ_NAME} PRIVATE
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/eventlist.cpp"
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/hostclasses.cpp"
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/parameterchanges.cpp"
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/pluginterfacesupport.cpp"
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/processdata.cpp")

    # Add generated source
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
    target_include_directories(${AUPLUGIN_PRJ_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include")
    string(TIMESTAMP SFIZZ_AU_CLASS_PREFIX_NUMBER "%s" UTC)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/aucocoaclassprefix.h"
        "#define SMTG_AU_NAMESPACE SMTGAUCocoa${SFIZZ_AU_CLASS_PREFIX_NUMBER}_")

    sfizz_enable_lto_if_needed (${AUPLUGIN_PRJ_NAME})

    # Create the bundle
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/Resources")
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
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mac/Plugin.icns"
        DESTINATION "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/Resources")

    file(COPY "gpl-3.0.txt"
        DESTINATION "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}")

    # Add the resource fork
    if (TRUE)
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
            "${VST3SDK_BASEDIR}/public.sdk/source/vst/auwrapper/auresource.r")
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${AUPLUGIN_PRJ_NAME} PRIVATE
            "-Wno-extra"
            "-Wno-multichar"
            "-Wno-reorder"
            "-Wno-class-memaccess"
            "-Wno-ignored-qualifiers"
            "-Wno-unknown-pragmas"
            "-Wno-unused-function"
            "-Wno-unused-parameter"
            "-Wno-unused-variable")
    endif()

    # Installation
    if(AUPLUGIN_INSTALL_DIR)
        install(DIRECTORY "${PROJECT_BINARY_DIR}/${AUPLUGIN_BUNDLE_NAME}"
            DESTINATION "${AUPLUGIN_INSTALL_DIR}"
            COMPONENT "au")
        install(DIRECTORY "${PROJECT_BINARY_DIR}/${VSTPLUGIN_BUNDLE_NAME}/"
            DESTINATION "${AUPLUGIN_INSTALL_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/Resources/plugin.vst3"
            COMPONENT "au")
        bundle_dylibs(au
            "${AUPLUGIN_INSTALL_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/MacOS/sfizz"
            COMPONENT "au")
        bundle_dylibs(au-vst
            "${AUPLUGIN_INSTALL_DIR}/${AUPLUGIN_BUNDLE_NAME}/Contents/Resources/plugin.vst3/Contents/MacOS/sfizz"
            COMPONENT "au")
    endif()
endif()
