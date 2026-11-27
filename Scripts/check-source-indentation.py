#!/usr/bin/env python3
"""Check or normalize four-column indentation in tracked project source."""

import argparse
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[1]
TAB_WIDTH = 4
LEADING_INDENT = re.compile(rb"(?m)^[ \t]+(?=[^ \t\r\n])")

# Consume comments and literals together so markers inside one cannot start another.
# Line breaks inside these tokens are content and must survive normalization.
COMMON_TOKENS = rb'/\*.*?\*/|"(?:\\.|[^"\\])*"|\x27(?:\\.|[^\x27\\\r\n])*\x27'
CPP_TOKENS = re.compile(
    rb'//(?:\\\r?\n|[^\r\n])*|R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=delimiter)"|'
    + COMMON_TOKENS,
    re.DOTALL,
)
CS_TOKENS = re.compile(
    rb'//[^\r\n]*|(?P<quotes>"{3,})(?!").*?(?P=quotes)|@"(?:""|[^"])*"|'
    + COMMON_TOKENS,
    re.DOTALL,
)


def indentation_edits(source, tokens):
    # Exclude the BOM from the scan while keeping offsets into the original bytes.
    start = 3 if source.startswith(b"\xef\xbb\xbf") else 0
    protected_tokens = tokens.finditer(source, start)
    token = next(protected_tokens, None)

    # The BOM occupies the first three bytes but no display columns.
    for indent in LEADING_INDENT.finditer(source[start:]):
        offset = start + indent.start()
        while token is not None and token.end() <= offset:
            token = next(protected_tokens, None)
        if token is not None and token.start() < offset < token.end():
            continue

        columns = len(indent.group().expandtabs(TAB_WIDTH))
        tabs, spaces = divmod(columns, TAB_WIDTH)
        replacement = b"\t" * tabs + b" " * spaces
        if indent.group() != replacement:
            yield offset, start + indent.end(), replacement


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fix", action="store_true", help="Apply indentation changes in place")
    args = parser.parse_args()

    paths = subprocess.check_output(
        ["git", "ls-files", "-z", "--", "Source"], cwd=ROOT
    )
    violations = 0
    files_checked = 0
    for name in paths.decode("utf-8").split("\0"):
        path = ROOT / name
        if path.suffix not in {".cpp", ".h", ".cs"} or not path.is_file():
            continue
        files_checked += 1
        source = path.read_bytes()
        tokens = CS_TOKENS if path.suffix == ".cs" else CPP_TOKENS
        edits = list(indentation_edits(source, tokens))
        if not edits:
            continue
        violations += len(edits)
        if args.fix:
            parts = []
            cursor = 0
            for start, end, replacement in edits:
                parts.extend((source[cursor:start], replacement))
                cursor = end
            parts.append(source[cursor:])
            path.write_bytes(b"".join(parts))
        else:
            for start, _, _ in edits:
                line = source.count(b"\n", 0, start) + 1
                print(f"{name}:{line}: use tabs for full four-column indentation steps")

    if args.fix:
        print(f"Normalized {violations} lines across {files_checked} checked source files.")
    elif violations:
        print("Run python3 Scripts/check-source-indentation.py --fix to normalize indentation.")
        return 1
    else:
        print(f"Indentation check passed for {files_checked} source files.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
