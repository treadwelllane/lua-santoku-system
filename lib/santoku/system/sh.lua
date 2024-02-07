local pread = require("santoku.system.pread")

local err = require("santoku.error")
local error = err.error

local varg = require("santoku.varg")
local tup = varg.tup

local arr = require("santoku.array")
local aoverlay = arr.overlay
local acat = arr.concat

local find = string.find
local sub = string.sub
local stderr = io.stderr

return function (opts)

  local iter = pread(opts)

  local done = false
  local chunks = {}

  local function helper ()

    if done then
      return
    end

    if #chunks > 0 then
      local s, e = find(chunks[#chunks], "\n+")
      if s then
        aoverlay(chunks, #chunks,
          (sub(chunks[#chunks], 1, s - 1)),
          (sub(chunks[#chunks], e + 1, #chunks[#chunks])))
        local out = acat(chunks, "", 1, #chunks - 1)
        aoverlay(chunks, 1, chunks[#chunks])
        return out
      end
    end

    return tup(function (ev, ...)

      if ev == nil then
        done = true
        if #chunks > 0 then
          return acat(chunks)
        else
          return
        end
      end

      if ev == "stdout" then

        chunks[#chunks + 1] = (...)
        return helper()

      elseif ev == "stderr" then

        stderr:write((...))
        return helper()

      elseif ev == "exit" then

        local reason, status = ...
        if reason == "exited" and status == 0 then
          done = true
          return
        else
          return error("child process exited with unexpected status", ...)
        end

      else
        return error("unexpected event from pread", ev, ...)
      end

    end, iter())

  end

  return helper

end
