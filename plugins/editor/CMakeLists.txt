set(VSTGUI_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/external/vstgui4")
include("cmake/Vstgui.cmake")

set(EDITOR_RESOURCES
    logo.png
    logo_orange.png
    logo_text.png
    logo_text_white.png
    logo_text_shaded.png
    logo_text@2x.png
    logo_text_white@2x.png
    logo_text_shaded@2x.png
    background.png
    background@2x.png
    icon_white.png
    icon_white@2x.png
    knob48.png
    knob48@2x.png
    Fonts/sfizz-fluentui-system-r20.ttf
    Fonts/sfizz-fluentui-system-f20.ttf
    Fonts/sfizz-misc-icons.ttf
    Fonts/Roboto-Regular.ttf
    Themes/Default/theme.xml
    PARENT_SCOPE)

function(copy_editor_resources TARGET SOURCE_DIR DESTINATION_DIR)
    set(_deps)
    foreach(res ${EDITOR_RESOURCES})
        get_filename_component(_dir "${res}" DIRECTORY)
        file(MAKE_DIRECTORY "${DESTINATION_DIR}/${_dir}")
        add_custom_command(
            OUTPUT "${DESTINATION_DIR}/${res}"
            COMMAND "${CMAKE_COMMAND}" "-E" "copy"
                    "${SOURCE_DIR}/${res}" "${DESTINATION_DIR}/${res}"
            DEPENDS "${SOURCE_DIR}/${res}")
        list(APPEND _deps "${DESTINATION_DIR}/${res}")
    endforeach()
    add_custom_target("${TARGET}_editor_resources" DEPENDS ${_deps})
    add_dependencies("${TARGET}" "${TARGET}_editor_resources")
endfunction()

set(UI_FILES layout/main.fl layout/about.fl)
source_group("Editor UI" FILES ${UI_FILES})

# editor
add_library(sfizz_editor STATIC EXCLUDE_FROM_ALL
    src/editor/EditIds.h
    src/editor/EditIds.cpp
    src/editor/Editor.h
    src/editor/Editor.cpp
    src/editor/EditorLibs.h
    src/editor/EditorLibs.cpp
    src/editor/EditorController.h
    src/editor/GUIComponents.h
    src/editor/GUIComponents.cpp
    src/editor/GUIHelpers.h
    src/editor/GUIHelpers.cpp
    src/editor/GUIPiano.h
    src/editor/GUIPiano.cpp
    src/editor/DlgAbout.h
    src/editor/DlgAbout.cpp
    src/editor/Theme.h
    src/editor/Theme.cpp
    src/editor/ColorHelpers.h
    src/editor/ColorHelpers.cpp
    src/editor/ImageHelpers.h
    src/editor/ImageHelpers.cpp
    src/editor/NativeHelpers.h
    src/editor/NativeHelpers.cpp
    src/editor/VSTGUIHelpers.h
    src/editor/VSTGUIHelpers.cpp
    src/editor/layout/main.hpp
    src/editor/layout/about.hpp
    src/editor/utility/vstgui_after.h
    src/editor/utility/vstgui_before.h
    src/editor/GitBuildId.h
    ${UI_FILES}
    "${CMAKE_CURRENT_BINARY_DIR}/git-build-id/GitBuildId.c")
add_library(sfizz::editor ALIAS sfizz_editor)
target_include_directories(sfizz_editor PUBLIC "src")
target_link_libraries(sfizz_editor PUBLIC sfizz::messaging sfizz::plugins-common)
target_link_libraries(sfizz_editor PRIVATE sfizz::vstgui)
target_compile_definitions(sfizz_editor PRIVATE
    "SFIZZ_VERSION=\"${CMAKE_PROJECT_VERSION}\"")
if(APPLE)
    find_library(APPLE_APPKIT_LIBRARY "AppKit")
    find_library(APPLE_CORESERVICES_LIBRARY "CoreServices")
    find_library(APPLE_FOUNDATION_LIBRARY "Foundation")
    target_sources(sfizz_editor PRIVATE
        src/editor/NativeHelpers.mm
        src/editor/VSTGUIHelpers.mm)
    target_link_libraries(sfizz_editor PRIVATE
        "${APPLE_APPKIT_LIBRARY}"
        "${APPLE_CORESERVICES_LIBRARY}"
        "${APPLE_FOUNDATION_LIBRARY}")
    target_compile_options(sfizz_editor PRIVATE "-fobjc-arc")
endif()

# dependencies
add_library(sfizz_colorspaces INTERFACE)
add_library(sfizz::colorspaces ALIAS sfizz_colorspaces)
target_include_directories(sfizz_colorspaces INTERFACE "external/color-spaces")

add_library(sfizz_stb_image INTERFACE)
add_library(sfizz::stb_image ALIAS sfizz_stb_image)
target_include_directories(sfizz_stb_image INTERFACE "external/stb_image")

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
target_link_libraries(sfizz_editor PRIVATE sfizz::colorspaces sfizz::stb_image
    sfizz::bit_array sfizz::filesystem sfizz::pugixml)

# layout tool
if(NOT CMAKE_CROSSCOMPILING)
    add_executable(layout-maker
        "tools/layout-maker/sources/layout.h"
        "tools/layout-maker/sources/reader.cpp"
        "tools/layout-maker/sources/reader.h"
        "tools/layout-maker/sources/main.cpp")
    target_link_libraries(layout-maker PRIVATE absl::strings)

    foreach(_layout main about)
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/layout/${_layout}.hpp"
            COMMAND "$<TARGET_FILE:layout-maker>"
            "${CMAKE_CURRENT_SOURCE_DIR}/layout/${_layout}.fl"
            > "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/layout/${_layout}.hpp"
            DEPENDS layout-maker "${CMAKE_CURRENT_SOURCE_DIR}/layout/${_layout}.fl")
    endforeach()
endif()

# git build identifier
add_custom_target(sfizz_editor_git_build_id
    COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
            "-DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/git-build-id/GitBuildId.c"
            "-P" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GitBuildID.cmake"
    BYPRODUCTS "git-build-id/GitBuildId.c")
add_dependencies(sfizz_editor sfizz_editor_git_build_id)
