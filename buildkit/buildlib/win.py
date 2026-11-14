"""EV Nova.exe 1.0.10 for Windows, and the EV Nova CE build (-DCEFIX), with the original
Metrowerks tools: mwcc 3.2.4 b418 for every source, mwwinrc for the resources, mwld for the archives and the link.

The steps:
  1. compile every sources/**.c (not include/, v111/; cefix/ only for CE), each on BASELINE plus the
     flags of its `// COMPILE:` marker (`-lang c++ -bool off` for the C++ files)
  2. the eleven QuickTime client-library members, copied with the names the tree also defines
     renamed away (tables/qtorder.tsv)
  3. the link inputs: the game's objects, then rt.lib (sources/runtime/) and qt.lib
  4. the resources: evnova.rc by mwwinrc, the icons read out of the user's own exe
  5. the link with MSL, QuickTime and the Windows libraries, then (CE) the large-address-aware bit
"""
import concurrent.futures
import re

from . import coff, pe
from .host import KIT, REPO, BuildError, exe_env, run_exe, say, winlist, winpath

SOURCES = REPO / "sources"
BASELINE = ["-opt", "level=4,intrinsics", "-enum", "min", "-Cpp_exceptions", "off"]
MARKER = re.compile(r"^\s*//\s*COMPILE:\s*(.+)$")
QT_SHADOW = "__qtmlshadow"


class Tools:
    """The Windows toolchain as build.ini names it."""

    def __init__(self, cfg):
        cw = self.cw = cfg.path("win", "codewarrior")
        cli = cw / "Other Metrowerks Tools" / "Command Line Tools"
        self.mwcc, self.mwld, self.mwwinrc = cli / "mwcc.exe", cli / "mwld.exe", cli / "mwwinrc.exe"
        self.dlls = [cw / "Bin"]                                     # LMGR8C.dll
        self.license = cfg.path("win", "license")
        self.includes = [cw / p for p in (
            "MSL/MSL_C/MSL_Common/Include", "MSL/MSL_C/MSL_Win32/Include", "MSL/MSL_C/MSL_X86",
            "MSL/MSL_C++/MSL_Common/Include", "MSL/MSL_Extras/MSL_Common/Include", "MSL/MSL_Extras/MSL_Win32/Include",
            "Win32-x86 Support/Headers/Win32 SDK", "Win32-x86 Support/Headers/Precompiled Headers",
            "Win32-x86 Support/Headers/Prefix Files", "Win32-x86 Support/Headers/VCPP Headers",
            "Win32-x86 Support/Headers/Alloca", "Win32-x86 Support/Headers/Intrinsics", "MSL/MSL_C++/MSL_Precompiled_Headers")]
        self.libdirs = [cw / p for p in (
            "Win32-x86 Support/Libraries/Runtime/Libs", "Win32-x86 Support/Libraries/Win32 SDK",
            "MSL/MSL_C/MSL_Win32/Lib/x86", "MSL/MSL_C++/MSL_Win32/Lib/x86", "MSL/MSL_Extras/MSL_Win32/Lib/x86")]

    def env(self, **more):
        base = dict(LM_LICENSE_FILE=winpath(self.license), METROWKS_LICENSE_FILE=winpath(self.license),
                    MWCWinx86Includes=winlist(self.includes))
        base.update(more)
        return exe_env(self.dlls, **base)


# -- 1. compile ------------------------------------------------------------------------------------
def sources_for(cefix):
    out, stems = [], {}
    for p in sorted(SOURCES.rglob("*.c")):
        parts = p.relative_to(SOURCES).parts
        if parts[0] in ("include", "mac", "v111") or (parts[0] == "cefix" and not cefix):
            continue
        if p.stem.lower() in stems:
            raise BuildError("two sources with the stem %s: %s, %s" % (p.stem, stems[p.stem.lower()], p))
        stems[p.stem.lower()] = p
        out.append(p)
    return out


def compile_one(t, src, objdir, cefix):
    head = src.read_text(encoding="latin-1").splitlines()[:30]
    flags = BASELINE + next((MARKER.match(l).group(1).split() for l in head if MARKER.match(l)), [])
    flags = flags + ["-i", winpath(SOURCES / "include")] + (["-DCEFIX"] if cefix else [])
    obj = objdir / (src.stem + ".o")
    if obj.exists():
        obj.unlink()
    rc, msg = run_exe(t.mwcc, ["-c"] + flags + ["-o", winpath(obj), winpath(src)], env=t.env())
    if rc or not obj.exists():
        if obj.exists():
            obj.unlink()                         # never leave an object that would link as if current
        raise BuildError("compile failed: %s\n%s" % (src.relative_to(REPO), msg[-3000:]))
    return src


def compile_all(cfg, objdir, cefix):
    t = Tools(cfg)
    objdir.mkdir(parents=True, exist_ok=True)
    for old in objdir.glob("*.o"):
        old.unlink()
    srcs = sources_for(cefix)
    say("  compiling %d files (%d at a time) ..." % (len(srcs), cfg.jobs("win")))
    with concurrent.futures.ThreadPoolExecutor(cfg.jobs("win")) as ex:
        for f in [ex.submit(compile_one, t, s, objdir, cefix) for s in srcs]:
            f.result()
    return t


# -- 2. the QuickTime client-library members --------------------------------------------------------
def table(name):
    """A tab-separated table of buildkit/tables/: rows of fields."""
    rows = []
    for line in (KIT / "tables" / name).read_text().splitlines():
        if line and not line.startswith("#"):
            rows.append(line.split("\t"))
    return rows


def qt_members(qtlib, objdir, outdir):
    """Write qt_<member> for every row of tables/qtorder.tsv, with the names the tree also defines
    renamed away (and the table's own renames); return their paths."""
    lib = {}
    for name, body in coff.members(qtlib / "qtmlClient.lib"):
        lib.setdefault(name, []).append(body)
    tree = set()
    for o in objdir.glob("*.o"):
        tree |= coff.defined(o.read_bytes())
    outdir.mkdir(parents=True, exist_ok=True)
    out = []
    for row in table("qtorder.tsv"):
        name = row[0]
        ren = dict(p.split("=") for p in row[2].split(",")) if len(row) > 2 and row[2] else {}
        if len(lib.get(name, [])) != 1:
            raise BuildError("qtmlClient.lib: %d members named %s" % (len(lib.get(name, [])), name))
        b = lib[name][0]
        defs = {n for _i, n, sec, sc, _v in coff.symbols(b) if sc == coff.EXTERNAL and sec > 0}
        allren = dict(ren)
        allren.update({n: QT_SHADOW + n for n in defs if n not in ren and n in tree})
        if allren:
            b = coff.rename(b, allren)
        p = outdir / ("qt_" + name)
        p.write_bytes(b)
        out.append(p)
    return out


# -- 3. the link inputs ----------------------------------------------------------------------------
def link_inputs(objdir, work, qtmembers):
    """The game's objects as plain inputs, then the runtime (sources/runtime/) as rt.lib and the
    QuickTime members as qt.lib, whose members the link pulls as they are referenced."""
    runtime = {p.stem.lower() for p in (SOURCES / "runtime").glob("*.c")}
    objs = sorted(objdir.glob("*.o"), key=lambda o: o.name.lower())
    plain = [o for o in objs if o.stem.lower() not in runtime]
    rt = [o for o in objs if o.stem.lower() in runtime]
    coff.write_archive(work / "rt.lib", [(o.name, o.read_bytes()) for o in rt])
    coff.write_archive(work / "qt.lib", [(p.name, p.read_bytes()) for p in qtmembers])
    return plain + [work / "rt.lib", work / "qt.lib"]


# -- 4. resources ----------------------------------------------------------------------------------
TYPE_NAME = {3: "ICON", 14: "GROUP_ICON"}


def resources(t, exe, work):
    """novaport.res: VERSION and STRING from sources/rsrc/evnova.rc, ICON and GROUP_ICON read out of the
    user's exe into work/rsrc/ (never stored anywhere else)."""
    rsrc = work / "rsrc"
    rsrc.mkdir(parents=True, exist_ok=True)
    assets = [(ty, rid, b) for ty, rid, b in pe.resources(exe) if ty in (3, 14)]
    if sorted((ty, rid) for ty, rid, _b in assets) != [(3, 1), (14, 202)]:
        raise BuildError("%s: not the 1.0.10 icon set (ICON 1, GROUP_ICON 202)" % exe)
    for ty, rid, b in assets:
        (rsrc / ("%s_%s.bin" % (TYPE_NAME.get(ty, ty), rid))).write_bytes(b)
    res = work / "novaport.res"
    rc, msg = run_exe(t.mwwinrc, ["-cwd", "proj", "-d", "EVNOVA_ASSETS", "-o", winpath(res),
                                  winpath(SOURCES / "rsrc" / "evnova.rc")], cwd=work, env=t.env())
    if rc or not res.exists():
        raise BuildError("mwwinrc failed\n" + msg[-2000:])
    return res


# -- 5. the link -----------------------------------------------------------------------------------
LIBS = ["MSL_All_x86.lib", "qtmlClient.lib", "kernel32.lib", "user32.lib", "gdi32.lib", "winmm.lib", "wsock32.lib",
        "advapi32.lib", "comctl32.lib", "shell32.lib", "version.lib", "comdlg32.lib", "iphlpapi.lib", "ole32.lib",
        "oleaut32.lib", "opengl32.lib"]


def link(cfg, objdir, work, out_exe, cefix):
    t = Tools(cfg)
    work.mkdir(parents=True, exist_ok=True)
    exe = cfg.path("game", "win1010") / "EV Nova.exe"
    qtm = qt_members(cfg.path("win", "quicktime"), objdir, work / "qt")
    inputs = link_inputs(objdir, work, qtm) + [resources(t, exe, work)]
    # iconv.dll is loaded through an import library made from tables/iconv.def (the hints of the DLL the
    # exe was linked against); CE draws text without it, and then the DLL leaves the import table
    if any("libiconv" in n for o in objdir.glob("*.o") for n in coff.undefined(o.read_bytes())):
        lib = work / "iconv_imp.lib"
        if lib.exists():
            lib.unlink()
        run_exe(t.mwld, ["-importlib", "-o", winpath(lib), "-name", "iconv.dll", winpath(KIT / "tables" / "iconv.def")], env=t.env())
        if not lib.exists():
            raise BuildError("mwld -importlib did not write iconv_imp.lib")
        inputs.append(lib)
    rsp = work / "link.rsp"
    rsp.write_text("".join('"%s"\n' % winpath(p) for p in inputs))
    if out_exe.exists():
        out_exe.unlink()
    libdirs = t.libdirs + [cfg.path("win", "quicktime")]
    env = t.env(MWWinx86Libraries=winlist(libdirs), MWWinx86LibraryFiles=";".join(LIBS + (["GdiPlus.lib"] if cefix else [])))
    say("  linking %d inputs ..." % len(inputs))
    # -filealign 512 and -checksum: the original's header (FileAlignment 512, CheckSum set)
    rc, log = run_exe(t.mwld, ["-subsystem", "windows", "-subsysver", "4,0", "-filealign", "512", "-checksum",
                               "-o", winpath(out_exe), "-map", winpath(work / "link.map"), "-maxerrors", "0",
                               "-maxwarnings", "0", "@" + winpath(rsp)], env=env)
    (work / "link.log").write_text(log)
    undef = sorted(set(re.findall(r"Undefined symbol:\s*'([^']+)'", log)))
    if undef:
        raise BuildError("link: %d undefined symbols, first %s (log: %s)" % (len(undef), " ".join(undef[:8]), work / "link.log"))
    if rc or not out_exe.exists():
        raise BuildError("link failed (log: %s)\n%s" % (work / "link.log", log[-2000:]))
    # mwld keeps one definition of a clash, warns and exits 0. The one expected kind: a qtmlClient.lib
    # Toolbox-shim COMDAT losing to the port's own function of that name
    blocks = re.findall(r"Multiply defined section:\s*(\S+) in\s*\r?\n#\s*files ([^\r\n]*)\r?\n#\s*keeping definition in ([^\r\n]*)", log)
    bad = [b[0] for b in blocks if not ("(qtmlClient.lib)" in b[1] and re.match(r"^[A-Za-z0-9_]+\.o$", b[2].strip()))]
    if bad or len(blocks) != log.count("Multiply defined section:"):
        raise BuildError("link: a definition was silently displaced: %s (log: %s)" % (" ".join(bad) or "?", work / "link.log"))
    return exe


def build(cfg, out, cefix=False):
    """Compile, link and finish one Windows target into OUT; returns the exe's path."""
    name = "EV Nova CE.exe" if cefix else "EV Nova.exe"
    objdir, work, exe = out / "obj", out / "link", out / name
    say("  [1/%d] compile" % (3 if cefix else 2) + (" (-DCEFIX, sources/cefix/ included)" if cefix else ""))
    compile_all(cfg, objdir, cefix)
    say("  [2/%d] link" % (3 if cefix else 2))
    link(cfg, objdir, work, exe, cefix)
    if cefix:
        say("  [3/3] large-address-aware")
        pe.set_large_address_aware(exe)      # CE links with --large-address-aware for its 2,040 MB pool
    return exe
