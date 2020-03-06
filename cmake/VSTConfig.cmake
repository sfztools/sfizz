set (VSTPLUGIN_NAME            "sfizz")
set (VSTPLUGIN_VENDOR          "Paul Ferrand")
set (VSTPLUGIN_URL             "http://sfztools.github.io/sfizz")
set (VSTPLUGIN_EMAIL           "paul@ferrand.cc")

# The variable CMAKE_SYSTEM_PROCESSOR is incorrect on Visual studio...
# see https://gitlab.kitware.com/cmake/cmake/issues/15170

if(MSVC)
    set(VST3_SYSTEM_PROCESSOR "${MSVC_CXX_ARCHITECTURE_ID}")
else()
    set(VST3_SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")
endif()

message(STATUS "The system architecture is: ${VST3_SYSTEM_PROCESSOR}")

# --- VST3 Bundle architecture ---
if(NOT VST3_PACKAGE_ARCHITECTURE)
    if(APPLE)
        # VST3 packages are universal on Apple, architecture string not needed
    else()
        if(VST3_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
            set(VST3_PACKAGE_ARCHITECTURE "x86_64")
        elseif(VST3_SYSTEM_PROCESSOR MATCHES "^(i.86|x86)$")
            if(WIN32)
                set(VST3_PACKAGE_ARCHITECTURE "x86")
            else()
                set(VST3_PACKAGE_ARCHITECTURE "i386")
            endif()
        else()
            message(FATAL_ERROR "We don't know this architecture for VST3: ${VST3_SYSTEM_PROCESSOR}.")
        endif()
    endif()
endif()

message(STATUS "The VST3 architecture is deduced as: ${VST3_PACKAGE_ARCHITECTURE}")
