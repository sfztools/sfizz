add_library(plugins-common STATIC EXCLUDE_FROM_ALL
    "common/plugin/MessageUtils.h"
    "common/plugin/MessageUtils.cpp"
    "common/plugin/InstrumentDescription.h"
    "common/plugin/InstrumentDescription.cpp")
target_include_directories(plugins-common PUBLIC "common")
target_link_libraries(plugins-common
    PUBLIC sfizz::spin_mutex
    PUBLIC sfizz::filesystem absl::strings
    PRIVATE sfizz::internal sfizz::sfizz)
add_library(sfizz::plugins-common ALIAS plugins-common)

if((SFIZZ_LV2 AND SFIZZ_LV2_UI) OR SFIZZ_VST)
    add_subdirectory(editor)
endif()

if(SFIZZ_LV2)
    add_subdirectory(lv2)
endif()

if(SFIZZ_VST)
    add_subdirectory(vst)
else()
    if(SFIZZ_AU)
        message(WARNING "Audio Unit requires VST to be enabled")
    endif()
endif()
