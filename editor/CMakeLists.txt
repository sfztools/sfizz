set(VSTGUI_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/external/vstgui4")
include("cmake/Vstgui.cmake")

set(EDITOR_RESOURCES
    logo.png
    logo_text.png
    logo_text_white.png
    logo_text@2x.png
    logo_text_white@2x.png
    background.png
    background@2x.png
    icon_white.png
    icon_white@2x.png
    knob48.png
    knob48@2x.png
    Fonts/sfizz-fluentui-system-r20.ttf
    Fonts/Roboto-Regular.ttf
    PARENT_SCOPE)

function(copy_editor_resources SOURCE_DIR DESTINATION_DIR)
    foreach(res ${EDITOR_RESOURCES})
        get_filename_component(_dir "${res}" DIRECTORY)
        file(MAKE_DIRECTORY "${DESTINATION_DIR}/${_dir}")
        file(COPY "${SOURCE_DIR}/${res}" DESTINATION "${DESTINATION_DIR}/${_dir}")
    endforeach()
endfunction()

# editor
add_library(sfizz_editor STATIC EXCLUDE_FROM_ALL
    src/editor/EditIds.h
    src/editor/EditIds.cpp
    src/editor/Editor.h
    src/editor/Editor.cpp
    src/editor/EditorController.h
    src/editor/GUIComponents.h
    src/editor/GUIComponents.cpp
    src/editor/GUIPiano.h
    src/editor/GUIPiano.cpp
    src/editor/NativeHelpers.h
    src/editor/NativeHelpers.cpp
    src/editor/layout/main.hpp
    src/editor/utility/vstgui_after.h
    src/editor/utility/vstgui_before.h)
target_include_directories(sfizz_editor PUBLIC "src")
target_link_libraries(sfizz_editor PUBLIC sfizz::messaging)
target_link_libraries(sfizz_editor PRIVATE sfizz::vstgui)
target_link_libraries(sfizz_editor PUBLIC absl::strings)
if(APPLE)
    find_library(APPLE_APPKIT_LIBRARY "AppKit")
    find_library(APPLE_CORESERVICES_LIBRARY "CoreServices")
    find_library(APPLE_FOUNDATION_LIBRARY "Foundation")
    target_sources(sfizz_editor PRIVATE
        src/editor/NativeHelpers.mm)
    target_link_libraries(sfizz_editor PRIVATE
        "${APPLE_APPKIT_LIBRARY}"
        "${APPLE_CORESERVICES_LIBRARY}"
        "${APPLE_FOUNDATION_LIBRARY}")
    target_compile_options(sfizz_editor PRIVATE "-fobjc-arc")
endif()

# dependencies
if(WIN32)
    #
elseif(APPLE)
    #
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(sfizz-gio "gio-2.0" REQUIRED)
    target_include_directories(sfizz_editor PRIVATE ${sfizz-gio_INCLUDE_DIRS})
    target_link_libraries(sfizz_editor PRIVATE ${sfizz-gio_LIBRARIES})
endif()
target_link_libraries(sfizz_editor PRIVATE sfizz::filesystem)

# layout tool
if(NOT CMAKE_CROSSCOMPILING)
    add_executable(layout-maker
        "tools/layout-maker/sources/layout.h"
        "tools/layout-maker/sources/reader.cpp"
        "tools/layout-maker/sources/reader.h"
        "tools/layout-maker/sources/main.cpp")
    target_link_libraries(layout-maker PRIVATE absl::strings)

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/layout/main.hpp"
        COMMAND "$<TARGET_FILE:layout-maker>"
        "${CMAKE_CURRENT_SOURCE_DIR}/layout/main.fl"
        > "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/layout/main.hpp"
        DEPENDS layout-maker "${CMAKE_CURRENT_SOURCE_DIR}/layout/main.fl")
endif()
