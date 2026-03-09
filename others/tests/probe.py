#!/usr/bin/env python3
"""
probe — look up candidates for one or more typed inputs.

Usage:
    ./probe.py [-s SCHEMA] [-d SHARED_DIR] [-u USER_DIR] INPUT [INPUT ...]

Options:
    -s SCHEMA      schema to use (default: icemix_flypy)
    -d SHARED_DIR  system rime data dir (default: /usr/share/rime-data)
    -u USER_DIR    user rime data dir   (default: $HOME/.config/ibus/rime)

Examples:
    ./probe.py xl vh bwjk
    ./probe.py -s icemix_zrm xd bzjy
    ./probe.py -s icemix_abc asgo bqjy
    ./probe.py -u /tmp/rime_icemix_test xl vh
"""

import sys
from pyrime import rime

import pyrime_ext

USAGE = "Usage: probe.py [-s SCHEMA] [-d SHARED_DIR] [-u USER_DIR] INPUT [INPUT ...]"
MAX_CANDS = 9


def main():
    schema = "icemix_flypy"
    shared_dir = "/usr/share/rime-data"
    user_dir = None

    args = sys.argv[1:]
    i = 0
    while i < len(args) and args[i].startswith("-"):
        flag = args[i]
        i += 1
        if i >= len(args):
            print(f"probe: {flag} requires an argument", file=sys.stderr)
            sys.exit(1)
        if flag == "-s":
            schema = args[i]
        elif flag == "-d":
            shared_dir = args[i]
        elif flag == "-u":
            user_dir = args[i]
        else:
            print(f"probe: unknown option {flag}\n{USAGE}", file=sys.stderr)
            sys.exit(1)
        i += 1

    inputs = args[i:]
    if not inputs:
        print(USAGE, file=sys.stderr)
        sys.exit(1)

    pyrime_ext.init(shared_data_dir=shared_dir, user_data_dir=user_dir)

    sid = rime.create_session()
    if not rime.select_schema(sid, schema):
        print(f"probe: cannot load schema '{schema}'", file=sys.stderr)
        rime.destroy_session(sid)
        sys.exit(1)
    rime.set_option(sid, "ascii_mode", False)

    for inp in inputs:
        pyrime_ext.type_input(sid, inp)
        ctx = rime.get_context(sid)
        cands = ctx.menu.candidates[:MAX_CANDS]
        extra = ctx.menu.num_candidates - MAX_CANDS
        line = f"{inp:<12} →  " + "  ".join(c.text for c in cands)
        if extra > 0:
            line += f"  …({extra} more)"
        print(line)

    rime.destroy_session(sid)
    sys.exit(0)


if __name__ == "__main__":
    main()
