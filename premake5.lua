-- premake5.lua
workspace "VulkanL"
   configurations { "Debug", "Release" }
   platforms { "x64", "arm64" }   
   startproject "VulkanL"
   includedirs { "." }
   defines { "_CRT_SECURE_NO_WARNINGS" }

filter "action:vs*"
   system "windows"
   architecture "x86_64"  
   vectorextensions "SSE4.1"
   toolset "msc-clangcl"
   includedirs { ".", "source", "$(VK_SDK_PATH)/Include", "external/glfw-3.3.2/include", "external/stb" } --, "extenral/threads" }
--   sysincludedirs { "external/threads" }
   libdirs { "$(VK_SDK_PATH)/Lib" }
   libdirs { "external/glfw-3.3.2/lib-vc2019" }
   links { "vulkan-1.lib", "glfw3" }
   defines { "WIN32", "RE_PLATFORM_WIN64", "_CRT_SECURE_NO_WARNINGS", "RGFX_USE_VULKAN" }

filter "action:xcode*"
   system "macosx"
   architecture "ARM64"
   vectorextensions "NEON"
   toolset "clang"
   defines { "MACOS" }    
   buildoptions { "-Xclang -flto-visibility-public-std -fblocks" }
   linkoptions { "-framework Cocoa -framework IOKit -framework CoreFoundation -framework IOSurface -framework Metal -framework QuartzCore" }
   sysincludedirs { "external/glfw-3.3/include", "../MoltenVK/Package/Release/MoltenVK/include", "external" }
   includedirs { ".", "source", "external/glfw-3.3.2/include", "external/stb" } --, "extenral/threads" }
   libdirs { "external/glfw-3.3/lib-macos", "../MoltenVK/Package/Release/MoltenVK/MoltenVK.xcframework/macos-arm64_x86_64" }
   links { "c++", "glfw3", "MoltenVK" }
   architecture "ARM64"  
   vectorextensions "NEON"
   defines { "ARM64", "RE_PLATFORM_MACOS" }

   xcodebuildsettings
   {
      ["GCC_C_LANGUAGE_STANDARD"] = "c11";         
      ["CLANG_CXX_LANGUAGE_STANDARD"] = "c++11";
      ["CLANG_CXX_LIBRARY"]  = "libc++";
      ["SDKROOT"] = "macosx";
      ["CLANG_ENABLE_OBJC_WEAK"] = "YES";
      ["CODE_SIGN_IDENTITY"] = "-";                      --iphoneos";  
      ["VALIDATE_WORKSPACE_SKIPPED_SDK_FRAMEWORKS"] = "OpenGL";
      ["CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING"] = "YES";
      ["CLANG_WARN_BOOL_CONVERSION"] = "YES";
      ["CLANG_WARN_COMMA"] = "YES";
      ["CLANG_WARN_CONSTANT_CONVERSION"] = "YES";
      ["CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS"] = "YES";
      ["CLANG_WARN_EMPTY_BODY"] = "YES";
      ["CLANG_WARN_ENUM_CONVERSION"] = "YES";
      ["CLANG_WARN_INFINITE_RECURSION"] = "YES";
      ["CLANG_WARN_INT_CONVERSION"] = "YES";
      ["CLANG_WARN_NON_LITERAL_NULL_CONVERSION"] = "YES";
      ["CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF"] = "YES";
      ["CLANG_WARN_OBJC_LITERAL_CONVERSION"] = "YES";
      ["CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER"] = "YES";
      ["CLANG_WARN_RANGE_LOOP_ANALYSIS"] = "YES";
      ["CLANG_WARN_STRICT_PROTOTYPES"] = "YES";
      ["CLANG_WARN_SUSPICIOUS_MOVE"] = "YES";
      ["CLANG_WARN_UNREACHABLE_CODE"] = "YES";
      ["CLANG_WARN__DUPLICATE_METHOD_MATCH"] = "YES";
      ["GCC_WARN_64_TO_32_BIT_CONVERSION"] = "YES";
      ["GCC_WARN_UNDECLARED_SELECTOR"] = "YES";
      ["GCC_WARN_UNINITIALIZED_AUTOS"] = "YES";
      ["GCC_WARN_UNUSED_FUNCTION"] = "YES";
      ["GCC_NO_COMMON_BLOCKS"] = "YES";
      ["ENABLE_STRICT_OBJC_MSGSEND"] = "YES";
   }   

filter { "action:xcode*", "configurations:Debug" }
   xcodebuildsettings
   {
      ["ENABLE_TESTABILITY"] = "YES";
   } 

project "VulkanL"
   location "source"
   kind "ConsoleApp"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"
   debugdir "."


   pchheader "stdafx.h"
   pchsource "source/stdafx.c"

   files 
   { 
      "source/**.h", 
      "source/**.c",
      "source/**.cpp",
      "shaders/**.vert",   
      "shaders/**.frag",     
   }

   excludes
   {
      "source/Framework/**",     
      "source/Vulkan/**",    
      "source/rgfx/mesh.*",
      "source/rgfx/resource.*",
      "source/rgfx/scene.*",
      "source/rgfx/texture.*",
      "source/rgfx/GL4.6/dds.*"
    }

   filter "configurations:Debug"
      defines { "DEBUG", "MEM_DEBUG" }
      symbols  "On"
      libdirs { "lib/Debug" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
      libdirs { "lib/Release" }

filter { "action:not xcode*", "files:**.vert or files:**.frag" }
   -- A message to display while this build step is running (optional)
   buildmessage 'Compiling %{file.relpath}'

   -- One or more commands to run (required)
   buildcommands {
   --   'glslangValidator -V -o "%{file.basename}%{file.extension}.spv" "%{file.relpath}"'
      'glslangValidator -V -o "%{file.relpath}.spv" "%{file.relpath}"'
   }

   -- One or more outputs resulting from the build (required)
   --buildoutputs { '%{file.basename}%{file.extension}.spv' }
   buildoutputs { '%{file.relpath}.spv' }
