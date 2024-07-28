local pread = require("santoku.system.pread")

local err = require("santoku.error")
local varg = require("santoku.varg")
local arr = require("santoku.array")
local str = require("santoku.string")

return function (opts)

  local iter = pread(opts)

  local done = false
  local flushed = false
  local pid_chunks = {}
  local pid_ready = {}

  local function helper ()

    if done and flushed then
      return
    elseif done and not flushed then
      while true do
        local pid, chunks = next(pid_chunks)
        if not pid then
          flushed = true
          return
        end
        pid_chunks[pid] = nil
        if #chunks > 0 then
          local out = arr.concat(chunks)
          return (str.gsub(out, "\n+$", ""))
        end
      end
    end

    local pid, se = next(pid_ready)

    if pid then

      local chunks = pid_chunks[pid]

      local s, e = se.s, se.e

      arr.overlay(chunks, #chunks,
        (str.sub(chunks[#chunks], 1, s - 1)),
        (str.sub(chunks[#chunks], e + 1, #chunks[#chunks])))

      local out = arr.concat(chunks, "", 1, #chunks - 1)

      arr.overlay(chunks, 1, chunks[#chunks])

      if chunks[#chunks] == "" then
        chunks[#chunks] = nil
      end

      if chunks[#chunks] then
        local s, e = str.find(chunks[#chunks], "\n+")
        if s then
          pid_ready[pid] = { s = s, e = e }
        else
          pid_ready[pid] = nil
        end
      else
        pid_ready[pid] = nil
      end

      return out

    end

    return varg.tup(function (ev, pid, ...)

      if ev == nil then
        done = true
        return helper()
      end

      if ev == "stdout" then

        local chunks = pid_chunks[pid] or {}
        pid_chunks[pid] = chunks

        local chunk = (...)
        chunks[#chunks + 1] = chunk

        local s, e = str.find(chunks[#chunks], "\n+")
        if s then
          pid_ready[pid] = { s = s, e = e }
        end

        return helper()

      elseif ev == "exit" then

        local reason, status = ...
        if reason == "exited" and status == 0 then
          return helper()
        else
          return err.error("child process exited with unexpected status", ...)
        end

      else
        return err.error("unexpected event from pread", ev, ...)
      end

    end, iter())

  end

  return helper

end
