-- premake5.lua
workspace "VulkanL"
   configurations { "Debug", "Release" }
   startproject "VulkanL"

includedirs { "." }

configuration "windows"
   system "windows"
   architecture "x86_64"  
   vectorextensions "SSE4.1"
   toolset "msc-clangcl"
   includedirs { ".", "source", "$(VK_SDK_PATH)/Include", "external/glfw-3.3.2/include", "external/stb", "extenral/threads" }
   sysincludedirs { "external/threads" }
   libdirs { "$(VK_SDK_PATH)/Lib" }
   libdirs { "external/glfw-3.3.2/lib-vc2019" }
   links { "vulkan-1.lib", "glfw3" }
   defines { "WIN32", "RE_PLATFORM_WIN64", "_CRT_SECURE_NO_WARNINGS", "RGFX_USE_VULKAN" }


configuration "vs*"
--defines { "_CRT_SECURE_NO_WARNINGS" }
 
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
    }

   filter "configurations:Debug"
      defines { "DEBUG", "MEM_DEBUG" }
      symbols  "On"
      libdirs { "lib/Debug" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
      libdirs { "lib/Release" }

filter 'files:**.vert or files:**.frag'
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
