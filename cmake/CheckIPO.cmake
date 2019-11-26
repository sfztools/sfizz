# Added in CMake 3.9
include (CheckIPOSupported)
check_ipo_supported (RESULT result OUTPUT output)

if (result AND ENABLE_LTO AND ${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message (STATUS "\nLTO enabled.")
else()
    if (${output})
        message (WARNING "\nIPO disabled: ${output}")
    else()
        message (WARNING "\nIPO was disabled, not in a Release build?")
    endif()
    set (ENABLE_LTO OFF)
endif()
