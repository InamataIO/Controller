#!/usr/bin/env python3
"""Small helper to build Improv RPC command frames from text input.

Frame format:
  [command][data_length][len(field1)][field1 bytes]...[checksum]

Checksum is the low 8 bits of the sum of all previous frame bytes.
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass


@dataclass(frozen=True)
class CommandSpec:
    name: str
    opcode: int
    field_count: int
    allow_omitted_trailing_empty_fields: bool = False


COMMANDS = [
    CommandSpec("WIFI_SETTINGS", 0x01, 2),
    CommandSpec("IDENTIFY", 0x02, 0),
    CommandSpec("GET_DEVICE_INFO", 0x03, 0),
    CommandSpec("GET_WIFI_NETWORKS", 0x04, 0),
    CommandSpec("X_SET_ALLOWED_MOBILE_OPERATORS", 0xF9, 1),
    CommandSpec("X_GET_MOBILE_OPERATORS", 0xFA, 1, True),
    CommandSpec("X_GET_MOBILE_STATE", 0xFB, 0),
    CommandSpec("X_SET_USER_DATA", 0xFC, 1),
    CommandSpec("X_GET_DEVICE_TYPE", 0xFD, 0),
    CommandSpec("X_SET_SERVER_AUTH", 0xFE, 2),
]

COMMANDS_BY_NAME = {cmd.name.upper(): cmd for cmd in COMMANDS}
COMMANDS_BY_OPCODE = {cmd.opcode: cmd for cmd in COMMANDS}


def build_improv_frame(spec: CommandSpec, field1: str = "", field2: str = "") -> bytes:
    fields: list[str]
    if spec.field_count == 0:
        fields = []
    elif spec.field_count == 1:
        fields = [field1]
    elif spec.field_count == 2:
        fields = [field1, field2]
    else:
        raise ValueError(f"Unsupported field_count: {spec.field_count}")

    if spec.allow_omitted_trailing_empty_fields:
        while fields and fields[-1] == "":
            fields.pop()

    payload = bytearray()
    for value in fields:
        encoded = value.encode("utf-8")
        if len(encoded) > 255:
            raise ValueError("Each field must be at most 255 bytes (UTF-8 encoded).")
        payload.append(len(encoded))
        payload.extend(encoded)

    frame = bytearray()
    frame.append(spec.opcode)
    frame.append(len(payload))
    frame.extend(payload)
    frame.append(sum(frame) & 0xFF)
    return bytes(frame)


def choose_command() -> CommandSpec:
    print("Available commands:")
    for i, cmd in enumerate(COMMANDS, start=1):
        print(f"  {i:2d}. {cmd.name} (0x{cmd.opcode:02X}, fields: {cmd.field_count})")

    while True:
        raw = input("\nSelect command number: ").strip()
        try:
            idx = int(raw)
            if 1 <= idx <= len(COMMANDS):
                return COMMANDS[idx - 1]
        except ValueError:
            pass
        print("Invalid selection. Enter one of the listed numbers.")


def parse_command_selector(selector: str) -> CommandSpec:
    selector = selector.strip()

    by_name = COMMANDS_BY_NAME.get(selector.upper())
    if by_name is not None:
        return by_name

    try:
        opcode = int(selector, 0)
    except ValueError as exc:
        raise ValueError(
            "Command must be a known name (e.g. X_SET_USER_DATA) or opcode (e.g. 0xFC)."
        ) from exc

    by_opcode = COMMANDS_BY_OPCODE.get(opcode)
    if by_opcode is None:
        raise ValueError(
            f"Unknown opcode: 0x{opcode:02X}. Use one of the listed commands."
        )
    return by_opcode


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Build Improv RPC command frames from command + text fields. "
            "Without --command the script runs in interactive mode."
        )
    )
    parser.add_argument(
        "-c",
        "--command",
        help=(
            "Command name or opcode (e.g. WIFI_SETTINGS, X_SET_USER_DATA, 0x01, 0xFC)."
        ),
    )
    parser.add_argument(
        "--field1",
        "-1",
        default="",
        help="Input field 1 (mapped to first length-prefixed payload item).",
    )
    parser.add_argument(
        "--field2",
        "-2",
        default="",
        help="Input field 2 (mapped to second length-prefixed payload item).",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="Print available commands and exit.",
    )
    return parser


def print_available_commands() -> None:
    print("Available commands:")
    for i, cmd in enumerate(COMMANDS, start=1):
        print(f"  {i:2d}. {cmd.name} (0x{cmd.opcode:02X}, fields: {cmd.field_count})")


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    if args.list:
        print_available_commands()
        return

    if args.command:
        try:
            spec = parse_command_selector(args.command)
        except ValueError as exc:
            parser.error(str(exc))

        field1 = args.field1
        field2 = args.field2
    else:
        spec = choose_command()
        field1 = ""
        field2 = ""
        if spec.field_count >= 1:
            field1 = input("Input field 1: ")
        if spec.field_count >= 2:
            field2 = input("Input field 2: ")

    frame = build_improv_frame(spec, field1, field2)

    spaced_hex = " ".join(f"{b:02X}" for b in frame)
    compact_hex = "".join(f"{b:02X}" for b in frame)
    c_escape = "".join(f"\\x{b:02X}" for b in frame)

    print("\nCommand:", spec.name)
    print("Opcode :", f"0x{spec.opcode:02X}")
    print("Hex    :", spaced_hex)
    print("Compact:", compact_hex)
    print("C-bytes:", c_escape)


if __name__ == "__main__":
    try:
        main()
    except ValueError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(2) from exc
