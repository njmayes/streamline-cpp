from typing import Callable

def PromptUserForTask( desc: str, job: Callable ):
    permissionGranted = False
    while not permissionGranted:
        reply = str(input(desc)).lower().strip()[:1]
        if reply == 'n':
            return False
        permissionGranted = (reply == 'y')

    job()
    return True