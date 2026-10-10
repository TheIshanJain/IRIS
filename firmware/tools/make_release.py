#!/usr/bin/env python3
"""
make_release.py - prepare an OTA release for CloudOTA.

1. In Arduino IDE: bump FW_VERSION in Config.h, then Sketch > Export Compiled Binary.
2. Run:
     python make_release.py --bin path/to/ESP32S3_Cute_Eyes.ino.bin --version 1.0.1 --repo YOUR_USER/YOUR_REPO
3. Upload the two files from ./release/ to a GitHub release tagged v1.0.1
   (the script prints the exact `gh` command).

Use the plain  *.ino.bin  file, NOT *.merged.bin / *.bootloader.bin / *.partitions.bin.
"""
import argparse
import hashlib
import json
import os
import shutil
import sys


def main():
    p = argparse.ArgumentParser(description="Build a CloudOTA release (firmware.bin + manifest.json)")
    p.add_argument("--bin", required=True, help="Exported app binary (*.ino.bin)")
    p.add_argument("--version", required=True, help="Must equal FW_VERSION in Config.h, e.g. 1.0.1")
    p.add_argument("--repo", help="GitHub repo USER/REPO (builds the download URL for you)")
    p.add_argument("--url", help="Custom firmware URL if you host it elsewhere")
    p.add_argument("--device", default="cute-eyes-s3", help="Must equal OTA_DEVICE_TYPE in Config.h")
    p.add_argument("--notes", default="", help="Release notes")
    p.add_argument("--out", default="release", help="Output folder")
    a = p.parse_args()

    name = os.path.basename(a.bin)
    if any(x in name for x in (".merged.", ".bootloader.", ".partitions.")):
        sys.exit(f"'{name}' is not the app image. Use the plain *.ino.bin file.")
    if not os.path.isfile(a.bin):
        sys.exit(f"File not found: {a.bin}")

    version = a.version.lstrip("vV")
    tag = f"v{version}"
    if a.url:
        url = a.url
    elif a.repo:
        url = f"https://github.com/{a.repo}/releases/download/{tag}/firmware.bin"
    else:
        sys.exit("Give --repo USER/REPO or --url")

    os.makedirs(a.out, exist_ok=True)
    fw_path = os.path.join(a.out, "firmware.bin")
    shutil.copyfile(a.bin, fw_path)

    with open(fw_path, "rb") as f:
        data = f.read()
    md5 = hashlib.md5(data).hexdigest()

    manifest = {
        "device": a.device,
        "version": version,
        "url": url,
        "md5": md5,
        "notes": a.notes,
    }
    with open(os.path.join(a.out, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=2)

    print(f"Firmware : {fw_path}  ({len(data) / 1024:.0f} KB)")
    print(f"MD5      : {md5}")
    print(f"Manifest : {os.path.join(a.out, 'manifest.json')}")
    print(json.dumps(manifest, indent=2))
    if a.repo:
        print("\nPublish with the GitHub CLI:")
        print(f'  gh release create {tag} {a.out}/firmware.bin {a.out}/manifest.json '
              f'--repo {a.repo} --title "{tag}" --notes "{a.notes or tag}"')
        print("\nOr upload both files manually at:")
        print(f"  https://github.com/{a.repo}/releases/new  (tag: {tag})")


if __name__ == "__main__":
    main()
