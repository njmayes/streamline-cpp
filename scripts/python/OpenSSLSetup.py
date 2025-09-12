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
            print("\nYou don't have the OpenSSL SDK installed!")
            
            permissionGranted = False
            while not permissionGranted:
                reply = str(input("Would you like to install the OpenSSL SDK? [Y/N]: ")).lower().strip()[:1]
                if reply == 'n':
                    return False
                permissionGranted = (reply == 'y')

            Utils.InstallOpenSSL()
            return False

        return True