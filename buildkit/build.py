#!/usr/bin/env python3
"""Build EV Nova from this repository's sources.

    python buildkit/build.py check              verify every tool and original build.ini names
    python buildkit/build.py win                EV Nova.exe for Windows
    python buildkit/build.py ce                 the EV Nova CE build of the same sources (-DCEFIX)
    python buildkit/build.py all                both (those the image has the inputs of)

Options:
    --config FILE    another build.ini (default: buildkit/build.ini)

Every output goes to buildkit/out/<target>/. At the end each one's SHA-256 is printed.
"""
import argparse
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from buildlib import host, toolchain, win  # noqa: E402
from buildlib.host import BuildError, say  # noqa: E402

TARGETS = ["win", "ce"]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0], formatter_class=argparse.RawDescriptionHelpFormatter,
                                 epilog=__doc__.split("\n\n", 1)[1])
    ap.add_argument("target", choices=TARGETS + ["all", "check"])
    ap.add_argument("--config", default=str(Path(__file__).resolve().parent / "build.ini"))
    a = ap.parse_args()
    cfg = host.Config(a.config)
    listed = cfg.path("paths", "targets")        # the targets the image was given the inputs of
    have = [t for t in TARGETS if t in listed.read_text().split()] if listed.is_file() else TARGETS

    if a.target == "check":
        say("Checking the tools and originals build.ini names, for %s ..." % " ".join(have))
        bad = toolchain.check(cfg, have)
        say("check: %s" % ("everything is in place" if not bad else "%d problem(s) above" % bad))
        return 1 if bad else 0

    if a.target != "all" and a.target not in have:
        raise BuildError("%s was left out when the image was built: its inputs were not in the folder "
                         "(README.md, What you supply); the image has %s" % (a.target, " ".join(have) or "none"))
    results = []
    for t in have if a.target == "all" else [a.target]:
        out = cfg.out(t)
        say("== %s -> %s" % (t, out))
        t0 = time.time()
        try:
            f = win.build(cfg, out, cefix=t == "ce")
            say("   %s, %.0f s" % (f, time.time() - t0))
        except BuildError as e:
            say("FAILED: %s" % e)
            f = None
        results.append((t, f))

    say("")
    say("%-8s %s" % ("target", "SHA-256 of the build"))
    for t, f in results:
        say("%-8s %s" % (t, host.sha256(f) if f else "FAILED"))
    return 0 if all(f for _t, f in results) else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BuildError as e:
        say("ERROR: %s" % e)
        sys.exit(2)
