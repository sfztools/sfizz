set(VSTGUI_BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}/external/vstgui4")
include("cmake/Vstgui.cmake")

set(EDITOR_RESOURCES
    logo.png
    PARENT_SCOPE)

# editor
add_library(sfizz_editor STATIC EXCLUDE_FROM_ALL
    src/editor/EditIds.h
    src/editor/EditIds.cpp
    src/editor/Editor.h
    src/editor/Editor.cpp
    src/editor/EditorController.h
    src/editor/GUIComponents.h
    src/editor/GUIComponents.cpp
    src/editor/utility/vstgui_after.h
    src/editor/utility/vstgui_before.h)
target_include_directories(sfizz_editor PUBLIC "src")
target_link_libraries(sfizz_editor PRIVATE sfizz-vstgui)
target_link_libraries(sfizz_editor PUBLIC absl::strings)
