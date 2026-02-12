#!/usr/bin/python3
# pylint: disable=locally-disabled, line-too-long, unused-argument, unused-variable, pointless-string-statement, too-many-locals, missing-docstring, too-many-statements, invalid-name, wrong-import-position, too-many-arguments, too-many-branches
# Copyright Novatek Microelectronics Corp. 2016.  All rights reserved.

import os
import sys
import re
import colorama
from colorama import Fore, Style
import sys, termios, tty, os, time
import fcntl

def getch():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)

    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def repo_to_dict(file_path):
    repo_dict = dict()
    re_repo = re.compile(r'<project groups=".*"\s*name="(.*)"\s*path="(.*)"\s*revision="(.*)" ')
    with open(file_path, 'rt',encoding="Big5", errors='ignore') as fin:
        line = fin.readline()
        while line:
            line = line.encode("ascii", errors="ignore").decode("ascii", errors="ignore")
            match = re.search(re_repo, line)
            if not match:
                line = fin.readline()
                continue
            name = match.group(1)
            path = match.group(2)
            revision = match.group(3)
            repo_dict[name] = {"path": path, "revision": revision}
            #print("{}, {}, {}".format(name, path, revision))
            line = fin.readline()
    return repo_dict

def repo_reset(repo_src, repo_dst, sdk_path):
    # check sanity
    for key, value in repo_src.items():
        name = key
        path = value["path"]
        revision = value["revision"]
        if name not in repo_dst:
            print(Fore.RED + "ERROR:")
            print("  between two xmls, the repo groups are not matched")
            print("  unable to find '{}' in your target xml.".format(name))
            print("  please check ./repo/manifest.xml and your target xml" + Style.RESET_ALL)
            return -1
    # show info
    print(Fore.YELLOW + "WARNING: {} will be reset and clean. ".format(sdk_path) + Style.RESET_ALL)
    for key, value in repo_src.items():
        name = key
        path = value["path"]
        revision = value["revision"]
        target_revision = repo_dst[name]["revision"]
        print("{} => {}".format(path, target_revision))

    print("are you sure? (y/n)")
    key = getch()
    while key != 'y' and key != 'n':
        print("press y or n.")
        key = getch()

    if key == 'n':
        return 0

    for key, value in repo_src.items():
        name = key
        path = value["path"]
        revision = value["revision"]
        target_revision = repo_dst[name]["revision"]
        dir = os.path.join(sdk_path, path)
        print(Fore.GREEN + "[processing {} => {}]".format(path, target_revision) + Style.RESET_ALL)
        cmd = \
            "pushd {} > /dev/null && ".format(dir) +\
            "git fetch && " +\
            "git reset --hard {} && ".format(target_revision) +\
            "git clean -xdf && " +\
            "popd > /dev/null"
        er = os.system(cmd)
        print("")
        if er != 0:
            print(Fore.RED + "failed to run: {}".format(cmd))
            return -1

    # rollback build/Makefile to ./Makefile
    top_folder = os.getenv('LINUX_BUILD_TOP')
    cmd = \
        "pushd {} > /dev/null && ".format(top_folder) +\
        "ln -s build/Makefile Makefile && " +\
        "popd > /dev/null"
    er = os.system(cmd)
    if er != 0:
            print(Fore.RED + "failed to run: {}".format(cmd))
            return -1

    return 0

def main(argv):
    colorama.init()
    sdk_path = os.getenv('LINUX_BUILD_TOP')
    if sdk_path is None:
        print(Fore.RED + "'source build/envsetup.sh' is required." + Style.RESET_ALL)
        return -1
    sdk_path = os.path.realpath(os.path.join(sdk_path, ".."))
    path_src = os.path.realpath(os.path.join(sdk_path, ".repo/manifest.xml"))
    if not os.path.isfile(path_src):
        print(Fore.RED + "cannot open xml: {}".format(path_src) + Style.RESET_ALL)
        return -1
    path_dst = sys.argv[1]
    if not os.path.isfile(path_dst):
        print(Fore.RED + "cannot open xml: {}".format(path_dst) + Style.RESET_ALL)
        return -1
    repo_src = repo_to_dict(path_src)
    repo_dst = repo_to_dict(path_dst)
    er = repo_reset(repo_src, repo_dst, sdk_path)
    return er

# ============================================================================================
# DO NOT EDIT FOLLOWING CODES
# ============================================================================================
if __name__ == "__main__":
    def local_run():
        if len (sys.argv) != 2:
            print("Usage: python3 repo_reset.py [path_to xml]");
            return -1
        err = main(sys.argv)
        return err
    sys.exit(local_run())
# ============================================================================================
