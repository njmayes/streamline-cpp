import sys
import os
import subprocess
import platform

from PremakeSetup import PremakeConfiguration as PremakeRequirements
from ProjectSetup import ProjectConfiguration as ProjectConfig
from VulkanSetup import VulkanConfiguration as VulkanRequirements
from OpenSSLSetup import OpenSSLConfiguration as OpenSSLRequirements

def main():
    os.chdir(f"{sys.path[0]}/../..") # Change working dir to repo root
    projectConfigured = ProjectConfig.CheckProjectConfig()
    premakeInstalled = PremakeRequirements.Validate()

    VulkanRequirements.Validate()
    OpenSSLRequirements.Validate()

    if (not projectConfigured):
        namespace = str(input("Enter the top level name for the repo...\n")).strip()
        ProjectConfig.SetupNamespace(namespace)

    if (not projectConfigured):
        projectName = str(input("Enter the name for the template project...\n")).strip()
        ProjectConfig.SetupProject(projectName)

    if (platform.system() == "Windows"):
        if (premakeInstalled):
            print("\nRunning premake...")
            subprocess.call([os.path.abspath("./scripts/gen-projects/msvc.bat"), "nopause"])
        else:
            print("This project requires Premake to generate project files.")

    print("\nSetup completed!")
        
if __name__ == "__main__":
    main()