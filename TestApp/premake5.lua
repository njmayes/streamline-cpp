project "TestApp"
    language "C++"
    cppdialect "C++latest"
	kind "ConsoleApp"
	
    targetdir 	("%{wks.location}/bin/%{prj.name}/" .. outputDir)
    objdir 		("%{wks.location}/obj/%{prj.name}/" .. outputDir)

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
        "%{IncludeDir.glm}",
        "%{IncludeDir.imgui}",
        "%{IncludeDir.json}",
        "%{IncludeDir.magic_enum}",
    }

	links
	{
		"StreamlineCore",
	}
	
    filter "system:windows"
        staticruntime "off"
        systemversion "latest"
		buildoptions 
		{
			"/Zc:preprocessor"
		}
		
	filter "system:linux"
        staticruntime "off"
        pic "On"
        systemversion "latest"

    filter "configurations:Debug"
		runtime "Debug"
        symbols "on"
	    defines 
        {
            "SLC_DEBUG"
        }
    filter "configurations:Release"
		runtime "Release"
        optimize "on"
	    defines 
        {
            "SLC_RELEASE"
        }