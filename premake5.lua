workspace "streamline-cpp"
    startproject "TestApp"

    configurations 
    { 
        "Debug",
        "Release"
    }
    
    platforms
    {
        "x64",
        "ARM32",
        "ARM64"
    }

	filter "platforms:x64"
		architecture "x86_64"

	filter "platforms:ARM32"
		architecture "ARM"

 	filter "configurations:ARM64"
		architecture "ARM64"

outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["StreamlineCore"] 	= "%{wks.location}/StreamlineCore/src"
IncludeDir["glfw"] 	            = "%{wks.location}/StreamlineCore/dependencies/glfw/include"
IncludeDir["glad"] 	            = "%{wks.location}/StreamlineCore/dependencies/glad/include"
IncludeDir["imgui"] 	        = "%{wks.location}/StreamlineCore/dependencies/imgui"
IncludeDir["ImGuizmo"] 			= "%{wks.location}/StreamlineCore/dependencies/ImGuizmo"

include "StreamlineCore"
include "TestApp"

group "Dependencies"

include "StreamlineCore/dependencies/glfw"
include "StreamlineCore/dependencies/glad"
include "StreamlineCore/dependencies/imgui"

group ""