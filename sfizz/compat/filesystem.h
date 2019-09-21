#pragma once
#ifdef USE_STD_FILESYSTEM
	#warning FS
	#include <filesystem>
	namespace fs = std::filesystem;
#else
	#warning EXPFS
	#include <experimental/filesystem>
	namespace fs = std::experimental::filesystem;
#endif

// #if __cplusplus < 201703L
// 	#warning EXPFSNS
// #else
// 	#warning FSNS
// #endif