"""What `build.py check` verifies: every tool, library and original the targets read, by size, CRC32 and
SHA-256. `exact` rows must match."""
from .host import WINE, BuildError, crc32, run, say, sha256

CLI = "Other Metrowerks Tools/Command Line Tools/"

# (targets, build.ini section, key, path inside it, size, CRC32, SHA-256, kind)
FILES = [
    # CodeWarrior for Windows 9.2 with its 9.3 update: mwcc/mwld/mwwinrc 3.2.4 build 418
    ("win ce", "win", "codewarrior", CLI + "mwcc.exe", 2445312, "a797f81c", "0f9a7167a5d4a8175865d25efcdee13f903d74a85af7365136b55b8fbe52363d", "exact"),
    ("win ce", "win", "codewarrior", CLI + "mwld.exe", 921600, "39a1d419", "2f53d5be2b0f3081d43d5514893ed3182d20b25dccc6dfb61c82749b6fb2b460", "exact"),
    ("win ce", "win", "codewarrior", CLI + "mwwinrc.exe", 368640, "61aad37c", "42e5dc26c2fe8a8bdcaae80abbdc1aa7d0df1d96ea405afe1a70741a1b61ac59", "exact"),
    ("win ce", "win", "codewarrior", "Bin/lmgr8c.dll", 851968, "f997f290", "996041141858fd4724ec36add0913755a0067498b93271d7d26e4a1bfa04a561", "exact"),
    ("win ce", "win", "codewarrior", "Win32-x86 Support/Libraries/Runtime/Libs/MSL_All_x86.lib", 969696, "0635a099", "9ccc2025c52358ac7bbbe1ef9fd9a617817a41ff8d56c94f538275fe345bd707", "exact"),
    # Apple's QuickTime 6 SDK for Windows: the exe's QTML client library
    ("win ce", "win", "quicktime", "qtmlClient.lib", 774658, "bf1f090c", "9662e541e5e1b30b17888427940602dbe237d24c2199f5ca579a7bb294581faa", "exact"),
]

# Folders and files that must exist but have no single hash (headers, licences, SDK trees)
EXIST = [
    ("win ce", "win", "license", ""),
]


def check(cfg, targets):
    """Print one line per file; return the number of failures."""
    bad = 0
    for tg, sec, key, rel, size, crc, sha, kind in FILES:
        if not set(tg.split()) & set(targets):
            continue
        try:
            p = cfg.path(sec, key) / rel if rel else cfg.path(sec, key)
        except BuildError as e:
            say("  FAIL     %s" % e)
            bad += 1
            continue
        if not p.is_file():
            say("  MISSING  [%s] %s: %s" % (sec, key, p))
            bad += 1
            continue
        got = (p.stat().st_size, crc32(p), sha256(p))
        if got == (size, crc, sha):
            say("  ok       %s" % p)
        else:
            say("  WRONG    %s: %d B CRC32 %s SHA-256 %s, expected %d B %s %s" % (p, got[0], got[1], got[2], size, crc, sha))
            bad += 1
    for tg, sec, key, rel in EXIST:
        if not set(tg.split()) & set(targets):
            continue
        try:
            p = cfg.path(sec, key) / rel if rel else cfg.path(sec, key)
        except BuildError as e:
            say("  FAIL     %s" % e)
            bad += 1
            continue
        if p.exists():
            say("  ok       %s" % p)
        else:
            say("  MISSING  [%s] %s: %s" % (sec, key, p))
            bad += 1
    if set(targets) & {"win", "ce"}:
        try:
            rc, out = run(WINE + ["--version"], timeout=60)
            say("  %s %s" % ("ok      " if rc == 0 else "FAIL    ", out.strip() or "wine"))
            bad += rc != 0
        except BuildError as e:
            say("  MISSING  %s" % e)
            bad += 1
    return bad
