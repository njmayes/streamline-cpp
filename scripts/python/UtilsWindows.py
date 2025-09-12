from re import T
import sys
import os
import winreg

import requests
import time
import urllib

from zipfile import ZipFile
from pathlib import Path

def DownloadFile(url, filepath):
    path = filepath
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

def UnpackFile(filepath, filter, deleteZipFile=True):
    zipFilePath = os.path.abspath(filepath) # get full path of files
    zipFileLocation = os.path.dirname(zipFilePath)
    zipFileContent = dict()
    zipFileContentSize = 0

    with ZipFile(zipFilePath, 'r') as zipFileFolder:
        for name in zipFileFolder.namelist():
            valid = False
            for extension in filter:
                if (Path(name).suffix == extension):
                    valid = True
                    break
            if (len(filter) == 0):
                valid = True

            if (not valid):
                continue
            zipFileContent[name] = zipFileFolder.getinfo(name).file_size

        zipFileContentSize = sum(zipFileContent.values())
        extractedContentSize = 0
        startTime = time.time()

        for zippedFileName, zippedFileSize in zipFileContent.items():
            UnzippedFilePath = os.path.abspath(f"{zipFileLocation}/{zippedFileName}")
            os.makedirs(os.path.dirname(UnzippedFilePath), exist_ok=True)
            if os.path.isfile(UnzippedFilePath):
                zipFileContentSize -= zippedFileSize
            else:
                zipFileFolder.extract(zippedFileName, path=zipFileLocation, pwd=None)
                extractedContentSize += zippedFileSize
            try:
                done = int(50*extractedContentSize/zipFileContentSize)
                percentage = (extractedContentSize / zipFileContentSize) * 100
            except ZeroDivisionError:
                done = 50
                percentage = 100
            elapsedTime = time.time() - startTime
            try:
                avgKBPerSecond = (extractedContentSize / 1024) / elapsedTime
            except ZeroDivisionError:
                avgKBPerSecond = 0.0
            avgSpeedString = '{:.2f} KB/s'.format(avgKBPerSecond)
            if (avgKBPerSecond > 1024):
                avgMBPerSecond = avgKBPerSecond / 1024
                avgSpeedString = '{:.2f} MB/s'.format(avgMBPerSecond)
            sys.stdout.write('\r[{}{}] {:.2f}% ({})     '.format('█' * done, '.' * (50-done), percentage, avgSpeedString))
            sys.stdout.flush()

    sys.stdout.write('\n')

    if deleteZipFile:
        os.remove(zipFilePath) # delete zip file


def InstallVulkan(version):
    vulkanDirectory = "./streamline-cpp/dependencies/VulkanSDK"
    vulkanFilename = "vulkan_sdk.exe"
    vulkanExecPath = f"{vulkanDirectory}/VulkanSDK-{version}-Installer.exe"
    

    vulkanInstallURL = f"https://sdk.lunarg.com/sdk/download/{version}/windows/{vulkanFilename}"
    vulkanInstallPath = f"{vulkanDirectory}/{vulkanFilename}"
    print("Downloading {0:s} to {1:s}".format(vulkanInstallURL, vulkanInstallPath))
    DownloadFile(vulkanInstallURL, vulkanInstallPath)
    print("Running Vulkan SDK installer...")        
    
    os.startfile(os.path.abspath(vulkanExecPath))
    print("Re-run this script after installation!")


def __CheckOpenSSLDevDir(root):
    """Check if development files exist in a given directory."""
    include_path = os.path.join(root, "include", "openssl", "ssl.h")
    lib_dir = os.path.join(root, "lib")
    
    has_libs = False

    for root, _, files in os.walk(lib_dir):
        if "libssl.lib" in files and "libcrypto.lib" in files:
           has_libs = True

    return os.path.exists(include_path) and has_libs

def __FindOpenSSLFromEnv():
    root = os.environ.get("OPENSSL_ROOT_DIR")
    if root and __CheckOpenSSLDevDir(root):
        return root
    return None

def __FindOpenSSLFromPath():
    """Check PATH for openssl.exe and infer dev directory."""
    paths = os.environ.get("PATH", "").split(os.pathsep)
    for p in paths:
        # Heuristic: assume include/lib are one level up from the bin folder
        candidate = os.path.abspath(os.path.join(p, ".."))
        if __CheckOpenSSLDevDir(candidate):
            return candidate
    return None

def __FindOpenSSLInCommonDirs():
    common_dirs = [
        "C:\\Program Files\\OpenSSL-Win64",
        "C:\\Program Files (x86)\\OpenSSL-Win32",
        "C:\\OpenSSL-Win64",
        "C:\\OpenSSL-Win32"
    ]
    for d in common_dirs:
        if os.path.exists(d) and __CheckOpenSSLDevDir(d):
            return d
    return None

def __FindOpenSSLRecursively(dirs=None):
    print("Searching for OpenSSL installation recursively (this may take a while)...")
    if dirs is None:
        dirs = ["C:\\", "C:\\Program Files", "C:\\Program Files (x86)"]

    for root_dir in dirs:
        for root, _, files in os.walk(root_dir):
            if "ssl.h" in files and "libssl.lib" in files and "libcrypto.lib" in files:
                return os.path.abspath(root_dir)
    return None

def CheckOpenSSLInstalled(search_recusively=False):
    dev_path = (
        __FindOpenSSLFromEnv() or
        __FindOpenSSLFromPath() or
        __FindOpenSSLInCommonDirs()
    )

    # If still not found, optionally use slow recursive search
    if not dev_path and search_recusively:
        dev_path = __FindOpenSSLRecursively()

    dev_installed = dev_path is not None
    return {
        "dev_installed": dev_installed,
        "dev_path": dev_path
    }


def __AddOpenSSLEnvironmentVariable():
    validation = CheckOpenSSLInstalled(search_recusively=True)

    path = validation["dev_path"]
    if path is None:
        raise EnvironmentError("OpenSSL development files not found following installation.")

    key = winreg.HKEY_CURRENT_USER
    sub_key = r"Environment"

    # Open registry key for writing
    with winreg.OpenKey(key, sub_key, 0, winreg.KEY_SET_VALUE) as reg_key:
        winreg.SetValueEx(reg_key, "OPENSSL_ROOT_DIR", 0, winreg.REG_SZ, path)

    # Notify Windows about the environment change (so new processes see it)
    import ctypes
    HWND_BROADCAST = 0xFFFF
    WM_SETTINGCHANGE = 0x1A
    SMTO_ABORTIFHUNG = 0x0002
    ctypes.windll.user32.SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, "Environment", SMTO_ABORTIFHUNG, 5000, None)

    print(f"Environment variable OPENSSL_ROOT_DIR set to {path}.")

def InstallOpenSSL():
    import requests
    from zipfile import ZipFile

    openssl_url = "https://slproweb.com/download/Win64OpenSSL-3_5_2.exe"
    installer_path = os.path.abspath("./Win64OpenSSL-3_5_2.exe")
    print(f"Downloading OpenSSL installer from {openssl_url}...")
    with requests.get(openssl_url, stream=True) as r:
        r.raise_for_status()
        with open(installer_path, "wb") as f:
            for chunk in r.iter_content(chunk_size=8192):
                f.write(chunk)

    print("Running OpenSSL installer...")
    os.startfile(installer_path)

    input("Please follow the installer prompts. Hit Enter to continue after installation.")
    os.remove(installer_path)
    __AddOpenSSLEnvironmentVariable()

    print("OpenSSL installation complete. Please re-run this script.") 

    quit()