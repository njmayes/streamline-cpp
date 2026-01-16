import sys
import os
import subprocess
import platform

from VulkanSetup import VulkanConfiguration as VulkanRequirements
from OpenSSLSetup import OpenSSLConfiguration as OpenSSLRequirements

def main():
    os.chdir(f"{sys.path[0]}/../..") # Change working dir to repo root

    VulkanRequirements.Validate()
    OpenSSLRequirements.Validate()

    print("\nSetup completed!")
        
if __name__ == "__main__":
    main()