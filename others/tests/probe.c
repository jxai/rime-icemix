/*
 * probe — look up candidates for one or more typed inputs.
 *
 * Usage:
 *   ./probe [-s SCHEMA] [-d SHARED_DIR] [-u USER_DIR] INPUT [INPUT ...]
 *
 * Options:
 *   -s SCHEMA      schema to use (default: icemix_flypy)
 *   -d SHARED_DIR  system rime data dir (default: /usr/share/rime-data)
 *   -u USER_DIR    user rime data dir   (default: $HOME/.config/ibus/rime)
 *
 * Examples:
 *   ./probe xl vh bwjk
 *   ./probe -s icemix_zrm xd bzjy
 *   ./probe -s icemix_abc asgo bqjy
 *   ./probe -u /tmp/rime_icemix_test xl vh
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rime_api.h"

#define MAX_CANDS 9

#define USAGE \
  "Usage: probe [-s SCHEMA] [-d SHARED_DIR] [-u USER_DIR] INPUT [INPUT ...]\n"

int main(int argc, char* argv[]) {
  const char* schema = "icemix_flypy";
  const char* shared_dir = "/usr/share/rime-data";
  const char* user_dir = NULL; /* resolved below if not set */
  int argi = 1;

  while (argi < argc && argv[argi][0] == '-') {
    const char* flag = argv[argi++];
    if (argi >= argc) {
      fprintf(stderr, "probe: %s requires an argument\n", flag);
      return 1;
    }
    if (strcmp(flag, "-s") == 0)
      schema = argv[argi++];
    else if (strcmp(flag, "-d") == 0)
      shared_dir = argv[argi++];
    else if (strcmp(flag, "-u") == 0)
      user_dir = argv[argi++];
    else {
      fprintf(stderr, "probe: unknown option %s\n" USAGE, flag);
      return 1;
    }
  }

  if (argi >= argc) {
    fprintf(stderr, USAGE);
    return 1;
  }

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
  traits.min_log_level = 3;

  RimeApi* r = rime_get_api();
  r->setup(&traits);
  r->initialize(&traits);

  RimeSessionId sid = r->create_session();
  if (!r->select_schema(sid, schema)) {
    fprintf(stderr, "probe: cannot load schema '%s'\n", schema);
    r->destroy_session(sid);
    r->finalize();
    return 1;
  }
  r->set_option(sid, "ascii_mode", False);

  for (; argi < argc; argi++) {
    r->clear_composition(sid);
    r->set_input(sid, argv[argi]);

    RIME_STRUCT(RimeContext, ctx);
    r->get_context(sid, &ctx);

    printf("%-12s →", argv[argi]);
    int n = ctx.menu.num_candidates < MAX_CANDS ? ctx.menu.num_candidates
                                                : MAX_CANDS;
    for (int i = 0; i < n; i++)
      printf("  %s",
             ctx.menu.candidates[i].text ? ctx.menu.candidates[i].text : "?");
    if (ctx.menu.num_candidates > MAX_CANDS)
      printf("  …(%d more)", ctx.menu.num_candidates - MAX_CANDS);
    printf("\n");

    r->free_context(&ctx);
  }

  r->destroy_session(sid);
  r->finalize();
  return 0;
}
