#!/usr/bin/env python3
"""unpack.py INPUTS DEST -- lay out every tool, library and game copy the kit reads, from the original
downloads in one folder (README.md lists them), the way their installers would, without running any.
Run by the Dockerfile; DEST is /opt/evnova, and build.ini names the folders below it. Every input is
optional: a target is laid out when all of its inputs are there, and DEST/targets.txt lists those."""
import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

import installshield

# the originals, as supplied: name in the inputs folder, SHA-256 (None: your own file, any content).
INPUTS = {
    "cw92": ("CW_Windows_9.2.exe", "86608e30a15f501065fc0e0aab55b0ac3372aa02818f7f37f2aa20e47dccd6dc"),
    "cw93": ("CW_Windows_9.3_Update.exe", "3b23182e13646d076829861f46fa963066f04ec639bc06fba6c02edd2df96467"),
    "cw9lic": ("CW_Windows_9_license.dat", None),
    "qt6": ("QT6SDK_Win.hqx", "fbcb3822ac4a16a5d21fa6575677b17cfa888bf937f4d59e8aa7ad75d72efb11"),
    "win1010": ("EV_Nova_1.0.10_for_Windows.zip", "ce3650391697be80968cea2527a612e6e8cd836b898c355724d2671af4a5cb05"),
}
# the inputs each target needs
TARGETS = {
    "win": ["cw92", "cw93", "cw9lic", "qt6", "win1010"],
    "ce": ["cw92", "cw93", "cw9lic", "qt6", "win1010"],
}
WORK = Path("/var/tmp/unpack")


def sh(*cmd):
    subprocess.run([str(c) for c in cmd], check=True, stdout=subprocess.DEVNULL)


def fresh(name):
    d = WORK / name
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    return d


def check_inputs(src):
    """the inputs present, each checked; exits on a wrong copy"""
    have, bad = set(), 0
    for key, (name, sha) in INPUTS.items():
        p = src / name
        if not p.is_file():
            continue
        h = hashlib.sha256()
        with open(p, "rb") as f:
            for block in iter(lambda: f.read(1 << 20), b""):
                h.update(block)
        ok = sha is None or h.hexdigest() == sha
        print("  %s %s" % ("ok      " if ok else "WRONG   ", name))
        bad += not ok
        have.add(key)
    if bad:
        sys.exit("unpack: an input is not the expected copy (README.md lists them)")
    return have


def cw9(src, dest):
    # CodeWarrior for Windows 9.2 and its 9.3 update: mwcc, mwld, mwwinrc 3.2.4 build 418
    cw = dest / "cw-win-9.2-with-9.3-update"
    w = fresh("cw92")
    sh("7z", "x", "-y", "-o%s" % w, src / INPUTS["cw92"][0])
    installshield.groups(str(w / "data1.cab"), str(cw), ["Tools"])
    w = fresh("cw93")
    installshield.sfx7(src / INPUTS["cw93"][0], str(w))
    installshield.groups(str(w / "Disk1" / "data1.cab"), str(cw), ["Patch Files"])
    shutil.copyfile(src / INPUTS["cw9lic"][0], cw / "license.dat")


def qt6(src, dest):
    # Apple's QuickTime 6 SDK for Windows (BinHex around a zip): qtmlClient.lib
    w = fresh("qt6")
    sh("unar", "-q", "-o", w, src / INPUTS["qt6"][0])
    sh("7z", "e", "-y", "-o%s" % (dest / "apple-quicktime-6-sdk"), w / "QT6SDK.zip",
       "QT6 SDK (Win)/Interfaces & Libraries/QTDevWin/Libraries/qtmlClient.lib")


def win1010(src, dest):
    w = fresh("win1010")
    sh("7z", "x", "-y", "-o%s" % w, src / INPUTS["win1010"][0], "EV Nova 1.0.10 for Windows/EV Nova 1.0.10.exe")
    sh("7z", "e", "-y", "-o%s" % (dest / "game" / "win-1.0.10"), w / "EV Nova 1.0.10 for Windows" / "EV Nova 1.0.10.exe",
       "EV Nova.exe")


# each unpacking step and the inputs it reads
STEPS = [(cw9, ["cw92", "cw93", "cw9lic"]), (qt6, ["qt6"]), (win1010, ["win1010"])]


def main(src, dest):
    src, dest = Path(src), Path(dest)
    print("checking the inputs in %s ..." % src)
    have = check_inputs(src)
    targets, used = [], set()
    for t, need in TARGETS.items():
        gone = [INPUTS[k][0] for k in need if k not in have]
        if gone:
            print("  %-8s left out: no %s" % (t, ", ".join(gone)))
        else:
            targets.append(t)
            used |= set(need)
    for step, keys in STEPS:
        if set(keys) <= used:
            print("unpacking: %s" % step.__name__)
            step(src, dest)
    shutil.rmtree(WORK, ignore_errors=True)
    dest.mkdir(parents=True, exist_ok=True)
    (dest / "targets.txt").write_text("".join(t + "\n" for t in targets))
    print("unpack: done; targets: %s" % (" ".join(targets) or "none"))


if __name__ == "__main__":
    main(*sys.argv[1:3])
