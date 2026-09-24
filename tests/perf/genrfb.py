#!/usr/bin/env python3
"""Generate deterministic raw-RFB traces for encperf.

These traces are synthetic microbenchmarks, not substitutes for captured
desktop workloads. The file starts at the point expected by encperf: after
the RFB handshake and ServerInit message.

Example:
  python3 tests/perf/genrfb.py ui /tmp/ui.rfb --width 640 --height 360
  tests/perf/encperf encoding=7 width=640 height=360 format=rgb888 /tmp/ui.rfb

Run the same trace with encoding=5, 16, or 21 to compare Hextile, ZRLE, or
JPEG. encperf reports CPU time and encoded bytes. These numbers do not include
network latency or client/server scheduling.
"""

import argparse
import struct


def pixel(red, green, blue):
    # rgb888 parsed by TigerVNC is little-endian 32bpp, with an unused byte.
    return bytes((blue, green, red, 0))


def rect_header(x, y, width, height):
    return struct.pack(">HHHHi", x, y, width, height, 0)  # Raw encoding


def update(out, x, y, width, height, data):
    out.write(struct.pack(">BBH", 0, 0, 1))  # FramebufferUpdate, one rect
    out.write(rect_header(x, y, width, height))
    out.write(data)


def desktop_frame(width, height):
    rows = []
    for y in range(height):
        rows.append(b"".join(pixel(25 + x * 30 // width,
                                    40 + y * 30 // height, 72)
                              for x in range(width)))
    return b"".join(rows)


def ui_trace(out, width, height, frames):
    update(out, 0, 0, width, height, desktop_frame(width, height))
    box_w, box_h = min(320, width), min(80, height)
    for frame in range(frames - 1):
        x = (frame * 37) % max(1, width - box_w + 1)
        y = (frame * 19) % max(1, height - box_h + 1)
        data = bytearray()
        for py in range(box_h):
            for px in range(box_w):
                # Repeating glyph-like edges and a changing cursor-sized block.
                ink = ((px % 17) < 2 and (py % 23) < 15)
                cursor = (x + px - frame * 11) % max(1, width) < 12 and py < 16
                data += pixel(225 if ink else 45,
                              235 if cursor else (220 if ink else 90),
                              245 if cursor else (180 if ink else 130))
        update(out, x, y, box_w, box_h, data)


def motion_trace(out, width, height, frames):
    for frame in range(frames):
        rows = []
        for y in range(height):
            row = bytearray()
            for x in range(width):
                band = ((x + frame * 13) // 48) % 6
                red = (band * 41 + (y // 8)) & 255
                green = (y * 255 // max(1, height - 1) + frame * 3) & 255
                blue = ((x // 8) * 7 + frame * 5) & 255
                row += pixel(red, green, blue)
            rows.append(row)
        update(out, 0, 0, width, height, b"".join(rows))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("workload", choices=("ui", "motion"))
    parser.add_argument("output")
    parser.add_argument("--width", type=int, default=1280)
    parser.add_argument("--height", type=int, default=720)
    parser.add_argument("--frames", type=int, default=30)
    args = parser.parse_args()
    if not (1 <= args.width <= 65535 and 1 <= args.height <= 65535):
        parser.error("width and height must be in 1..65535")
    if args.frames < 1:
        parser.error("frames must be positive")

    with open(args.output, "wb") as out:
        (ui_trace if args.workload == "ui" else motion_trace)(
            out, args.width, args.height, args.frames)


if __name__ == "__main__":
    main()
