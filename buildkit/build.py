#!/usr/bin/env python3
"""Build EV Nova from this repository's sources.

    python buildkit/build.py check              verify every tool and original build.ini names
    python buildkit/build.py win                EV Nova.exe 1.0.10 for Windows
    python buildkit/build.py ce                 the EV Nova CE build of the same sources (-DCEFIX)
    python buildkit/build.py all                both (those the image has the inputs of)

Options:
    --config FILE    another build.ini (default: buildkit/build.ini)
    --no-pin-date    keep the link time in the headers instead of the original's date
    --unsigned       leave the code signature off (win)
    --twice          build each target twice and compare (the 2.4.7 b335 compiler is not always
                     deterministic; a second build tells a flaky compile from a real difference)

Every output goes to buildkit/out/<target>/. At the end each one's SHA-256 is printed.
"""
import argparse
import shutil
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from buildlib import host, toolchain  # noqa: E402
from buildlib.host import BuildError, say  # noqa: E402

TARGETS = ["win", "ce"]


def build_one(cfg, target, out, a):
    """Build TARGET into OUT; returns the output file."""
    from buildlib import win
    return win.build(cfg, out, cefix=target == "ce", pin_date=not a.no_pin_date, signed=not a.unsigned)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0], formatter_class=argparse.RawDescriptionHelpFormatter,
                                 epilog=__doc__.split("\n\n", 1)[1])
    ap.add_argument("target", choices=TARGETS + ["all", "check"])
    ap.add_argument("--config", default=str(Path(__file__).resolve().parent / "build.ini"))
    ap.add_argument("--no-pin-date", action="store_true")
    ap.add_argument("--unsigned", action="store_true")
    ap.add_argument("--twice", action="store_true")
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
    targets = have if a.target == "all" else [a.target]
    results = []
    for t in targets:
        runs = 2 if a.twice else 1
        shas = []
        for n in range(runs):
            out = cfg.out(t if n == 0 else t + "-twice")
            say("== %s%s -> %s" % (t, " (second build)" if n else "", out))
            t0 = time.time()
            try:
                f = build_one(cfg, t, out, a)
            except BuildError as e:
                say("FAILED: %s" % e)
                results.append((t, None, "FAILED"))
                break
            shas.append((f, host.sha256(f)))
            say("   %s, %.0f s" % (f, time.time() - t0))
        else:
            f, sha = shas[0]
            twice = "" if runs == 1 else "twice: same" if shas[0][1] == shas[1][1] else "twice: THE TWO BUILDS DIFFER"
            results.append((t, f, twice))
            if a.twice and shas[0][1] == shas[1][1]:
                shutil.rmtree(cfg.out(t + "-twice"), ignore_errors=True)

    say("")
    say("%-8s %s" % ("target", "SHA-256 of the build"))
    for t, f, twice in results:
        say("%-8s %s %s" % (t, host.sha256(f) if f else "FAILED", twice))
    return 0 if all(r[1] for r in results) else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BuildError as e:
        say("ERROR: %s" % e)
        sys.exit(2)
