import platform
import Common

if platform.system() == "Windows":
    import UtilsWindows as Utils
elif platform.system() == "Linux":
    import UtilsLinux as Utils
else:
    raise ImportError("Unsupported platform")

class OpenSSLConfiguration:

    @classmethod
    def Validate(cls):
        validation = Utils.CheckOpenSSLInstalled()
        if (not validation["dev_installed"]):
            print("\nYou don't have the OpenSSL SDK installed!")

            Common.PromptUserForTask("Would you like to install the OpenSSL SDK? [Y/N]: ", Utils.InstallOpenSSL )
            return False

        return True