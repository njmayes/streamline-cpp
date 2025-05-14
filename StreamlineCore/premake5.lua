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
        "dependencies/stb_image/**.h",
        "dependencies/stb_image/**.cpp",
        "dependencies/ImGuizmo/ImGuizmo.h",
        "dependencies/ImGuizmo/ImGuizmo.cpp",
        "dependencies/glm/glm/**.hpp",
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
        "%{IncludeDir.glm}",
        "%{IncludeDir.imgui}",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.json}",
        "%{IncludeDir.magic_enum}",
        "%{IncludeDir.pfd}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.VulkanSDK}",
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
		buildoptions 
		{
			"/Zc:preprocessor"
		}
		
	filter "system:linux"
        kind "SharedLib"
        pic "On"
        systemversion "latest"

    filter "configurations:Debug"
		runtime "Debug"
        symbols "on"
	    defines 
        {
            "SLC_DEBUG"
        }
        links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}"
		}

    filter "configurations:Release"
		runtime "Release"
        optimize "on"
	    defines 
        {
            "SLC_RELEASE"
        }
        links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}