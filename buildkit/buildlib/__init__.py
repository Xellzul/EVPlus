"""The EV Nova build kit's build path: buildkit/build.py is the entry point.

Modules, in the order a build uses them:
  host       build.ini, paths, running a Windows exe under Wine, hashes
  toolchain  the expected size, CRC32 and SHA-256 of every tool, and `build.py check`
  coff, pe   the small object, archive and PE edits the Windows link needs
  win        EV Nova.exe 1.0.10 and the CE build: compile, archives, resources, link, post-steps
"""
