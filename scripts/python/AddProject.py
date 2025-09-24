import sys
import os
import subprocess
import platform

from PremakeSetup import PremakeConfiguration as PremakeRequirements
from ProjectSetup import ProjectConfiguration as ProjectConfig

def main():
    os.chdir(f"{sys.path[0]}/../..") # Change working dir to repo root
    projectConfigured = ProjectConfig.CheckProjectConfig()
    
    ProjectConfig.SetupStreamlineDependency()

    if (not projectConfigured):
        projectName = str(input("Enter the name for the new project...\n")).strip()
        ProjectConfig.SetupProject(projectName)

    if (platform.system() == "Windows"):
        premakeInstalled = PremakeRequirements.Validate()
        if (premakeInstalled):
            print("\nRunning premake...")
            subprocess.call([os.path.abspath("./scripts/gen-projects/msvc.bat"), "nopause"])
        else:
            print("This project requires Premake to generate project files.")

    print("\nSetup completed!")
        
if __name__ == "__main__":
    main()