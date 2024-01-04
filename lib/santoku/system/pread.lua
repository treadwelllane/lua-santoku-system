local err = require("santoku.err")
local tup = require("santoku.tuple")
local gen = require("santoku.gen")

local posix = require("santoku.system.posix")
local poll = require("santoku.system.posix.poll")

local function run_child (check, opts, file, args, sr, sw, er, ew)
  for k, v in pairs(opts.env or {}) do
    check(posix.setenv(k, v))
  end
  if not opts.execute then
    check(posix.close(sr))
    check(posix.close(er))
    check(posix.dup2(sw, 1))
    check(posix.dup2(ew, 2))
  end
  local _, err, cd = posix.execp(file, args)
  io.stderr:write(table.concat({ "Error in exec for ", file, ": ", err, ": ", cd, "\n" }))
  io.stderr:flush()
  os.exit(1)
end

local function run_parent_loop (check, yield, opts, pid, fds, sr, er)

  if opts.execute then
    local _, reason, status = check(posix.wait(pid))
    yield("exit", reason, status)
    return
  end

  while true do

    check(poll(fds))

    for fd, cfg in pairs(fds) do

      if cfg.revents.IN then
        local res = check(posix.read(fd, opts.bufsize))
        if fd == sr then
          yield("stdout", res)
        elseif fd == er then
          yield("stderr", res)
        else
          check(false, "Invalid state: fd neither sr nor er")
        end
      elseif cfg.revents.HUP then
        check:exists(posix.close(fd))
        fds[fd] = nil
      end

      if not next(fds) then
        local _, reason, status = check(posix.wait(pid))
        yield("exit", reason, status)
        return
      end

    end
  end

end

local function run_parent (check, opts, pid, sr, sw, er, ew)

  check:exists(posix.close(sw))
  check:exists(posix.close(ew))

  local fds = { [sr] = { events = { IN = true } },
                [er] = { events = { IN = true } } }

  return gen(function (yield)
    err.check(err.pwrap(function (check)
      return run_parent_loop(check, yield, opts, pid, fds, sr, er)
    end))
  end)

end

return function (...)

  local opts, args, file

  if type((...)) == "table" then
    opts = tup.get(1, ...)
    file = tup.get(2, ...)
    args = { tup.sel(3, ...) }
  else
    opts = {}
    file = tup.get(1, ...)
    args = { tup.sel(2, ...) }
  end

  -- TODO: PIPE_BUF is probably not the best default
  opts.bufsize = opts.bufsize or 4096

  return err.pwrap(function (check)

    io.flush()
    local sr, sw = check(posix.pipe())
    local er, ew = check(posix.pipe())
    local pid = check(posix.fork())

    if pid == 0 then
      return run_child(check, opts, file, args, sr, sw, er, ew)
    else
      return run_parent(check, opts, pid, sr, sw, er, ew)
    end

  end)
end
