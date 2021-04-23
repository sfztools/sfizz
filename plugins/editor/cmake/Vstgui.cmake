add_library(sfizz_vstgui STATIC EXCLUDE_FROM_ALL
    "${VSTGUI_BASEDIR}/vstgui/lib/animation/animations.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/animation/animator.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/animation/timingfunctions.cpp"
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
    "${VSTGUI_BASEDIR}/vstgui/lib/platform/platformfactory.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/platform/common/fileresourceinputstream.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/platform/common/genericoptionmenu.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/platform/common/generictextedit.cpp"
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
    "${VSTGUI_BASEDIR}/vstgui/lib/pixelbuffer.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/vstguidebug.cpp"
    "${VSTGUI_BASEDIR}/vstgui/lib/vstguiinit.cpp")

add_library(sfizz::vstgui ALIAS sfizz_vstgui)

if(WIN32)
    target_sources(sfizz_vstgui PRIVATE
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/direct2d/d2dbitmap.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/direct2d/d2ddrawcontext.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/direct2d/d2dfont.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/direct2d/d2dgraphicspath.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32datapackage.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32dragging.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32factory.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32frame.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32openglview.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32optionmenu.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32resourcestream.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32support.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/win32textedit.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/winfileselector.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/winstring.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/win32/wintimer.cpp")
elseif(APPLE)
    target_sources(sfizz_vstgui PRIVATE
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/carbon/hiviewframe.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/carbon/hiviewoptionmenu.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/carbon/hiviewtextedit.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/autoreleasepool.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/cocoahelpers.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/cocoaopenglview.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/cocoatextedit.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/nsviewdraggingsession.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/nsviewframe.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cocoa/nsviewoptionmenu.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/caviewlayer.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cfontmac.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cgbitmap.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/cgdrawcontext.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/macclipboard.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/macfactory.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/macfileselector.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/macglobals.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/macstring.mm"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/mactimer.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/mac/quartzgraphicspath.cpp")
else()
    target_sources(sfizz_vstgui PRIVATE
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/cairobitmap.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/cairocontext.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/cairofont.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/cairogradient.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/cairopath.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/linuxfactory.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/linuxstring.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11dragging.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11fileselector.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11frame.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11platform.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11timer.cpp"
        "${VSTGUI_BASEDIR}/vstgui/lib/platform/linux/x11utils.cpp")
endif()

target_include_directories(sfizz_vstgui PUBLIC "${VSTGUI_BASEDIR}")

if(WIN32)
    if (NOT MSVC)
        # autolinked on MSVC with pragmas
        target_link_libraries(sfizz_vstgui PRIVATE
            "opengl32"
            "d2d1"
            "dwrite"
            "dwmapi"
            "windowscodecs"
            "shlwapi")
    endif()
elseif(APPLE)
    target_link_libraries(sfizz_vstgui PRIVATE
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
    pkg_check_modules(PANGO REQUIRED pangocairo pangoft2)
    pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
    pkg_check_modules(GLIB REQUIRED glib-2.0)
    target_include_directories(sfizz_vstgui PRIVATE
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
        ${PANGO_INCLUDE_DIRS}
        ${FONTCONFIG_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS})
    target_link_libraries(sfizz_vstgui  PRIVATE
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
        ${PANGO_LIBRARIES}
        ${FONTCONFIG_LIBRARIES}
        ${GLIB_LIBRARIES})
    find_library(DL_LIBRARY "dl")
    if(DL_LIBRARY)
        target_link_libraries(sfizz_vstgui PRIVATE "${DL_LIBRARY}")
    endif()
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    target_compile_definitions(sfizz_vstgui PUBLIC "DEVELOPMENT")
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    target_compile_definitions(sfizz_vstgui PUBLIC "RELEASE")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows 10 RS2 DDI for custom fonts
    target_compile_definitions(sfizz_vstgui PRIVATE "NTDDI_VERSION=0x0A000003")
    # disable custom fonts if dwrite3 API is unavailable in MinGW
    if(MINGW)
        check_cxx_source_compiles("
#include <windows.h>
#include <dwrite_3.h>
HRESULT FeatureCheck(IDWriteFontSet* self, const WCHAR* name, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style, IDWriteFontSet** fontset)
{
    return self->GetMatchingFonts(name, weight, stretch, style, fontset);
}
int main()
{
    return 0;
}" SFIZZ_MINGW_SUPPORTS_DWRITE3)
        if(NOT SFIZZ_MINGW_SUPPORTS_DWRITE3)
            message(WARNING "This version of MinGW does not support DirectWrite 3. Custom font support is disabled.")
            target_compile_definitions(sfizz_vstgui PRIVATE "VSTGUI_WIN32_CUSTOMFONT_SUPPORT=0")
        endif()
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    gw_target_warn(sfizz_vstgui PRIVATE
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
