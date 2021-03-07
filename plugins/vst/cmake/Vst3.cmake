find_package(Threads REQUIRED)

# --- VST3SDK ---
function(plugin_add_vst3sdk NAME)
    target_sources("${NAME}" PRIVATE
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
        target_sources("${NAME}" PRIVATE
            "${VST3SDK_BASEDIR}/public.sdk/source/common/threadchecker_win32.cpp"
            "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstgui_win32_bundle_support.cpp"
            "${VST3SDK_BASEDIR}/public.sdk/source/main/dllmain.cpp")
        if(NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 14)
            set_property(TARGET "${NAME}" PROPERTY CXX_STANDARD 14)
        endif()
    elseif(APPLE)
        target_sources("${NAME}" PRIVATE
            "${VST3SDK_BASEDIR}/public.sdk/source/main/macmain.cpp")
    else()
        target_sources("${NAME}" PRIVATE
            "${VST3SDK_BASEDIR}/public.sdk/source/common/threadchecker_linux.cpp"
            "${VST3SDK_BASEDIR}/public.sdk/source/main/linuxmain.cpp")
    endif()
    target_include_directories("${NAME}" PRIVATE "${VST3SDK_BASEDIR}")
    target_link_libraries("${NAME}" PRIVATE Threads::Threads)
    if(MINGW)
        target_compile_definitions("${NAME}" PRIVATE
            "_NATIVE_WCHAR_T_DEFINED=1" "__wchar_t=wchar_t")
    endif()

    set(_vst_release_build_types MinSizeRel Release RelWithDebInfo)

    if(CMAKE_BUILD_TYPE IN_LIST _vst_release_build_types)
        target_compile_definitions("${NAME}" PRIVATE "RELEASE")
    else()
        target_compile_definitions("${NAME}" PRIVATE "DEVELOPMENT")
    endif()
endfunction()

# --- VSTGUI ---
function(plugin_add_vstgui NAME)
    target_link_libraries("${NAME}" PRIVATE sfizz::vstgui)
    target_sources("${NAME}" PRIVATE "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstguieditor.cpp")
    target_compile_definitions("${NAME}" PRIVATE "SMTG_MODULE_IS_BUNDLE=1")
endfunction()
