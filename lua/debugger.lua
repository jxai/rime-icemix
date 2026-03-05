local enable_debug
local enable_log

local function debugger(input, env)
  local function _get_bool(cfg, def)
    local val = env.engine.schema.config:get_bool('debugger/' .. cfg)
    if val == nil then val = def end
    return val
  end
  if enable_debug == nil then enable_debug = _get_bool('enable', false) end
  if enable_log == nil then enable_log = _get_bool('enable_log', false) end

  if not enable_debug then
    for cand in input:iter() do yield(cand) end
    return
  end

  -- Starts debugging
  local ctx = env.engine.context
  local inp = ctx.input
  local all = {}

  for cand in input:iter() do
    table.insert(all, cand)
  end

  -- Log all candidates to /tmp/rime.ibus.INFO for this input
  if enable_log then
    log.info(string.format("=== debugger: input=%q  #cands=%d ===", inp, #all))
    for i, cand in ipairs(all) do
      local span = cand._end - cand.start
      local extra = ""
      local p = cand:to_phrase()
      if p then
        extra = string.format(" w=%.2f cc=%d lang=%s",
          p.weight, p.entry.commit_count, p.lang_name)
      end
      log.info(string.format("(%s)  [%d] type=%-12s span=%d q=%.3f text=%q preedit=%q%s",
        tostring(enable_log), i, cand.type, span, cand.quality, cand.text, cand.preedit, extra))
    end
  end

  -- Yield candidates with richer comment for on-screen display
  for _, cand in ipairs(all) do
    local span = cand._end - cand.start
    local extra = ""
    local p = cand:to_phrase()
    if p then
      extra = string.format("; w=%.2f", p.weight)
    end
    local info = string.format("[%s|sp:%d|q:%.2f%s] %s",
      cand.type, span, cand.quality, extra, cand.preedit)
    yield(ShadowCandidate(cand, cand.type, cand.text, info))
  end
end

return debugger
