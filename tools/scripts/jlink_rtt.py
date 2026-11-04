#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Read RTT through the J-Link SDK without resetting or flashing the target."""

import argparse
import re
import sys
import time
from pathlib import Path

import pylink


def status(message):
    if isinstance(message, bytes):
        message = message.decode(errors="replace")
    print(str(message).rstrip(), file=sys.stderr, flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ip", required=True)
    parser.add_argument("--device", required=True)
    parser.add_argument("--interface", choices=("SWD", "JTAG"), default="SWD")
    parser.add_argument("--speed", type=int, default=1000)
    parser.add_argument("--address", type=lambda value: int(value, 0))
    parser.add_argument("--map", type=Path)
    parser.add_argument("--search-ranges", default="0x20000000 0x10000")
    parser.add_argument("--library", default="/opt/segger/jlink/libjlinkarm.so")
    args = parser.parse_args()

    address = args.address
    if address is None and args.map and args.map.is_file():
        match = re.search(r"^\s*(0x[0-9a-fA-F]+)\s+_SEGGER_RTT\s*$",
                          args.map.read_text(), re.MULTILINE)
        if match:
            address = int(match.group(1), 16)
            status(f"RTT address from {args.map}: {address:#010x}; "
                   "the board must contain the matching firmware")

    link = pylink.JLink(lib=pylink.library.Library(dllpath=args.library),
                       log=status, error=status, warn=status)
    started = False
    try:
        status(f"J-Link library: {args.library}")
        status(f"Remote: {args.ip}; target: {args.device}; "
               f"{args.interface} {args.speed} kHz")
        link.open(ip_addr=args.ip)
        link.set_tif(getattr(pylink.enums.JLinkInterfaces, args.interface))
        link.connect(args.device, speed=args.speed, verbose=True)
        if address is None:
            link.exec_command(f"SetRTTSearchRanges {args.search_ranges}")
        link.rtt_start(block_address=address)
        started = True

        # RTT discovery is asynchronous. Allow initialization to finish rather
        # than exiting immediately when the control block is not ready yet.
        deadline = time.monotonic() + 10
        while True:
            try:
                if link.rtt_get_num_up_buffers() > 0:
                    break
            except pylink.errors.JLinkRTTException:
                pass
            if time.monotonic() >= deadline:
                if address is not None:
                    header = bytes(link.memory_read8(address, 16))
                    status(f"Control block header at {address:#010x}: {header!r}")
                raise RuntimeError("No RTT up-channel found within 10 seconds")
            time.sleep(0.05)

        status("Reading RTT up-channel 0; press Ctrl+C to stop.")
        while True:
            data = link.rtt_read(0, 4096)
            if data:
                sys.stdout.buffer.write(bytes(data))
                sys.stdout.buffer.flush()
            else:
                time.sleep(0.01)
    finally:
        try:
            if started:
                link.rtt_stop()
        finally:
            link.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
    except (pylink.errors.JLinkException, OSError, RuntimeError) as error:
        status(f"RTT error: {error}")
        sys.exit(1)
