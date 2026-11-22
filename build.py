#!/usr/bin/env python3
"""Cross-compile the Windows x64 DLL and pack a CPS ZIP without cps_pack.exe."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parent

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--zig", default=shutil.which("zig"), help="Zig 0.15.2 executable")
    args = parser.parse_args()
    if not args.zig:
        parser.error("Install official Zig 0.15.2 or pass --zig /path/to/zig")
    zig = str(Path(args.zig).resolve())
    env = os.environ.copy()
    env.setdefault("ZIG_GLOBAL_CACHE_DIR", str(ROOT / ".cache" / "zig-global"))
    env.setdefault("ZIG_LOCAL_CACHE_DIR", str(ROOT / ".cache" / "zig-local"))
    raw = subprocess.check_output([zig, "env"], text=True, env=env)
    try:
        lib_dir = json.loads(raw)["lib_dir"]
    except (ValueError, KeyError):
        match = re.search(r'\.lib_dir\s*=\s*"([^"\n]+)"', raw)
        if not match:
            raise SystemExit("Could not locate Zig's libc headers")
        lib_dir = match.group(1)
    lib = Path(lib_dir).resolve()
    dist = ROOT / "dist"
    dist.mkdir(exist_ok=True)
    dll = dist / "headless_vitcoin.dll"
    command = [zig, "cc", "-target", "x86_64-windows-gnu", "-O2", "-fno-builtin",
               "-shared", "-nostdlib", "-Wl,--entry,DllMain",
               "-I" + str(lib / "libc" / "include" / "any-windows-any"),
               "-I" + str(ROOT / "include"), str(ROOT / "src" / "plugin.c"),
               "-lkernel32", "-o", str(dll)]
    subprocess.run(command, check=True, env=env)
    package = dist / "headless_vitcoin.cps"
    with zipfile.ZipFile(package, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.write(dll, dll.name)
        for name in ("plugin.toml", "LICENSE", "LICENSE.CPS-SDK"):
            archive.write(ROOT / name, name)
    (dist / "SHA256SUMS").write_text("".join(
        f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n" for p in (dll, package)))
    print(package)

if __name__ == "__main__":
    main()
