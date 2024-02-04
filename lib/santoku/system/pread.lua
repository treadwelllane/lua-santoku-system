local err = require("santoku.error")
local wrapnil = err.wrapnil
local assert = err.assert
local error = err.error

local validate = require("santoku.validate")
local hasindex = validate.hasindex

local arr = require("santoku.array")
local ashift = arr.shift
local acat = arr.concat

local posix = require("santoku.system.posix")
local BUFSIZ = posix.BUFSIZ
local pipe = posix.pipe
local fork = posix.fork
local read = posix.read
local setenv = posix.setenv
local dup2 = posix.dup2
local close = posix.close
local execp = posix.execp
local wait = posix.wait

local _flush = io.flush
local _write = io.write
local _stderr = io.stderr
local _exit = os.exit

local _poll = require("santoku.system.posix.poll")
local poll = wrapnil(_poll)

local function run_child (opts, sr, sw, er, ew)

  if opts.env then
    for k, v in pairs(opts.env) do
      setenv(k, v)
    end
  end

  if not opts.execute then
    close(sr)
    close(er)
    dup2(sw, 1)
    dup2(ew, 2)
  end

  local _, err, cd = execp(opts[1], ashift(opts))

  _write(_stderr, acat({ "Error in exec for ", opts[1], ": ", err, ": ", cd, "\n" }))
  _flush(_stderr)
  _exit(1)

end

local function run_parent_loop (opts, pid, fds, sr, er)

  if opts.execute then
    local _, reason, status = wait(pid)
    return "exit", reason, status
  end

  local fd, cfg
  local done = false

  local function helper ()

    if done then
      return
    end

    poll(fds)

    fd, cfg = next(fds, fd)

    if not fd then
      done = true
      local _, reason, status = wait(pid)
      return "exit", reason, status
    end

    local revents = cfg.revents

    if revents.IN then
      local res = read(fd, opts.bufsize)
      if fd == sr then
        return "stdout", res
      elseif fd == er then
        return "stderr", res
      else
        error("polled fd is neither stdout nor stderr", fd)
      end
    elseif revents.HUP then
      close(fd)
      fds[fd] = nil
    end

    return helper()

  end

  return helper

end

local function run_parent (opts, pid, sr, sw, er, ew)

  close(sw)
  close(ew)

  local fds = { [sr] = { events = { IN = true } },
                [er] = { events = { IN = true } } }

  return run_parent_loop(opts, pid, fds, sr, er)

end

return function (opts)

  assert(hasindex(opts))
  opts.bufsize = opts.bufsize or BUFSIZ

  _flush()
  local sr, sw = pipe()
  local er, ew = pipe()
  local pid = fork()

  if pid == 0 then
    return run_child(opts, sr, sw, er, ew)
  else
    return run_parent(opts, pid, sr, sw, er, ew)
  end

end
