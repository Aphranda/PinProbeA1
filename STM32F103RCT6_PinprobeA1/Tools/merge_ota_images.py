#!/usr/bin/env python3
"""
Merge PinProbe A1 bootloader and application Intel HEX images.

The output HEX is intended for factory flashing:
  - bootloader at 0x08000000
  - application at  0x08006000
"""

from __future__ import annotations

import argparse
import json
import zlib
from pathlib import Path


FLASH_BASE = 0x08000000
BOOT_BASE = 0x08000000
APP_BASE = 0x08006000
APP_CONFIG_BASE = 0x0803F800
FLASH_END = 0x08040000


class HexError(RuntimeError):
    pass


def parse_record(line: str, line_no: int) -> tuple[int, int, int, bytes]:
    line = line.strip()
    if not line:
        raise HexError(f"line {line_no}: empty line")
    if not line.startswith(":"):
        raise HexError(f"line {line_no}: missing ':'")

    try:
        raw = bytes.fromhex(line[1:])
    except ValueError as exc:
        raise HexError(f"line {line_no}: invalid hex") from exc

    if len(raw) < 5:
        raise HexError(f"line {line_no}: record too short")
    count = raw[0]
    if len(raw) != count + 5:
        raise HexError(f"line {line_no}: length mismatch")
    if (sum(raw) & 0xFF) != 0:
        raise HexError(f"line {line_no}: checksum mismatch")

    address = (raw[1] << 8) | raw[2]
    rectype = raw[3]
    data = raw[4:4 + count]
    return count, address, rectype, data


def read_ihex(path: Path) -> dict[int, int]:
    image: dict[int, int] = {}
    upper = 0

    for line_no, line in enumerate(path.read_text(encoding="ascii").splitlines(), start=1):
        count, address, rectype, data = parse_record(line, line_no)

        if rectype == 0x00:
            full = upper + address
            for offset, value in enumerate(data):
                absolute = full + offset
                old = image.get(absolute)
                if old is not None and old != value:
                    raise HexError(f"{path}: conflicting byte at 0x{absolute:08X}")
                image[absolute] = value
        elif rectype == 0x01:
            break
        elif rectype == 0x02:
            if count != 2:
                raise HexError(f"{path}: invalid extended segment address")
            upper = (((data[0] << 8) | data[1]) << 4)
        elif rectype == 0x04:
            if count != 2:
                raise HexError(f"{path}: invalid extended linear address")
            upper = (((data[0] << 8) | data[1]) << 16)
        elif rectype in (0x03, 0x05):
            continue
        else:
            raise HexError(f"{path}: unsupported record type 0x{rectype:02X}")

    return image


def make_record(address: int, rectype: int, data: bytes) -> str:
    count = len(data)
    body = bytes([count, (address >> 8) & 0xFF, address & 0xFF, rectype]) + data
    checksum = (-sum(body)) & 0xFF
    return ":" + (body + bytes([checksum])).hex().upper()


def write_ihex(path: Path, image: dict[int, int]) -> None:
    lines: list[str] = []
    current_upper: int | None = None

    addresses = sorted(image)
    index = 0
    while index < len(addresses):
        start = addresses[index]
        upper = start >> 16
        if current_upper != upper:
            current_upper = upper
            lines.append(make_record(0, 0x04, upper.to_bytes(2, "big")))

        chunk = bytearray()
        low = start & 0xFFFF
        while (
            index < len(addresses)
            and len(chunk) < 16
            and addresses[index] == start + len(chunk)
            and (addresses[index] >> 16) == upper
        ):
            chunk.append(image[addresses[index]])
            index += 1

        lines.append(make_record(low, 0x00, bytes(chunk)))

    lines.append(make_record(0, 0x01, b""))
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def merge_images(boot: dict[int, int], app: dict[int, int]) -> dict[int, int]:
    validate_range("bootloader", boot, BOOT_BASE, APP_BASE)
    validate_range("application", app, APP_BASE, APP_CONFIG_BASE)

    merged = dict(boot)
    for address, value in app.items():
        old = merged.get(address)
        if old is not None and old != value:
            raise HexError(f"image overlap at 0x{address:08X}")
        merged[address] = value
    return merged


def validate_range(name: str, image: dict[int, int], start: int, end: int) -> None:
    if not image:
        raise HexError(f"{name}: empty image")
    low = min(image)
    high = max(image) + 1
    if low < start or high > end:
        raise HexError(
            f"{name}: address range 0x{low:08X}-0x{high - 1:08X} outside "
            f"0x{start:08X}-0x{end - 1:08X}"
        )
    if high > FLASH_END:
        raise HexError(f"{name}: exceeds device flash")


def image_to_bin(image: dict[int, int], base: int, end: int) -> bytes:
    data = bytearray([0xFF] * (end - base))
    for address, value in image.items():
        data[address - base] = value
    return bytes(data)


def image_summary(name: str, image: dict[int, int]) -> dict[str, object]:
    low = min(image)
    high = max(image) + 1
    payload = bytes(image[address] for address in sorted(image))
    return {
        "name": name,
        "start": f"0x{low:08X}",
        "end_exclusive": f"0x{high:08X}",
        "sparse_bytes": len(image),
        "crc32_sparse_ordered": f"0x{zlib.crc32(payload) & 0xFFFFFFFF:08X}",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Merge PinProbe A1 OTA images")
    parser.add_argument("--boot", type=Path,
                        default=Path("build/PinProbeA1_Bootloader/PinProbeA1_Bootloader.hex"))
    parser.add_argument("--app", type=Path,
                        default=Path("build/STM32F103RCT6_PinprobeA1/STM32F103RCT6_PinprobeA1.hex"))
    parser.add_argument("--out-dir", type=Path, default=Path("build/PinProbeA1_Factory"))
    parser.add_argument("--name", default="PinProbeA1_Factory")
    args = parser.parse_args()

    boot = read_ihex(args.boot)
    app = read_ihex(args.app)
    merged = merge_images(boot, app)

    args.out_dir.mkdir(parents=True, exist_ok=True)
    hex_path = args.out_dir / f"{args.name}.hex"
    bin_path = args.out_dir / f"{args.name}.bin"
    json_path = args.out_dir / f"{args.name}.json"

    write_ihex(hex_path, merged)
    bin_end = max(merged) + 1
    bin_path.write_bytes(image_to_bin(merged, FLASH_BASE, bin_end))

    manifest = {
        "flash_base": f"0x{FLASH_BASE:08X}",
        "binary_base": f"0x{FLASH_BASE:08X}",
        "hex": str(hex_path),
        "bin": str(bin_path),
        "bootloader": image_summary("bootloader", boot),
        "application": image_summary("application", app),
        "merged": image_summary("factory", merged),
    }
    json_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    print(f"merged hex: {hex_path}")
    print(f"merged bin: {bin_path} @ 0x{FLASH_BASE:08X}")
    print(f"manifest:   {json_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
