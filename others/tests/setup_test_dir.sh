#!/usr/bin/env bash
# Prepares /tmp/rime_icemix_test for e2e_test.
# Run once (or after schema changes that require redeployment).
#
# Copies schema YAMLs, Lua scripts, dict data, and pre-compiled binaries
# from the ibus-rime user directory into an isolated test sandbox so tests
# never read from or write to the live ibus-rime deployment.

set -euo pipefail

IBUS_RIME="$HOME/.config/ibus/rime"
TEST_DIR="/tmp/rime_icemix_test"

echo "Setting up $TEST_DIR ..."
mkdir -p "$TEST_DIR/build"

# Schema YAMLs and dict YAML source files
cp -f "$IBUS_RIME"/*.yaml    "$TEST_DIR/"     2>/dev/null || true

# Pre-compiled binary files (prism, table, reverse) — avoids a full recompile
# on first run.  The test harness will recompile anything that's stale.
cp -f "$IBUS_RIME/build"/*.bin  "$TEST_DIR/build/"  2>/dev/null || true
# Copy .yaml from build EXCEPT default.yaml, which we rewrite below so that
# all icemix schemas are deployed (the user's default.custom.yaml may only
# enable a subset).
cp -f "$IBUS_RIME/build"/*.yaml "$TEST_DIR/build/"  2>/dev/null || true
rm -f "$TEST_DIR/build/default.yaml"

# Lua scripts required by the icemix schema
cp -rf "$IBUS_RIME/lua"      "$TEST_DIR/"     2>/dev/null || true

# Dictionary data sub-directories
cp -rf "$IBUS_RIME/cn_dicts" "$TEST_DIR/"     2>/dev/null || true
cp -rf "$IBUS_RIME/en_dicts" "$TEST_DIR/"     2>/dev/null || true
cp -rf "$IBUS_RIME/frost"    "$TEST_DIR/"     2>/dev/null || true

# Override default.custom.yaml so maintenance deploys ALL icemix schemas.
# The user's live default.custom.yaml may only list a subset of schemas.
cat > "$TEST_DIR/default.custom.yaml" << 'YAML'
patch:
  schema_list:
    - schema: icemix_flypy
    - schema: icemix_abc
    - schema: icemix_jiajia
    - schema: icemix_mspy
    - schema: icemix_sogou
    - schema: icemix_ziguang
    - schema: icemix_zrm
  menu/page_size: 9
YAML

echo "Done.  Test dir contains:"
echo "  $(ls "$TEST_DIR"/*.yaml 2>/dev/null | wc -l) YAML files"
echo "  $(ls "$TEST_DIR/build"/*.bin 2>/dev/null | wc -l) pre-compiled .bin files"
echo "  $(ls "$TEST_DIR/lua"/*.lua 2>/dev/null | wc -l) Lua scripts"
