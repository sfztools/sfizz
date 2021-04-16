find_package(Threads REQUIRED)

# --- VST3SDK ---
add_library(vst3sdk STATIC EXCLUDE_FROM_ALL
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
    "${VST3SDK_BASEDIR}/public.sdk/source/common/commoniids.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/common/memorystream.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/common/pluginview.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/main/pluginfactory.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstaudioeffect.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstbus.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstcomponent.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstcomponentbase.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vsteditcontroller.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstinitiids.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstnoteexpressiontypes.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstparameters.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstpresetfile.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstrepresentation.cpp")
if(WIN32)
    target_sources(vst3sdk PRIVATE
        "${VST3SDK_BASEDIR}/public.sdk/source/common/threadchecker_win32.cpp")
elseif(APPLE)
else()
    target_sources(vst3sdk PRIVATE
        "${VST3SDK_BASEDIR}/public.sdk/source/common/threadchecker_linux.cpp")
endif()
target_include_directories(vst3sdk PUBLIC "${VST3SDK_BASEDIR}")
target_link_libraries(vst3sdk PUBLIC Threads::Threads)
if(APPLE)
    target_link_libraries(vst3sdk PUBLIC ${APPLE_FOUNDATION_LIBRARY})
endif()
if(MINGW)
    target_compile_definitions(vst3sdk PUBLIC
        "_NATIVE_WCHAR_T_DEFINED=1" "__wchar_t=wchar_t")
endif()
set(_vst_release_build_types MinSizeRel Release RelWithDebInfo)
if(CMAKE_BUILD_TYPE IN_LIST _vst_release_build_types)
    target_compile_definitions(vst3sdk PUBLIC "RELEASE")
else()
    target_compile_definitions(vst3sdk PUBLIC "DEVELOPMENT")
endif()

function(plugin_add_vst3sdk NAME)
    target_link_libraries("${NAME}" PRIVATE vst3sdk)
    target_sources("${NAME}" PRIVATE
        "${VST3SDK_BASEDIR}/public.sdk/source/main/moduleinit.cpp"
        "${VST3SDK_BASEDIR}/public.sdk/source/main/pluginfactory.cpp")
    if(WIN32)
        target_sources("${NAME}" PRIVATE
            "${VST3SDK_BASEDIR}/public.sdk/source/main/dllmain.cpp")
    elseif(APPLE)
        target_sources("${NAME}" PRIVATE
            "${VST3SDK_BASEDIR}/public.sdk/source/main/macmain.cpp")
    else()
        target_sources("${NAME}" PRIVATE
            "${VST3SDK_BASEDIR}/public.sdk/source/main/linuxmain.cpp")
    endif()
endfunction()

# --- VST3SDK hosting ---
add_library(vst3sdk_hosting STATIC EXCLUDE_FROM_ALL
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/connectionproxy.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/eventlist.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/hostclasses.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/parameterchanges.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/pluginterfacesupport.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/plugprovider.cpp"
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/processdata.cpp")
if(FALSE)
    if(WIN32)
        target_sources(vst3sdk_hosting PRIVATE
            "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/module_win32.cpp")
    elseif(APPLE)
        target_sources(vst3sdk_hosting PRIVATE
            "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/module_mac.mm")
    else()
        target_sources(vst3sdk_hosting PRIVATE
            "${VST3SDK_BASEDIR}/public.sdk/source/vst/hosting/module_linux.cpp")
    endif()
endif()
target_link_libraries(vst3sdk_hosting PUBLIC vst3sdk)

# --- VSTGUI ---
add_library(vst3sdk_vstgui STATIC EXCLUDE_FROM_ALL
    "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstguieditor.cpp")
if(WIN32)
    target_sources(vst3sdk_vstgui PRIVATE
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstgui_win32_bundle_support.cpp")
    target_compile_definitions(vst3sdk_vstgui PRIVATE "SMTG_MODULE_IS_BUNDLE=1")
endif()
target_link_libraries(vst3sdk_vstgui PUBLIC vst3sdk sfizz::vstgui)

function(plugin_add_vstgui NAME)
    target_link_libraries("${NAME}" PRIVATE vst3sdk_vstgui)
endfunction()

# --- Warning suppressions ---
foreach(_target vst3sdk_vstgui vst3sdk)
    gw_target_warn("${_target}" PUBLIC
        "-Wno-extra"
        "-Wno-class-memaccess")
    gw_target_warn("${_target}" PRIVATE
        "-Wno-multichar"
        "-Wno-reorder"
        "-Wno-class-memaccess"
        "-Wno-ignored-qualifiers"
        "-Wno-unknown-pragmas"
        "-Wno-unused-function"
        "-Wno-unused-parameter"
        "-Wno-unused-variable")
endforeach()
