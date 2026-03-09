#!/usr/bin/env python3
"""
End-to-end test for icemix schemas.

Tests that typed double-pinyin inputs produce the expected Chinese character
candidates, exercising the full speller algebra → syllabification → translation
pipeline via pyrime.

Usage:
    ./e2e_test.py
    ./e2e_test.py -s flypy              # run only tests whose schema contains "flypy"
    ./e2e_test.py -d /usr/share/rime-data -u /tmp/rime_icemix_test

Options:
    -s FILTER      only run tests whose schema name contains FILTER
    -d SHARED_DIR  system rime data dir (default: /usr/share/rime-data)
    -u USER_DIR    user rime data dir   (default: $HOME/.config/ibus/rime)
"""

import sys
from pyrime import rime

import pyrime_ext

# ── algebra key mappings (comment reference) ──────────────────────────────────
#
# icemix_flypy (小鹤双拼):
#   Initials: zh→v  ch→i  sh→u
#   Finals:   iu→q  ei→w  uan→r  ue/ve→t  un→y  uo→o  ie→p
#             ong/iong→s  ing/uai→k  ai→d  en→f  eng→g
#             iang/uang→l  ang→h  ian→m  an→j  ou→z
#             ia/ua→x  iao→n  ao→c  ui→v  in→b
#
# icemix_zrm (自然码):
#   Initials: zh→v  ch→i  sh→u    (same as flypy)
#   Finals:   iu→q  ia/ua→w  uan→r  ue/ve→t  ing/uai→y  uo→o  un→p
#             ong/iong→s  iang/uang→d  en→f  eng→g  ang→h
#             ian→m  an→j  iao→c  ao→k  ai→l  ei→z  ie→x  ui→v  ou→b  in→n
#
# icemix_abc (智能ABC双拼):
#   Initials: zh→a  ch→e  sh→v
#   Finals:   ei→q  ian→w  er/iu→r  iang/uang→t  ing→y  uo→o  uan→p
#             ong/iong→s  ia/ua→d  en→f  eng→g  ang→h  an→j  iao→z  ao→k
#             in/uai→c  ai→l  ie→x  ou→b  un→n  ue/ui/ve→m
#
# ─────────────────────────────────────────────────────────────────────────────

TESTS = [
    # ════════════════════════════════════════════════════════════════════════
    # icemix_flypy  小鹤双拼  zh→v  ch→i  sh→u
    # ════════════════════════════════════════════════════════════════════════
    # initials
    ("icemix_flypy", "vh", "张", 5),  # zhang: zh(v) + ang(h)
    ("icemix_flypy", "ih", "长", 5),  # chang: ch(i) + ang(h)
    ("icemix_flypy", "uh", "上", 5),  # shang: sh(u) + ang(h)
    # finals
    ("icemix_flypy", "xl", "想", 5),  # xiang: x + iang(l)
    ("icemix_flypy", "xs", "熊", 5),  # xiong: x + iong(s)
    ("icemix_flypy", "bm", "边", 5),  # bian:  b + ian(m)
    ("icemix_flypy", "dk", "定", 5),  # ding:  d + ing(k)
    ("icemix_flypy", "fh", "方", 5),  # fang:  f + ang(h)
    ("icemix_flypy", "lk", "令", 5),  # ling:  l + ing(k)
    ("icemix_flypy", "yy", "云", 5),  # yun:   y + un(y)
    # lü/lüe (v-u common algebra swap)
    ("icemix_flypy", "lv", "旅", 9),  # lv (lü): bare v, no double-pinyin key
    ("icemix_flypy", "lt", "略", 5),  # lve→lt: l + ve(t) = lüe
    ("icemix_flypy", "nt", "虐", 5),  # nve→nt: n + ve(t)
    # zero-initial (a/e/o doubled then final mapped)
    ("icemix_flypy", "ah", "昂", 5),  # ang: a(zero-init→aa) + ang(h)
    # phrases
    ("icemix_flypy", "nihc", "你好", 3),  # ni + hǎo(hc)
    ("icemix_flypy", "vsgo", "中国", 2),  # zhōng(vs) + guó(go)
    ("icemix_flypy", "vswf", "中文", 2),  # zhōng(vs) + wén(wf)
    ("icemix_flypy", "bwjk", "北京", 3),  # běi(bw) + jīng(jk)
    ("icemix_flypy", "vfxb", "真心", 2),  # zhēn(vf) + xīn(xb)
    ("icemix_flypy", "qkxb", "清新", 2),  # qīng(qk) + xīn(xb)
    ("icemix_flypy", "uhxb", "伤心", 2),  # shāng(uh) + xīn(xb)
    ("icemix_flypy", "xbxk", "新型", 2),  # xīn(xb) + xíng(xk)
    # abbrev 首字母 — schema-agnostic, tested here once
    ("icemix_flypy", "zgyfjch", "直挂云帆济沧海", 1),
    # ════════════════════════════════════════════════════════════════════════
    # icemix_zrm  自然码  zh→v  ch→i  sh→u  (iang→d  ing→y  in→n  ei→z  ao→k)
    # ════════════════════════════════════════════════════════════════════════
    # initials
    ("icemix_zrm", "vh", "张", 5),  # zhang: zh(v) + ang(h)
    ("icemix_zrm", "ih", "长", 5),  # chang: ch(i) + ang(h)
    ("icemix_zrm", "uh", "上", 5),  # shang: sh(u) + ang(h)
    # finals — differences from flypy highlighted
    ("icemix_zrm", "xd", "想", 5),  # xiang: x + iang(d)  ← iang→d not l
    ("icemix_zrm", "xs", "熊", 5),  # xiong: x + iong(s)
    ("icemix_zrm", "bm", "边", 5),  # bian:  b + ian(m)
    ("icemix_zrm", "dy", "定", 5),  # ding:  d + ing(y)   ← ing→y not k
    ("icemix_zrm", "fh", "方", 5),  # fang:  f + ang(h)
    ("icemix_zrm", "lv", "旅", 9),  # lv (lü)
    ("icemix_zrm", "lt", "略", 5),  # lve→lt
    # phrases
    ("icemix_zrm", "nihk", "你好", 2),  # ni + hǎo(hk)  ← ao→k not c
    ("icemix_zrm", "vsgo", "中国", 2),  # zhōng(vs) + guó(go)
    ("icemix_zrm", "vswf", "中文", 2),  # zhōng(vs) + wén(wf)
    ("icemix_zrm", "bzjy", "北京", 2),  # běi(bz) + jīng(jy)  ← ei→z ing→y
    ("icemix_zrm", "vfxn", "真心", 2),  # zhēn(vf) + xīn(xn)  ← in→n not b
    ("icemix_zrm", "xdxn", "相信", 2),  # xiāng(xd) + xìn(xn)
    ("icemix_zrm", "uhxn", "伤心", 2),  # shāng(uh) + xīn(xn)
    # ════════════════════════════════════════════════════════════════════════
    # icemix_abc  智能ABC  zh→a  ch→e  sh→v  (iang→t  ian→w  ing→y  ei→q  ao→k)
    # ════════════════════════════════════════════════════════════════════════
    # initials
    ("icemix_abc", "ah", "张", 5),  # zhang: zh(a) + ang(h)
    ("icemix_abc", "eh", "长", 5),  # chang: ch(e) + ang(h)
    ("icemix_abc", "vh", "上", 5),  # shang: sh(v) + ang(h)
    # finals — differences from flypy highlighted
    ("icemix_abc", "xt", "想", 5),  # xiang: x + iang(t)  ← iang→t not l
    ("icemix_abc", "xs", "熊", 5),  # xiong: x + iong(s)
    ("icemix_abc", "bw", "边", 5),  # bian:  b + ian(w)   ← ian→w not m
    ("icemix_abc", "dy", "定", 5),  # ding:  d + ing(y)
    ("icemix_abc", "fh", "方", 5),  # fang:  f + ang(h)
    # phrases
    ("icemix_abc", "nihk", "你好", 2),  # ni + hǎo(hk)  ← ao→k
    ("icemix_abc", "asgo", "中国", 2),  # zhōng(as) + guó(go)
    ("icemix_abc", "aswf", "中文", 2),  # zhōng(as) + wén(wf)
    ("icemix_abc", "bqjy", "北京", 3),  # běi(bq) + jīng(jy)  ← ei→q ing→y
    ("icemix_abc", "ahjy", "张静", 2),  # zhāng(ah) + jìng(jy)
    ("icemix_abc", "xtxi", "详细", 2),  # xiáng(xt) + xì(xi)
]


def main():
    schema_filter = None
    shared_dir = "/usr/share/rime-data"
    user_dir = None

    args = sys.argv[1:]
    i = 0
    while i < len(args) and args[i].startswith("-"):
        flag = args[i]
        i += 1
        if i >= len(args):
            print(f"e2e_test: {flag} requires an argument", file=sys.stderr)
            sys.exit(1)
        if flag == "-s":
            schema_filter = args[i]
        elif flag == "-d":
            shared_dir = args[i]
        elif flag == "-u":
            user_dir = args[i]
        else:
            print(f"e2e_test: unknown option {flag}", file=sys.stderr)
            print(
                "Usage: e2e_test.py [-s FILTER] [-d SHARED_DIR] [-u USER_DIR]",
                file=sys.stderr,
            )
            sys.exit(1)
        i += 1

    pyrime_ext.init(shared_data_dir=shared_dir, user_data_dir=user_dir)

    passed = failed = skipped = 0
    sid = None
    cur_schema = None

    for schema, inp, expected, top_n in TESTS:
        if schema_filter and schema_filter not in schema:
            skipped += 1
            continue

        if schema != cur_schema:
            if sid is not None:
                rime.destroy_session(sid)
            sid = rime.create_session()
            cur_schema = schema
            rime.select_schema(sid, schema)
            rime.set_option(sid, "ascii_mode", False)
            if not pyrime_ext.is_operational(sid):
                print(
                    f"SKIP  [{schema:<14}] schema not deployed — "
                    f"add it to your ibus-rime schema_list to enable"
                )
                rime.destroy_session(sid)
                sid = None
                cur_schema = None
                skipped += 1
                continue

        if sid is None:
            skipped += 1
            continue

        pyrime_ext.type_input(sid, inp)
        ctx = rime.get_context(sid)
        cands = [c.text for c in ctx.menu.candidates[:top_n]]

        if expected in cands:
            print(f"PASS  [{schema:<14}] {inp:<8} → '{expected}' (top {top_n})")
            passed += 1
        else:
            all_cands = [c.text for c in ctx.menu.candidates[:8]]
            extra = ctx.menu.num_candidates - 8
            got = ", ".join(f"'{c}'" for c in all_cands)
            if extra > 0:
                got += f", …({extra} more)"
            print(
                f"FAIL  [{schema:<14}] {inp:<8} → expected '{expected}' "
                f"in top {top_n}, got: [{got}]"
            )
            failed += 1

    if sid is not None:
        rime.destroy_session(sid)

    print(f"\n{passed} passed, {failed} failed", end="")
    if skipped:
        print(f", {skipped} skipped", end="")
    print()

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
