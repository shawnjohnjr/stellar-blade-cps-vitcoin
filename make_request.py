#!/usr/bin/env python3
"""Create an explicit request in the user's chosen CPS config directory."""
import argparse
from pathlib import Path
import re
import uuid

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("config_dir", type=Path, help="CPS/Configs/HeadlessVitcoin directory")
    parser.add_argument("target", type=int, help="desired total, not amount to add")
    parser.add_argument("--id", default=None, help="optional new request ID, max 32 letters/digits/_/-")
    args = parser.parse_args()
    if not 0 <= args.target <= 999999:
        parser.error("target must be between 0 and 999999")
    request_id = args.id or uuid.uuid4().hex
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,32}", request_id):
        parser.error("invalid request ID")
    if not args.config_dir.is_dir():
        parser.error("config directory must already exist; start the installed plugin once first")
    if (args.config_dir / ("attempt-" + request_id + ".txt")).exists():
        parser.error("that request ID is already consumed; inspect its result before creating another request")
    temporary = args.config_dir / (".request-" + uuid.uuid4().hex + ".tmp")
    temporary.write_text(f"id={request_id}\ntarget={args.target}\n", encoding="utf-8")
    temporary.replace(args.config_dir / "request.ini")
    print(f"Requested total {args.target}; request ID {request_id}. Check status.txt for the result.")

if __name__ == "__main__":
    main()
