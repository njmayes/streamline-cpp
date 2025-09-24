import os
import platform
import subprocess
import shutil
import configparser
from unicodedata import name
from pathlib import Path

def ReplaceNamespace(namespace):
    with open('premake5.lua', 'r') as premakeRoot:
        filedata = premakeRoot.read()
    filedata = filedata.replace('scpp-template', namespace)
    with open('premake5.lua', 'w') as premakeRoot:
        premakeRoot.write(filedata)

def ReplaceProjectName(projectName, projectType):
    with open('premake5.lua', 'r') as premakeRoot:
        filedata = premakeRoot.read()
    filedata = filedata.replace('<ProjectName>', projectName)
    with open('premake5.lua', 'w') as premakeRoot:
        premakeRoot.write(filedata)

    with open('premake5.lua', 'a') as premakeRoot:
        premakeRoot.write('\n')
        premakeRoot.write('IncludeDir["{0:s}"]\t= "%{{wks.location}}/{0:s}/src"\n'.format(projectName))
        premakeRoot.write('include "{0:s}"\n'.format(projectName))
        
    with open('{name}/premake5.lua'.format(name=projectName), 'r') as premakeProj:
        filedata = premakeProj.read()
    filedata = filedata.replace('<ProjectName>', projectName)
    filedata = filedata.replace('<AppType>', projectType)
    with open('{name}/premake5.lua'.format(name=projectName), 'w') as premakeProj:
        premakeProj.write(filedata)
    
class ProjectConfiguration:

    @classmethod
    def CheckProjectConfig(cls):
        return not os.path.isdir('templates/TemplateProject')

    @classmethod
    def SetupNamespace(cls, namespace):
        ReplaceNamespace(namespace)
        
    @classmethod
    def SetupProject(cls, projectName):
        shutil.copytree( 'templates/TemplateProject', projectName )

        projectTypeSetup = False
        projectType = ''
        while not projectTypeSetup:
            projectType = str(input("Please choose binary type of project, executable or library? [E/L]: ")).lower().strip()[:1]
            projectTypeSetup = (projectType == 'e' or projectType == 'l')
        
        match projectType:
            case 'l':
                projectType = 'StaticLib' if platform.system() == "Windows" else 'SharedLib'
            case 'e':
                projectType = 'ConsoleApp'
            case _:
                return

        ReplaceProjectName(projectName, projectType)