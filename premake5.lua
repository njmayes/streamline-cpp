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

 	filter "platforms:ARM64"
		architecture "ARM64"

VULKAN_SDK = os.getenv("VULKAN_SDK")
OPENSSL_ROOT = os.getenv("OPENSSL_ROOT_DIR")

outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["StreamlineCore"] 	= "%{wks.location}/StreamlineCore/src"
IncludeDir["asio"] 			    = "%{wks.location}/StreamlineCore/dependencies/asio/include"
IncludeDir["glfw"] 	            = "%{wks.location}/StreamlineCore/dependencies/glfw/include"
IncludeDir["glad"] 	            = "%{wks.location}/StreamlineCore/dependencies/glad/include"
IncludeDir["glm"] 				= "%{wks.location}/StreamlineCore/dependencies/glm"
IncludeDir["imgui"] 	        = "%{wks.location}/StreamlineCore/dependencies/imgui"
IncludeDir["ImGuizmo"] 			= "%{wks.location}/StreamlineCore/dependencies/ImGuizmo"
IncludeDir["json"] 			    = "%{wks.location}/StreamlineCore/dependencies/json"
IncludeDir["magic_enum"] 		= "%{wks.location}/StreamlineCore/dependencies/magic_enum"
IncludeDir["pfd"] 				= "%{wks.location}/StreamlineCore/dependencies/portable-file-dialogs"
IncludeDir["stb_image"] 		= "%{wks.location}/StreamlineCore/dependencies/stb_image"
IncludeDir["VulkanSDK"] 		= "%{VULKAN_SDK}/Include"

LibraryDir = {}
LibraryDir["VulkanSDK"] 		 	= "%{VULKAN_SDK}/Lib"

Library = {}

Library["asio"] 					= "asio"
Library["glad"] 					= "glad"
Library["glfw"] 					= "glfw"
Library["imgui"] 					= "imgui"

filter "system:windows"
    -- OpenSSL Configuration
    IncludeDir["OpenSSL"]        = "%{OPENSSL_ROOT}/include"

filter {}


include "StreamlineCore"
include "TestApp"

group "Dependencies"

include "StreamlineCore/dependencies/asio"
include "StreamlineCore/dependencies/glfw"
include "StreamlineCore/dependencies/glad"
include "StreamlineCore/dependencies/imgui"

group ""