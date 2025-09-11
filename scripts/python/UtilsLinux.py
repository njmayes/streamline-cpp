import sys
import os

import requests
import time
import urllib
import subprocess
import shutil

import tarfile
from pathlib import Path

PACKAGE_MANAGERS = {
    "apt": ["sudo", "apt", "install", "-y"],
    "dnf": ["sudo", "dnf", "install", "-y"],
    "yum": ["sudo", "yum", "install", "-y"],
    "pacman": ["sudo", "pacman", "-S", "--noconfirm"],
    "zypper": ["sudo", "zypper", "install", "-y"]
}

def DetectPackageManager():
    for pm in PACKAGE_MANAGERS.keys():
        if shutil.which(pm):
            return pm
    return None

def AddRepository(name, repo_url, key_url=None):
    pm = DetectPackageManager()
    if pm is None:
        raise EnvironmentError("No supported package manager found!")

    if pm == "apt":
        if key_url:
            subprocess.run(f"wget -qO- {key_url} | sudo tee /etc/apt/trusted.gpg.d/{name}.asc",
                           shell=True, check=True)
        subprocess.run(f"sudo wget -qO /etc/apt/sources.list.d/{name}.list {repo_url}",
                       shell=True, check=True)
        subprocess.run("sudo apt update", shell=True, check=True)

    elif pm in ["dnf", "yum"]:
        repo_file = f"/etc/yum.repos.d/{name}.repo"
        subprocess.run(f"sudo wget -qO {repo_file} {repo_url}", shell=True, check=True)
        if pm == "dnf":
            subprocess.run("sudo dnf makecache", shell=True, check=True)
        else:
            subprocess.run("sudo yum makecache", shell=True, check=True)

    elif pm == "pacman":
        print(f"Please add repo manually in /etc/pacman.conf for {name}")

def CheckOpenSSLInstalled():
    include_paths = [
        "/usr/include/openssl/ssl.h",
        "/usr/local/include/openssl/ssl.h"
    ]

    library_paths = [
        "/usr/lib/x86_64-linux-gnu/libssl.so",
        "/usr/lib64/libssl.so",
        "/usr/local/lib/libssl.so"
    ]

    header_exists = any(os.path.exists(p) for p in include_paths)
    lib_exists = any(os.path.exists(p) for p in library_paths)

    dev_installed = header_exists and lib_exists
    
    return {
        "dev_installed": dev_installed,
        "dev_path": None
    }

def InstallPackage(package_name):
    pm = DetectPackageManager()
    if pm is None:
        raise EnvironmentError("No supported package manager found!")

    cmd = PACKAGE_MANAGERS[pm] + [package_name]
    subprocess.run(cmd, check=True)
    
def InstallVulkan(version):
    AddRepository( name="lunarg", repo_url="http://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list", key_url="https://packages.lunarg.com/lunarg-signing-key-pub.asc" )
    InstallPackage("vulkan-sdk")

def InstallOpenSSL():
    pm = DetectPackageManager()
    if pm is None:
        raise EnvironmentError("No supported package manager found!")

    if pm == "apt":
        InstallPackage("libssl-dev")

    elif pm in ["dnf", "yum"]:
        InstallPackage("openssl-devel")

    elif pm == "pacman":
        InstallPackage("openssl")