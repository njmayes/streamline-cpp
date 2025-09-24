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
def DownloadFile(url, filepath):
    filepath = os.path.abspath(filepath)
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
            
    if (type(url) is list):
        for url_option in url:
            print("Downloading", url_option)
            try:
                DownloadFile(url_option, filepath)
                return
            except urllib.error.URLError as e:
                print(f"URL Error encountered: {e.reason}. Proceeding with backup...\n\n")
                os.remove(filepath)
                pass
            except urllib.error.HTTPError as e:
                print(f"HTTP Error  encountered: {e.code}. Proceeding with backup...\n\n")
                os.remove(filepath)
                pass
            except:
                print(f"Something went wrong. Proceeding with backup...\n\n")
                os.remove(filepath)
                pass
        raise ValueError(f"Failed to download {filepath}")
    if not(type(url) is str):
        raise TypeError("Argument 'url' must be of type list or string")

    with open(filepath, 'wb') as f:
        headers = {'User-Agent': "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.97 Safari/537.36"}
        response = requests.get(url, headers=headers, stream=True)
        total = response.headers.get('content-length')

        if total is None:
            f.write(response.content)
        else:
            downloaded = 0
            total = int(total)
            startTime = time.time()
            for data in response.iter_content(chunk_size=max(int(total/1000), 1024*1024)):
                downloaded += len(data)
                f.write(data)
                
                try:
                    done = int(50*downloaded/total) if downloaded < total else 50
                    percentage = (downloaded / total) * 100 if downloaded < total else 100
                except ZeroDivisionError:
                    done = 50
                    percentage = 100
                elapsedTime = time.time() - startTime
                try:
                    avgKBPerSecond = (downloaded / 1024) / elapsedTime
                except ZeroDivisionError:
                    avgKBPerSecond = 0.0

                avgSpeedString = '{:.2f} KB/s'.format(avgKBPerSecond)
                if (avgKBPerSecond > 1024):
                    avgMBPerSecond = avgKBPerSecond / 1024
                    avgSpeedString = '{:.2f} MB/s'.format(avgMBPerSecond)
                sys.stdout.write('\r[{}{}] {:.2f}% ({})     '.format('█' * done, '.' * (50-done), percentage, avgSpeedString))
                sys.stdout.flush()
    sys.stdout.write('\n')

def UnpackFile(filepath, _, deleteTarFile=True):
    tarFilePath = os.path.abspath(filepath) # get full path of files

    myTar = tarfile.open(tarFilePath)
    myTar.extractall(os.path.dirname(tarFilePath))
    myTar.close()

    if deleteTarFile:
        os.remove(tarFilePath) # delete tar file
        
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