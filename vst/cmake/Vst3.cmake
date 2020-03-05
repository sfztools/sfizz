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
        # "${VST3SDK_BASEDIR}/base/source/timer.cpp"
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
endfunction()

# --- VSTGUI ---
function(plugin_add_vstgui NAME)
    target_sources("${NAME}" PRIVATE
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/animation/animations.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/animation/animator.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/animation/timingfunctions.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cbitmap.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cbitmapfilter.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/ccolor.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cdatabrowser.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cdrawcontext.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cdrawmethods.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cdropsource.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cfileselector.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cfont.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cframe.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cgradientview.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cgraphicspath.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/clayeredviewcontainer.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/clinestyle.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/coffscreencontext.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cautoanimation.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cbuttons.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/ccolorchooser.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/ccontrol.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cfontchooser.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cknob.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/clistcontrol.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cmoviebitmap.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cmoviebutton.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/coptionmenu.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cparamdisplay.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cscrollbar.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/csearchtextedit.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/csegmentbutton.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cslider.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cspecialdigit.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/csplashscreen.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cstringlist.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cswitch.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/ctextedit.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/ctextlabel.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cvumeter.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/controls/cxypad.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/copenglview.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cpoint.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/crect.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/crowcolumnview.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cscrollview.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cshadowviewcontainer.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/csplitview.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cstring.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/ctabview.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/ctooltipsupport.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cview.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cviewcontainer.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/cvstguitimer.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/genericstringlistdatabrowsersource.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/common/genericoptionmenu.cpp"
        "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/vstguidebug.cpp")

    if(WIN32)
        target_sources("${NAME}" PRIVATE
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/common/fileresourceinputstream.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/direct2d/d2dbitmap.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/direct2d/d2ddrawcontext.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/direct2d/d2dfont.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/direct2d/d2dgraphicspath.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/win32datapackage.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/win32dragging.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/win32frame.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/win32openglview.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/win32optionmenu.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/win32support.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/win32textedit.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/winfileselector.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/winstring.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/win32/wintimer.cpp")
    elseif(APPLE)
        target_sources("${NAME}" PRIVATE
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/common/fileresourceinputstream.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/common/genericoptionmenu.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/common/generictextedit.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/carbon/hiviewframe.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/carbon/hiviewoptionmenu.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/carbon/hiviewtextedit.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/caviewlayer.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/cfontmac.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/cgbitmap.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/cgdrawcontext.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/cocoa/autoreleasepool.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/cocoa/cocoahelpers.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/cocoa/cocoaopenglview.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/cocoa/cocoatextedit.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/cocoa/nsviewdraggingsession.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/cocoa/nsviewframe.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/cocoa/nsviewoptionmenu.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/macclipboard.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/macfileselector.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/macglobals.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/macstring.mm"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/mactimer.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/mac/quartzgraphicspath.cpp")
    else()
        target_sources("${NAME}" PRIVATE
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/common/fileresourceinputstream.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/common/generictextedit.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/cairobitmap.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/cairocontext.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/cairofont.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/cairogradient.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/cairopath.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/linuxstring.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/x11fileselector.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/x11frame.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/x11platform.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/x11timer.cpp"
            "${VST3SDK_BASEDIR}/vstgui4/vstgui/lib/platform/linux/x11utils.cpp")
    endif()

    target_include_directories("${NAME}" PRIVATE "${VST3SDK_BASEDIR}/vstgui4")

    if(WIN32)
        #
    elseif(APPLE)
        #
    else()
        find_package(X11 REQUIRED)
        find_package(Freetype REQUIRED)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(LIBXCB REQUIRED xcb)
        pkg_check_modules(LIBXCB_UTIL REQUIRED xcb-util)
        pkg_check_modules(LIBXCB_CURSOR REQUIRED xcb-cursor)
        pkg_check_modules(LIBXCB_KEYSYMS REQUIRED xcb-keysyms)
        pkg_check_modules(LIBXCB_XKB REQUIRED xcb-xkb)
        pkg_check_modules(LIBXKB_COMMON REQUIRED xkbcommon)
        pkg_check_modules(LIBXKB_COMMON_X11 REQUIRED xkbcommon-x11)
        pkg_check_modules(CAIRO REQUIRED cairo)
        pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
        target_include_directories("${NAME}" PRIVATE
            ${X11_INCLUDE_DIRS}
            ${FREETYPE_INCLUDE_DIRS}
            ${LIBXCB_INCLUDE_DIRS}
            ${LIBXCB_UTIL_INCLUDE_DIRS}
            ${LIBXCB_CURSOR_INCLUDE_DIRS}
            ${LIBXCB_KEYSYMS_INCLUDE_DIRS}
            ${LIBXCB_XKB_INCLUDE_DIRS}
            ${LIBXKB_COMMON_INCLUDE_DIRS}
            ${LIBXKB_COMMON_X11_INCLUDE_DIRS}
            ${CAIRO_INCLUDE_DIRS}
            ${FONTCONFIG_INCLUDE_DIRS})
        target_link_libraries("${NAME}"  PRIVATE
            ${X11_LIBRARIES}
            ${FREETYPE_LIBRARIES}
            ${LIBXCB_LIBRARIES}
            ${LIBXCB_UTIL_LIBRARIES}
            ${LIBXCB_CURSOR_LIBRARIES}
            ${LIBXCB_KEYSYMS_LIBRARIES}
            ${LIBXCB_XKB_LIBRARIES}
            ${LIBXKB_COMMON_LIBRARIES}
            ${LIBXKB_COMMON_X11_LIBRARIES}
            ${CAIRO_LIBRARIES}
            ${FONTCONFIG_LIBRARIES})
        find_library(DL_LIBRARY "dl")
        if(DL_LIBRARY)
            target_link_libraries("${NAME}" PRIVATE "${DL_LIBRARY}")
        endif()
    endif()

    target_sources("${NAME}" PRIVATE
        "${VST3SDK_BASEDIR}/public.sdk/source/vst/vstguieditor.cpp")

    target_include_directories("${NAME}" PRIVATE
        external/steinberg/src)
endfunction()

# --- VST3 Bundle architecture ---
if(NOT VST3_PACKAGE_ARCHITECTURE)
    if(APPLE)
        # VST3 packages are universal on Apple, architecture string not needed
    else()
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
            set(VST3_PACKAGE_ARCHITECTURE "x86_64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
            if(WIN32)
                set(VST3_PACKAGE_ARCHITECTURE "x86")
            else()
                set(VST3_PACKAGE_ARCHITECTURE "i386")
            endif()
        else()
            message(FATAL_ERROR "We don't know this architecture for VST3: ${CMAKE_SYSTEM_PROCESSOR}.")
        endif()
    endif()
endif()
