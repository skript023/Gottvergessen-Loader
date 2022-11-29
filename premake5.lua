	workspace "Gottvergesen Loader"
	architecture "x64"
	startproject "Gottvergesen Loader"

	configurations
	{
		-- "Debug", -- Debug isn't buildable and causes confusion for new people
		"Release"
	}

	outputdir = "%{cfg.buildcfg}"

	CppVersion = "C++20"
	MsvcToolset = "v143"
	WindowsSdkVersion = "10.0"
  
	function DeclareMSVCOptions()
		filter "system:windows"
		staticruntime "Off"
		floatingpoint "Fast"
		systemversion (WindowsSdkVersion)
		toolset (MsvcToolset)
		cppdialect (CppVersion)

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"NOMINMAX",
			"WIN32_LEAN_AND_MEAN"
--			"_WIN32_WINNT=0x601" -- Support Windows 7
		}

		disablewarnings
		{
			"4100", -- C4100: unreferenced formal parameter
			"4201", -- C4201: nameless struct/union
			"4307",  -- C4307: integral constant overflow
			"4996"  -- C4996: deprecated in C++17
		}
	end
  
	function file_exists(name)
		local f=io.open(name,"r")
		if f~=nil then io.close(f) return true else return false end
	end
   
	function DeclareDebugOptions()
		filter "configurations:Debug"
		    defines { "_DEBUG" }
		    symbols "On"
		filter "not configurations:Debug"
		    defines { "NDEBUG" }
	end
	
	project "g3log"
		location "vendor/%{prj.name}"
		kind "StaticLib"
		language "C++"

		targetdir ("bin/lib/" .. outputdir)
		objdir ("bin/lib/int/" .. outputdir .. "/%{prj.name}")
		
		includedirs
		{
		    "vendor/%{prj.name}/src"
		}

		g3log_file = "vendor/g3log/src/g3log/generated_definitions.hpp"
		if(file_exists(g3log_file) == false) then
			file = io.open(g3log_file, "w")
			if(file == nil) then
				premake.error("Failed to locate vendor directories. Try doing git pull --recurse-submodules.")
			end
			file:write("#pragma once");
		end
		
		files
		{
		    "vendor/%{prj.name}/src/**.hpp",
		    "vendor/%{prj.name}/src/**.cpp"
		}
		
		removefiles
		{
		    "vendor/%{prj.name}/src/crashhandler_unix.cpp"
		}

		DeclareMSVCOptions()
		DeclareDebugOptions()

    project "Gottvergessen-Loader"
		kind "ConsoleApp"
		language "C++"
		location "Gottvergessen-Loader"
		--symbols "Off"	
		characterset ("MBCS")

		targetdir ("bin/%{cfg.buildcfg}")
		objdir ("bin/obj/%{cfg.buildcfg}/%{prj.name}")

		PrecompiledHeaderInclude = "common.hpp"
		PrecompiledHeaderSource = "%{prj.name}/src/common.cpp"

		includedirs
		{
			"%{prj.name}/src",
			"vendor/g3log/src",
			"vendor/json/single_include"
		}

		files
		{
		    "%{prj.name}/src/**.hpp",
			"%{prj.name}/src/**.h",
			"%{prj.name}/src/**.cpp",
	        "%{prj.name}/src/**.rc",
	        "%{prj.name}/src/**.aps"
		}

		libdirs
		{
		    "bin/lib"
		}

		links
		{
		    "g3log"
		}

		filter "configurations:Debug"
		 defines { "DEBUG" }
		 symbols "On"

		filter "configurations:Release"
		 defines { "NDEBUG" }
		 optimize "On"

	    DeclareMSVCOptions()
	    DeclareDebugOptions()