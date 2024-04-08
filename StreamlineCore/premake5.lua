project "StreamlineCore"
    language "C++"
    cppdialect "C++latest"
    staticruntime "off"
		
    targetdir 	("%{wks.location}/bin/%{prj.name}/" .. outputDir)
    objdir 		("%{wks.location}/obj/%{prj.name}/" .. outputDir)

    files 
    { 
        "src/**.h", 
        "src/**.cpp",
        "dependencies/ImGuizmo/ImGuizmo.h",
        "dependencies/ImGuizmo/ImGuizmo.cpp",
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
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.magic_enum}",
    }

	links
	{
        "glad",
        "glfw",
        "imgui",
	}
	
    filter "system:windows"
        kind "StaticLib"
        systemversion "latest"
        links
        {
            "opengl32.lib",
        }
		
	filter "system:linux"
        kind "SharedLib"
        pic "On"
        systemversion "latest"

    filter "configurations:Debug"
		runtime "Debug"
        symbols "on"
    filter "configurations:Release"
		runtime "Release"
        optimize "on"