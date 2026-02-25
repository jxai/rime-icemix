local function debugger(input, env)
  local ctx = env.engine.context
  local inp = ctx.input
  local all = {}

  for cand in input:iter() do
    table.insert(all, cand)
  end

  -- Log all candidates to /tmp/rime.ibus.INFO for this input
  log.info(string.format("=== debugger: input=%q  #cands=%d ===", inp, #all))
  for i, cand in ipairs(all) do
    local span = cand._end - cand.start
    local extra = ""
    local p = cand:to_phrase()
    if p then
      extra = string.format(" w=%.2f cc=%d lang=%s",
        p.weight, p.entry.commit_count, p.lang_name)
    end
    log.info(string.format("  [%d] type=%-12s span=%d q=%.3f text=%q preedit=%q%s",
      i, cand.type, span, cand.quality, cand.text, cand.preedit, extra))
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
