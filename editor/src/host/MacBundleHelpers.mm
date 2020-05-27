/*=============================================================================
   Copyright (c) 2016-2020 Joel de Guzman

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/

#include <elements/support/resource_paths.hpp>
#include <elements/support/font.hpp>
#import <Cocoa/Cocoa.h>
#include <dlfcn.h>
#include <infra/filesystem.hpp>

namespace fs = cycfi::fs;

///////////////////////////////////////////////////////////////////////////////
// Helper utils
namespace
{
   CFBundleRef get_bundle_from_executable(const char* filepath)
   {
      NSString* exec_str = [NSString stringWithCString:filepath encoding : NSUTF8StringEncoding];
      NSString* mac_os_str = [exec_str stringByDeletingLastPathComponent];
      NSString* contents_str = [mac_os_str stringByDeletingLastPathComponent];
      NSString* bundleStr = [contents_str stringByDeletingLastPathComponent];
      return CFBundleCreate(0, (CFURLRef)[NSURL fileURLWithPath:bundleStr isDirectory : YES]);
   }

   CFBundleRef get_current_bundle()
   {
      Dl_info info;
      if (dladdr((const void*)get_current_bundle, &info) && info.dli_fname)
         return get_bundle_from_executable(info.dli_fname);
      return 0;
   }

   void get_resource_path(char resource_path[])
   {
      CFBundleRef main_bundle = get_current_bundle();
      CFURLRef resources_url = CFBundleCopyResourcesDirectoryURL(main_bundle);
      CFURLGetFileSystemRepresentation(resources_url, TRUE, (UInt8*) resource_path, PATH_MAX);
      CFRelease(resources_url);
   }
}

namespace cycfi { namespace elements
{
   // This is declared in font.hpp
   fs::path get_user_fonts_directory()
   {
      char resource_path[PATH_MAX];
      get_resource_path(resource_path);
      return fs::path(resource_path);
   }
}}
