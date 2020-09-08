add_library(sfizz-vstgui STATIC EXCLUDE_FROM_ALL
    "${VSTGUI_BASEDIR}/vstgui/lib/animation/animations.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/animation/animator.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/animation/timingfunctions.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cbitmap.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cbitmapfilter.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/ccolor.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cdatabrowser.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cdrawcontext.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cdrawmethods.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cdropsource.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cfileselector.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cfont.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cframe.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cgradientview.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cgraphicspath.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/clayeredviewcontainer.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/clinestyle.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/coffscreencontext.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cautoanimation.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cbuttons.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/ccolorchooser.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/ccontrol.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cfontchooser.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cknob.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/clistcontrol.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cmoviebitmap.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cmoviebutton.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/coptionmenu.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cparamdisplay.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cscrollbar.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/csearchtextedit.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/csegmentbutton.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cslider.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cspecialdigit.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/csplashscreen.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cstringlist.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cswitch.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/ctextedit.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/ctextlabel.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cvumeter.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/controls/cxypad.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/copenglview.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cpoint.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/crect.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/crowcolumnview.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cscrollview.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cshadowviewcontainer.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/csplitview.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cstring.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/ctabview.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/ctooltipsupport.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cview.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cviewcontainer.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/cvstguitimer.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/genericstringlistdatabrowsersource.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/platform/common/genericoptionmenu.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/vstguidebug.cpp")

if(WIN32)
    target_sources(sfizz-vstgui PRIVATE
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/common/fileresourceinputstream.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/direct2d/d2dbitmap.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/direct2d/d2ddrawcontext.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/direct2d/d2dfont.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/direct2d/d2dgraphicspath.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32datapackage.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32dragging.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32frame.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32openglview.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32optionmenu.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32support.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32textedit.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/winfileselector.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/winstring.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/wintimer.cpp")
elseif(APPLE)
    target_sources(sfizz-vstgui PRIVATE
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/common/fileresourceinputstream.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/common/genericoptionmenu.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/common/generictextedit.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/carbon/hiviewframe.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/carbon/hiviewoptionmenu.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/carbon/hiviewtextedit.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/caviewlayer.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cfontmac.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cgbitmap.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cgdrawcontext.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/autoreleasepool.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/cocoahelpers.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/cocoaopenglview.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/cocoatextedit.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/nsviewdraggingsession.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/nsviewframe.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/nsviewoptionmenu.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/macclipboard.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/macfileselector.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/macglobals.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/macstring.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/mactimer.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/quartzgraphicspath.cpp")
else()
    target_sources(sfizz-vstgui PRIVATE
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/common/fileresourceinputstream.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/common/generictextedit.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/cairobitmap.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/cairocontext.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/cairofont.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/cairogradient.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/cairopath.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/linuxstring.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11fileselector.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11frame.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11platform.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11timer.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11utils.cpp")
endif()

target_include_directories(sfizz-vstgui PUBLIC "${VSTGUI_BASEDIR}")

if(WIN32)
    if (NOT MSVC)
        # autolinked on MSVC with pragmas
        find_library(OPENGL32_LIBRARY "opengl32")
        find_library(D2D1_LIBRARY "d2d1")
        find_library(DWRITE_LIBRARY "dwrite")
        find_library(DWMAPI_LIBRARY "dwmapi")
        find_library(WINDOWSCODECS_LIBRARY "windowscodecs")
        find_library(SHLWAPI_LIBRARY "shlwapi")
        target_link_libraries(sfizz-vstgui PRIVATE
            "${OPENGL32_LIBRARY}"
            "${D2D1_LIBRARY}"
            "${DWRITE_LIBRARY}"
            "${DWMAPI_LIBRARY}"
            "${WINDOWSCODECS_LIBRARY}"
            "${SHLWAPI_LIBRARY}")
    endif()
elseif(APPLE)
    target_link_libraries(sfizz-vstgui PRIVATE
        "${APPLE_COREFOUNDATION_LIBRARY}"
        "${APPLE_FOUNDATION_LIBRARY}"
        "${APPLE_COCOA_LIBRARY}"
        "${APPLE_OPENGL_LIBRARY}"
        "${APPLE_ACCELERATE_LIBRARY}"
        "${APPLE_QUARTZCORE_LIBRARY}"
        "${APPLE_CARBON_LIBRARY}"
        "${APPLE_AUDIOTOOLBOX_LIBRARY}"
        "${APPLE_COREAUDIO_LIBRARY}"
        "${APPLE_COREMIDI_LIBRARY}")
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
    target_include_directories(sfizz-vstgui PRIVATE
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
    target_link_libraries(sfizz-vstgui  PRIVATE
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
        target_link_libraries(sfizz-vstgui PRIVATE "${DL_LIBRARY}")
    endif()
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    target_compile_definitions(sfizz-vstgui PRIVATE "DEVELOPMENT")
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    target_compile_definitions(sfizz-vstgui PRIVATE "RELEASE")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # higher C++ requirement on Windows
    set_property(TARGET sfizz-vstgui PROPERTY CXX_STANDARD 14)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(sfizz-vstgui PRIVATE
        "-Wno-deprecated-copy"
        "-Wno-deprecated-declarations"
        "-Wno-extra"
        "-Wno-ignored-qualifiers"
        "-Wno-multichar"
        "-Wno-reorder"
        "-Wno-sign-compare"
        "-Wno-unknown-pragmas"
        "-Wno-unused-function"
        "-Wno-unused-parameter"
        "-Wno-unused-variable")
endif()
