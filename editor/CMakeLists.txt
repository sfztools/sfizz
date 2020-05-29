# resource files
set(EDITOR_RESOURCES
    logo.png
    PARENT_SCOPE)

# dependency checks
if(NOT TARGET sfizz-cairo)
    message(FATAL_ERROR "Could not find cairo, required by editor")
endif()

if(NOT TARGET sfizz-fontconfig)
    message(FATAL_ERROR "Could not find fontconfig, required by editor")
endif()

# pugl library
add_library(sfizz-pugl STATIC EXCLUDE_FROM_ALL
    external/pugl/pugl/detail/implementation.c)
target_include_directories(sfizz-pugl PUBLIC
    external/pugl)
if(WIN32)
    target_sources(sfizz-pugl PRIVATE
        external/pugl/pugl/detail/win.c
        external/pugl/pugl/detail/win_cairo.c)
elseif(APPLE)
    target_sources(sfizz-pugl PRIVATE
        external/pugl/pugl/detail/mac.m
        external/pugl/pugl/detail/mac_cairo.m
        external/pugl/pugl/detail/mac_stub.m)
else()
    target_sources(sfizz-pugl PRIVATE
        external/pugl/pugl/detail/x11.c
        external/pugl/pugl/detail/x11_cairo.c)
endif()
target_link_libraries(sfizz-pugl PUBLIC sfizz-cairo)

if(WIN32)
elseif(APPLE)
    find_library(APPLE_COCOA_LIBRARY "Cocoa")
    target_link_libraries(sfizz-pugl PRIVATE "${APPLE_COCOA_LIBRARY}")
else()
    find_package(X11 REQUIRED)
    target_include_directories(sfizz-pugl PRIVATE "${X11_INCLUDE_DIR}")
    target_link_libraries(sfizz-pugl PRIVATE ${X11_LIBRARIES})
endif()

# whereami library
add_library(sfizz-whereami STATIC EXCLUDE_FROM_ALL external/whereami/src/whereami.c)
target_include_directories(sfizz-whereami PUBLIC external/whereami/src)

# elements library
add_library(sfizz-elements STATIC EXCLUDE_FROM_ALL
    external/elements/lib/src/view.cpp
    external/elements/lib/src/element/button.cpp
    external/elements/lib/src/element/composite.cpp
    external/elements/lib/src/element/dial.cpp
    external/elements/lib/src/element/element.cpp
    external/elements/lib/src/element/floating.cpp
    external/elements/lib/src/element/flow.cpp
    external/elements/lib/src/element/gallery/button.cpp
    external/elements/lib/src/element/gallery/check_box.cpp
    external/elements/lib/src/element/gallery/icon_button.cpp
    external/elements/lib/src/element/gallery/menu.cpp
    external/elements/lib/src/element/gallery/radio_button.cpp
    external/elements/lib/src/element/grid.cpp
    external/elements/lib/src/element/image.cpp
    external/elements/lib/src/element/layer.cpp
    external/elements/lib/src/element/menu.cpp
    external/elements/lib/src/element/misc.cpp
    external/elements/lib/src/element/popup.cpp
    external/elements/lib/src/element/port.cpp
    external/elements/lib/src/element/progress_bar.cpp
    external/elements/lib/src/element/proxy.cpp
    external/elements/lib/src/element/slider.cpp
    external/elements/lib/src/element/text.cpp
    external/elements/lib/src/element/tile.cpp
    external/elements/lib/src/support/canvas.cpp
    external/elements/lib/src/support/draw_utils.cpp
    external/elements/lib/src/support/font.cpp
    external/elements/lib/src/support/glyphs.cpp
    external/elements/lib/src/support/io_context.cpp
    external/elements/lib/src/support/pixmap.cpp
    external/elements/lib/src/support/rect.cpp
    external/elements/lib/src/support/resource_paths.cpp
    external/elements/lib/src/support/text_utils.cpp
    external/elements/lib/src/support/theme.cpp)
target_include_directories(sfizz-elements PUBLIC
    external/elements/lib/include
    external/elements/lib/infra/include
    ../src/external # use our ghc::filesystem
    )
target_link_libraries(sfizz-elements PUBLIC sfizz-cairo sfizz-fontconfig)
target_compile_definitions(sfizz-elements PUBLIC
    "ELEMENTS_HOST_UI_LIBRARY_CUSTOM=1")

# editor
add_library(sfizz_editor STATIC EXCLUDE_FROM_ALL
    src/editor/Editor.cpp
    src/editor/Res.cpp
    src/editor/Demo.cpp
    src/host/ElementsPuglWindow.cpp
    src/host/ElementsPuglBaseView.cpp
    src/host/PuglHelpers.cpp)
if(WIN32)
    target_sources(sfizz_editor PRIVATE
        src/host/NativeWindowHelpers_win.cpp)
elseif(APPLE)
    target_sources(sfizz_editor PRIVATE
        src/host/NativeWindowHelpers_mac.mm
        src/host/MacBundleHelpers.mm)
else()
    target_sources(sfizz_editor PRIVATE
        src/host/NativeWindowHelpers_x11.cpp)
endif()
target_include_directories(sfizz_editor PUBLIC src)
target_link_libraries(sfizz_editor PUBLIC sfizz-pugl sfizz-elements sfizz-whereami)
# need the cyclic dependency editor <-> elements
target_link_libraries(sfizz-elements PUBLIC sfizz_editor)

# require the C++ standard 14
set_target_properties(sfizz-elements PROPERTIES CXX_STANDARD 14)
set_target_properties(sfizz_editor PROPERTIES CXX_STANDARD 14)
