project "StreamlineCore"
    language "C++"
    cppdialect "C++20"
		
    targetdir 	("%{wks.location}/bin/%{prj.name}/" .. outputDir)
    objdir 		("%{wks.location}/obj/%{prj.name}/" .. outputDir)
	
	pchheader "pch.h"
	pchsource "src/pch.cpp"

    files 
    { 
        "src/**.h", 
        "src/**.cpp",
    }
	
	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

    includedirs
    {
        "%{IncludeDir.StreamlineCore}",
        "%{IncludeDir.glfw}",
        "%{IncludeDir.glad}",
        "%{IncludeDir.imgui}",
    }

	links
	{
        "glad",
        "glfw",
        "imgui",
	}
	
    filter "system:windows"
        kind "StaticLib"
        staticruntime "off"
        systemversion "latest"
        links
        {
            "opengl32.lib",
        }
		
	filter "system:linux"
        kind "SharedLib"
        staticruntime "off"
        pic "On"
        systemversion "latest"

    filter "configurations:Debug"
		runtime "Debug"
        symbols "on"
    filter "configurations:Release"
		runtime "Release"
        optimize "on"