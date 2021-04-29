find_path(PUREDATA_INCLUDE_DIR "m_pd.h" PATH_SUFFIXES "pd")

if(NOT PUREDATA_INCLUDE_DIR)
    message(FATAL_ERROR "Cannot find Puredata headers")
endif()

message(STATUS "Puredata headers: ${PUREDATA_INCLUDE_DIR}")

if(WIN32)
    set(PUREDATA_SUFFIX ".dll")
elseif(APPLE)
    set(PUREDATA_SUFFIX ".pd_darwin")
else()
    set(PUREDATA_SUFFIX ".pd_linux")
endif()
