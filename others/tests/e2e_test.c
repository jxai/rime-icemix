/*
 * End-to-end test for icemix schemas.
 *
 * Tests that typed double-pinyin inputs produce the expected Chinese character
 * candidates, exercising the full speller algebra → syllabification →
 * translation pipeline via the librime C API.
 *
 * Build:
 *   make
 * Run:
 *   ./e2e_test
 *   ./e2e_test -s flypy              # run only tests whose schema contains
 * "flypy"
 *   ./e2e_test -d /usr/share/rime-data -u /tmp/rime_icemix_test
 *
 * Options:
 *   -s SCHEMA_FILTER   only run tests whose schema name contains SCHEMA_FILTER
 *   -d SHARED_DIR      system rime data dir (default: /usr/share/rime-data)
 *   -u USER_DIR        user rime data dir   (default: $HOME/.config/ibus/rime)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rime_api.h"

/* ── directories ─────────────────────────────────────────────────────────── */

/* shared_data_dir: system rime schemas (avoids scanning ibus-rime's user dir
 * for stale userdbs).  user_data_dir: the live ibus-rime deployment, read at
 * runtime from $HOME so no path needs hardcoding. */

/* ── algebra key mappings (comment reference) ────────────────────────────────
 *
 * icemix_flypy (小鹤双拼):
 *   Initials: zh→v  ch→i  sh→u
 *   Finals:   iu→q  ei→w  uan→r  ue/ve→t  un→y  uo→o  ie→p
 *             ong/iong→s  ing/uai→k  ai→d  en→f  eng→g
 *             iang/uang→l  ang→h  ian→m  an→j  ou→z
 *             ia/ua→x  iao→n  ao→c  ui→v  in→b
 *
 * icemix_zrm (自然码):
 *   Initials: zh→v  ch→i  sh→u    (same as flypy)
 *   Finals:   iu→q  ia/ua→w  uan→r  ue/ve→t  ing/uai→y  uo→o  un→p
 *             ong/iong→s  iang/uang→d  en→f  eng→g  ang→h
 *             ian→m  an→j  iao→c  ao→k  ai→l  ei→z  ie→x  ui→v  ou→b  in→n
 *
 * icemix_abc (智能ABC双拼):
 *   Initials: zh→a  ch→e  sh→v
 *   Finals:   ei→q  ian→w  er/iu→r  iang/uang→t  ing→y  uo→o  uan→p
 *             ong/iong→s  ia/ua→d  en→f  eng→g  ang→h  an→j  iao→z  ao→k
 *             in/uai→c  ai→l  ie→x  ou→b  un→n  ue/ui/ve→m
 *
 * ───────────────────────────────────────────────────────────────────────────
 */

static RimeApi* rime;

typedef struct {
  const char* schema;   /* e.g. "icemix_flypy" */
  const char* input;    /* typed string, e.g. "xl" */
  const char* expected; /* character that must appear in top N candidates */
  int top_n;            /* search within the first top_n candidates */
} Test;

/* ── test table ──────────────────────────────────────────────────────────── */
static const Test TESTS[] = {
    /* ════════════════════════════════════════════════════════════════════════
     * icemix_flypy  小鹤双拼  zh→v  ch→i  sh→u
     * ════════════════════════════════════════════════════════════════════════
     */
    /* initials */
    {"icemix_flypy", "vh", "张", 5}, /* zhang: zh(v) + ang(h) */
    {"icemix_flypy", "ih", "长", 5}, /* chang: ch(i) + ang(h) */
    {"icemix_flypy", "uh", "上", 5}, /* shang: sh(u) + ang(h) */
    /* finals */
    {"icemix_flypy", "xl", "想", 5}, /* xiang: x + iang(l) */
    {"icemix_flypy", "xs", "熊", 5}, /* xiong: x + iong(s) */
    {"icemix_flypy", "bm", "边", 5}, /* bian:  b + ian(m) */
    {"icemix_flypy", "dk", "定", 5}, /* ding:  d + ing(k) */
    {"icemix_flypy", "fh", "方", 5}, /* fang:  f + ang(h) */
    {"icemix_flypy", "lk", "令", 5}, /* ling:  l + ing(k) */
    {"icemix_flypy", "yy", "云", 5}, /* yun:   y + un(y) */
    /* lü/lüe (v-u common algebra swap) */
    {"icemix_flypy", "lv", "旅", 9}, /* lv (lü): bare v, no double-pinyin key */
    {"icemix_flypy", "lt", "略", 5}, /* lve→lt: l + ve(t) = lüe */
    {"icemix_flypy", "nt", "虐", 5}, /* nve→nt: n + ve(t) */
    /* zero-initial (a/e/o doubled then final mapped) */
    {"icemix_flypy", "ah", "昂", 5}, /* ang: a(zero-init→aa) + ang(h) */
    /* phrases */
    {"icemix_flypy", "nihc", "你好", 3}, /* ni + hǎo(hc) */
    {"icemix_flypy", "vsgo", "中国", 2}, /* zhōng(vs) + guó(go) */
    {"icemix_flypy", "vswf", "中文", 2}, /* zhōng(vs) + wén(wf) */
    {"icemix_flypy", "bwjk", "北京", 3}, /* běi(bw) + jīng(jk) */
    {"icemix_flypy", "vfxb", "真心", 2}, /* zhēn(vf) + xīn(xb) */
    {"icemix_flypy", "qkxb", "清新", 2}, /* qīng(qk) + xīn(xb) */
    {"icemix_flypy", "uhxb", "伤心", 2}, /* shāng(uh) + xīn(xb) */
    {"icemix_flypy", "xbxk", "新型", 2}, /* xīn(xb) + xíng(xk) */
    /* abbrev 首字母 — schema-agnostic, tested here once */
    {"icemix_flypy", "zgyfjch", "直挂云帆济沧海", 1},

    /* ════════════════════════════════════════════════════════════════════════
     * icemix_zrm  自然码  zh→v  ch→i  sh→u  (iang→d  ing→y  in→n  ei→z  ao→k)
     * ════════════════════════════════════════════════════════════════════════
     */
    /* initials */
    {"icemix_zrm", "vh", "张", 5}, /* zhang: zh(v) + ang(h) */
    {"icemix_zrm", "ih", "长", 5}, /* chang: ch(i) + ang(h) */
    {"icemix_zrm", "uh", "上", 5}, /* shang: sh(u) + ang(h) */
    /* finals — differences from flypy highlighted */
    {"icemix_zrm", "xd", "想", 5}, /* xiang: x + iang(d)  ← iang→d not l */
    {"icemix_zrm", "xs", "熊", 5}, /* xiong: x + iong(s) */
    {"icemix_zrm", "bm", "边", 5}, /* bian:  b + ian(m) */
    {"icemix_zrm", "dy", "定", 5}, /* ding:  d + ing(y)   ← ing→y not k */
    {"icemix_zrm", "fh", "方", 5}, /* fang:  f + ang(h) */
    {"icemix_zrm", "lv", "旅", 9}, /* lv (lü) */
    {"icemix_zrm", "lt", "略", 5}, /* lve→lt */
    /* phrases */
    {"icemix_zrm", "nihk", "你好", 2}, /* ni + hǎo(hk)  ← ao→k not c */
    {"icemix_zrm", "vsgo", "中国", 2}, /* zhōng(vs) + guó(go) */
    {"icemix_zrm", "vswf", "中文", 2}, /* zhōng(vs) + wén(wf) */
    {"icemix_zrm", "bzjy", "北京", 2}, /* běi(bz) + jīng(jy)  ← ei→z ing→y */
    {"icemix_zrm", "vfxn", "真心", 2}, /* zhēn(vf) + xīn(xn)  ← in→n not b */
    {"icemix_zrm", "xdxn", "相信", 2}, /* xiāng(xd) + xìn(xn) */
    {"icemix_zrm", "uhxn", "伤心", 2}, /* shāng(uh) + xīn(xn) */

    /* ════════════════════════════════════════════════════════════════════════
     * icemix_abc  智能ABC  zh→a  ch→e  sh→v  (iang→t  ian→w  ing→y  ei→q  ao→k)
     * ════════════════════════════════════════════════════════════════════════
     */
    /* initials */
    {"icemix_abc", "ah", "张", 5}, /* zhang: zh(a) + ang(h) */
    {"icemix_abc", "eh", "长", 5}, /* chang: ch(e) + ang(h) */
    {"icemix_abc", "vh", "上", 5}, /* shang: sh(v) + ang(h) */
    /* finals — differences from flypy highlighted */
    {"icemix_abc", "xt", "想", 5}, /* xiang: x + iang(t)  ← iang→t not l */
    {"icemix_abc", "xs", "熊", 5}, /* xiong: x + iong(s) */
    {"icemix_abc", "bw", "边", 5}, /* bian:  b + ian(w)   ← ian→w not m */
    {"icemix_abc", "dy", "定", 5}, /* ding:  d + ing(y) */
    {"icemix_abc", "fh", "方", 5}, /* fang:  f + ang(h) */
    /* phrases */
    {"icemix_abc", "nihk", "你好", 2}, /* ni + hǎo(hk)  ← ao→k */
    {"icemix_abc", "asgo", "中国", 2}, /* zhōng(as) + guó(go) */
    {"icemix_abc", "aswf", "中文", 2}, /* zhōng(as) + wén(wf) */
    {"icemix_abc", "bqjy", "北京", 3}, /* běi(bq) + jīng(jy)  ← ei→q ing→y */
    {"icemix_abc", "ahjy", "张静", 2}, /* zhāng(ah) + jìng(jy) */
    {"icemix_abc", "xtxi", "详细", 2}, /* xiáng(xt) + xì(xi) */
};

#define NTESTS (int)(sizeof(TESTS) / sizeof(TESTS[0]))

/* ── helpers ────────────────────────────────────────────────────────────────
 */

static int find_candidate(RimeSessionId sid, const char* expected, int top_n) {
  RIME_STRUCT(RimeContext, ctx);
  if (!rime->get_context(sid, &ctx)) return 0;

  int found = 0;
  int limit = ctx.menu.num_candidates < top_n ? ctx.menu.num_candidates : top_n;
  for (int i = 0; i < limit && !found; i++) {
    if (ctx.menu.candidates[i].text &&
        strcmp(ctx.menu.candidates[i].text, expected) == 0)
      found = 1;
  }
  rime->free_context(&ctx);
  return found;
}

static void print_candidates(RimeSessionId sid, int max) {
  RIME_STRUCT(RimeContext, ctx);
  if (!rime->get_context(sid, &ctx)) return;

  int limit = ctx.menu.num_candidates < max ? ctx.menu.num_candidates : max;
  printf(" [");
  for (int i = 0; i < limit; i++) {
    if (i) printf(", ");
    printf("'%s'",
           ctx.menu.candidates[i].text ? ctx.menu.candidates[i].text : "?");
  }
  if (ctx.menu.num_candidates > max)
    printf(", …(%d more)", ctx.menu.num_candidates - max);
  printf("]");
  rime->free_context(&ctx);
}

/* ── main ───────────────────────────────────────────────────────────────────
 */

int main(int argc, char* argv[]) {
  const char* filter = NULL;
  const char* shared_dir = "/usr/share/rime-data";
  const char* user_dir = NULL; /* resolved below if not set */

  for (int i = 1; i < argc; i++) {
    const char* flag = argv[i];
    if ((strcmp(flag, "-s") == 0 || strcmp(flag, "-d") == 0 ||
         strcmp(flag, "-u") == 0) &&
        i + 1 < argc) {
      i++;
      if (strcmp(flag, "-s") == 0)
        filter = argv[i];
      else if (strcmp(flag, "-d") == 0)
        shared_dir = argv[i];
      else if (strcmp(flag, "-u") == 0)
        user_dir = argv[i];
    } else if (flag[0] != '-') {
      /* bare word: treat as schema filter for backwards compatibility */
      filter = flag;
    } else {
      fprintf(
          stderr,
          "Usage: e2e_test [-s SCHEMA_FILTER] [-d SHARED_DIR] [-u USER_DIR]\n");
      return 1;
    }
  }

  /* ── initialise librime ─────────────────────────────────────────── */
  char user_dir_buf[512];
  if (!user_dir) {
    snprintf(user_dir_buf, sizeof(user_dir_buf), "%s/.config/ibus/rime",
             getenv("HOME"));
    user_dir = user_dir_buf;
  }

  RIME_STRUCT(RimeTraits, traits);
  traits.shared_data_dir = shared_dir;
  traits.user_data_dir = user_dir;
  traits.app_name = "rime.icemix_test";
  traits.min_log_level = 3; /* suppress INFO/WARNING spam */

  rime = rime_get_api();
  rime->setup(&traits);
  rime->initialize(&traits);

  /* Let ibus-rime manage deployment; we only check for pending work.
   * Schemas not in the user's schema_list will not be compiled here —
   * their tests are skipped rather than failed. */
  rime->start_maintenance(False);
  rime->join_maintenance_thread();

  /* ── run tests ──────────────────────────────────────────────────── */
  int pass = 0, fail = 0, skip = 0;
  RimeSessionId sid = 0;
  const char* schema = NULL;

  for (int i = 0; i < NTESTS; i++) {
    const Test* t = &TESTS[i];

    /* optional schema filter */
    if (filter && strstr(t->schema, filter) == NULL) {
      skip++;
      continue;
    }

    /* (re)create session when schema changes */
    if (!sid || !schema || strcmp(schema, t->schema) != 0) {
      if (sid) rime->destroy_session(sid);
      sid = rime->create_session();
      schema = t->schema;
      rime->select_schema(sid, t->schema);
      rime->set_option(sid, "ascii_mode", False);
      /* Probe with a single letter to confirm the prism is compiled.
       * select_schema() returns True even when the prism is missing (the YAML
       * is found but the binary data is not yet built), so we must check that
       * actual candidates are produced before running tests. */
      rime->set_input(sid, "x");
      RIME_STRUCT(RimeContext, probe_ctx);
      rime->get_context(sid, &probe_ctx);
      int operational = probe_ctx.menu.num_candidates > 0;
      rime->free_context(&probe_ctx);
      rime->clear_composition(sid);
      if (!operational) {
        printf(
            "SKIP  [%-14s] schema not deployed — "
            "add it to your ibus-rime schema_list to enable\n",
            t->schema);
        rime->destroy_session(sid);
        sid = 0;
        schema = NULL;
        skip++;
        continue;
      }
    }

    /* If the current schema failed to load, skip its remaining tests. */
    if (!sid) {
      skip++;
      continue;
    }

    rime->clear_composition(sid);
    rime->set_input(sid, t->input);

    if (find_candidate(sid, t->expected, t->top_n)) {
      printf("PASS  [%-14s] %-8s → '%s' (top %d)\n", t->schema, t->input,
             t->expected, t->top_n);
      pass++;
    } else {
      printf("FAIL  [%-14s] %-8s → expected '%s' in top %d, got:", t->schema,
             t->input, t->expected, t->top_n);
      print_candidates(sid, 8);
      printf("\n");
      fail++;
    }
  }

  if (sid) rime->destroy_session(sid);
  rime->finalize();

  printf("\n%d passed, %d failed", pass, fail);
  if (skip) printf(", %d skipped", skip);
  printf("\n");
  return fail > 0 ? 1 : 0;
}
