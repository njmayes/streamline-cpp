import sys
import os
import shutil
import subprocess
import platform

if platform.system() == "Windows":
    import UtilsWindows as Utils
else:
    import UtilsLinux as Utils

class OpenSSLConfiguration:

    @classmethod
    def Validate(cls):
        validation = Utils.CheckOpenSSLInstalled()
        if (not validation["dev_installed"]):
            print("\nYou don't have the OpenSSL installed!")
            Utils.InstallOpenSSL()
            return False

        # Set environment variable for build systems on Windows
        if validation["dev_path"] is not None:
            os.environ["OPENSSL_ROOT_DIR"] = validation["dev_path"]

        return True