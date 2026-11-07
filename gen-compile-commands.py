#!/usr/bin/env python3
"""
Generate compile_commands.json for xv6-labs so that clangd understands the
build (RISC-V target, freestanding, -DLAB_*, -I. ...) instead of falling back
to host/x86-64 defaults and reporting bogus errors.

Usage:  ./gen-compile-commands.py
        (re-run after switching labs, i.e. when conf/lab.mk changes)
"""

import json
import os
import shlex
import subprocess
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
SOURCE_DIRS = ("kernel", "user", "mkfs")
SRC_EXTS = (".c", ".S")
HDR_EXTS = (".h",)

# xv6 headers are not self-contained (no include guards, and they assume
# kernel/types.h was included first).  Give each header a matching entry whose
# command force-includes this preamble so clangd can parse it standalone.
HEADER_PREAMBLE = (
    "kernel/types.h",
    "kernel/param.h",
    "kernel/memlayout.h",
    "kernel/riscv.h",
    "kernel/spinlock.h",
    "kernel/sleeplock.h",
    "kernel/fs.h",
    "kernel/file.h",
    "kernel/proc.h",
)

# Flags to strip from the gcc command line before handing it to clangd.
DROP_FLAGS = {"-MD", "-MMD", "-MP", "-c", "-o"}
# Flags whose following argument should be dropped together with them.
DROP_WITH_ARG = {"-o"}


def gather_sources():
    out = []
    for d in SOURCE_DIRS:
        dd = os.path.join(ROOT, d)
        if os.path.isdir(dd):
            for name in sorted(os.listdir(dd)):
                if name.endswith(SRC_EXTS):
                    out.append(f"{d}/{name}")
    return out


def gather_headers():
    out = []
    dd = os.path.join(ROOT, "kernel")
    if os.path.isdir(dd):
        for name in sorted(os.listdir(dd)):
            if name.endswith(HDR_EXTS):
                out.append(f"kernel/{name}")
    return out


# mkfs/mkfs.c is a host program built by its own explicit rule; building a
# bogus mkfs/mkfs.o would instead match the freestanding kernel rule.
HOST_GOALS = {"mkfs/mkfs.c": "mkfs/mkfs"}


def make_command(src):
    """Ask make for the exact compile command that would build `src`."""
    host = src in HOST_GOALS
    goal = HOST_GOALS[src] if host else os.path.splitext(src)[0] + ".o"

    try:  # make the source look newer than the object so the rule prints
        os.utime(os.path.join(ROOT, src), None)
    except OSError:
        pass

    proc = subprocess.run(["make", "-n", goal], cwd=ROOT,
                          capture_output=True, text=True)
    if proc.returncode != 0:
        return None
    for line in proc.stdout.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or src not in line:
            continue
        if host:
            # The mkfs rule compiles and links in one go; take it as-is.
            return line
        # We only want the compile step (has -c and names this source).
        if " -c " not in f" {line} ":
            continue
        return line
    return None


def clean_args(cmd, src):
    args = shlex.split(cmd)
    if not args:
        return None
    compiler, rest = args[0], args[1:]

    out, skip = [], False
    for a in rest:
        if skip:
            skip = False
            continue
        if a in DROP_WITH_ARG:
            skip = True
            continue
        if a in DROP_FLAGS:
            continue
        if a == src:            # source is supplied via "file" instead
            continue
        # Drop the output path of a combined compile+link step (mkfs/mkfs).
        if a.startswith("-o"):
            skip = True
            continue
        out.append(a)
    return [compiler] + out


def main():
    entries, failures = [], []
    # A representative command per directory, reused for header entries.
    base_by_dir = {}

    for src in gather_sources():
        cmd = make_command(src)
        if cmd is None:
            failures.append(src)
            continue
        args = clean_args(cmd, src)
        if not args:
            failures.append(src)
            continue
        entries.append({"directory": ROOT, "file": src, "arguments": args})
        base_by_dir.setdefault(os.path.dirname(src), args)

    # Header entries: reuse the flags of a TU in the same directory and
    # force-include the xv6 preamble so the header parses on its own.
    kernel_args = base_by_dir.get("kernel")
    if kernel_args:
        for hdr in gather_headers():
            preamble = []
            for h in HEADER_PREAMBLE:
                if h == hdr:  # don't force-include the header being checked
                    break
                preamble += ["-include", os.path.join(ROOT, h)]
            args = [kernel_args[0]] + preamble + kernel_args[1:]
            entries.append({"directory": ROOT, "file": hdr, "arguments": args})

    if failures:
        print("warning: no compile command for:", file=sys.stderr)
        for f in failures:
            print("   ", f, file=sys.stderr)

    with open(os.path.join(ROOT, "compile_commands.json"), "w") as fh:
        json.dump(entries, fh, indent=2)
        fh.write("\n")
    print(f"wrote compile_commands.json ({len(entries)} entries)")


if __name__ == "__main__":
    main()
