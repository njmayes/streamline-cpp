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
        "%{IncludeDir.asio}",
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
        "%{Library.asio}",
        "%{Library.glad}",
        "%{Library.glfw}",
        "%{Library.imgui}",
	}

    filter "action:vs*"
		buildoptions 
		{
			"/Zc:preprocessor"
		}
        
	
    filter "system:windows"
        kind "StaticLib"
        systemversion "latest"

        includedirs
        {
            "%{IncludeDir.OpenSSL}",
        }

        links
        {
            "opengl32.lib",
            "Ws2_32.lib",
            "Winmm.lib",
            "Version.lib",
            "Bcrypt.lib",
            "libssl.lib",
            "libcrypto.lib",
        }
        
    filter { "system:windows", "configurations:Debug", "platforms:x64" }
        libdirs { "%{OPENSSL_ROOT}/lib/VC/x64/MDd" }
        
    filter { "system:windows", "configurations:Release", "platforms:x64" }
        libdirs { "%{OPENSSL_ROOT}/lib/VC/x64/MD" }

    filter { "system:windows", "configurations:Debug", "platforms:ARM64" }
        libdirs { "%{OPENSSL_ROOT}/lib/VC/ARM64/MDd" }

    filter { "system:windows", "configurations:Release", "platforms:ARM64" }
        libdirs { "%{OPENSSL_ROOT}/lib/VC/ARM64/MD" }
		
	filter "system:linux"
        kind "SharedLib"
        pic "On"
        systemversion "latest"

        links
        {
            "ssl",
            "crypto",
        }

    filter "configurations:Debug"
		runtime "Debug"
        symbols "on"
	    defines 
        {
            "SLC_DEBUG"
        }
        links
		{
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
		}