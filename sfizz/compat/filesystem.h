#pragma once

#ifdef __cplusplus
	#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)
		#include <filesystem>
	#elif (__cplusplus >= 201402L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
		#warning std::experimental::filesystem in use
		#include <experimental/filesystem>
		namespace std {
			namespace filesystem = std::experimental::filesystem;
		}
	#endif
#else
	#error no filesystem support
#endif
