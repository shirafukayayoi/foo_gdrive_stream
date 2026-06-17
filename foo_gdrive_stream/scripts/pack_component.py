#!/usr/bin/env python3
import argparse
import pathlib
import sys
import zipfile

ROOT = pathlib.Path(__file__).resolve().parent.parent

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", default="x64")
    parser.add_argument("--configuration", default="Release")
    args = parser.parse_args()

    dll = ROOT / "_result" / f"{args.platform}_{args.configuration}" / "bin" / "foo_gdrive_stream.dll"
    if not dll.exists():
        print(f"ERROR: DLL not found: {dll}", file=sys.stderr)
        return 1
    out_dir = ROOT / "_result"
    out_dir.mkdir(exist_ok=True)
    out = out_dir / "foo_gdrive_stream.fb2k-component"
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.write(dll, "foo_gdrive_stream.dll")
    print(f"Packaged: {out} ({out.stat().st_size:,} bytes)")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
