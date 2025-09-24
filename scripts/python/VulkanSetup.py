import os
import platform
from pathlib import Path
import Common

if platform.system() == "Windows":
    import UtilsWindows as Utils
elif platform.system() == "Linux":
    import UtilsLinux as Utils
else:
    raise ImportError("Unsupported platform")

class VulkanConfiguration:
    requiredVulkanVersion = "1.3."
    installVulkanVersion = "1.3.216.0"

    @classmethod
    def Validate(cls):
        if (not cls.__CheckVulkanSDK()):
            print("Vulkan SDK not installed correctly.")
            return
            
        if (not cls.__CheckVulkanSDKDebugLibs()):
            print(f"\nNo Vulkan SDK debug libs found. Install Vulkan SDK with debug libs.")
            print(f"\nDebug configuration disabled.")

    @classmethod
    def __CheckVulkanSDK(cls):
        vulkanSDK = os.environ.get("VULKAN_SDK")
        if (vulkanSDK is None):
            print("\nYou don't have the Vulkan SDK installed!")
            cls.__InstallVulkanSDK()
            return False
        else:
            print(f"\nLocated Vulkan SDK at {vulkanSDK}")

        if (cls.requiredVulkanVersion not in vulkanSDK):
            print(f"You don't have the correct Vulkan SDK version! (Engine requires {cls.requiredVulkanVersion})")
            cls.__InstallVulkanSDK()
            return False
    
        print(f"Correct Vulkan SDK located at {vulkanSDK}")
        return True

    @classmethod
    def __InstallVulkanSDK(cls):
        install_job = lambda _: Utils.InstallVulkan(cls.installVulkanVersion)

        Common.PromptUserForTask( "Would you like to install VulkanSDK {0:s}? [Y/N]: ".format(cls.installVulkanVersion), install_job)

    @classmethod
    def __CheckVulkanSDKDebugLibs(cls):
        vulkanSDK = os.environ.get("VULKAN_SDK")
        shadercdLib = Path(f"{vulkanSDK}/Lib/shaderc_sharedd.lib")
        
        return shadercdLib.exists()