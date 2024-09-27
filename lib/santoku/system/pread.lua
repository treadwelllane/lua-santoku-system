local err = require("santoku.error")
local wrapnil = err.wrapnil

local varg = require("santoku.varg")
local tup = varg.tup

local arr = require("santoku.array")
local shift = arr.shift
local cat = arr.concat

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

local flush = io.flush
local stderr = io.stderr
local exit = os.exit

local poll = wrapnil(require("santoku.system.posix.poll"))

local function run_child (opts, job, sr, sw, er, ew)

  if opts.env then
    for k, v in pairs(opts.env) do
      setenv(k, v)
    end
  end

  if opts.job_var then
    setenv(opts.job_var, tostring(job))
  end

  if not opts.execute then
    if opts.stdout then
      close(sr)
      dup2(sw, 1)
    end
    if opts.stderr then
      close(er)
      dup2(ew, 2)
    end
  end

  if opts.fn then

    opts.fn(job, opts)
    exit(0)

  else

    local prog = opts[1]
    shift(opts)

    tup(function (_, err, cd)

      stderr:write(cat({ "Error in exec for: ", prog or "(nil)", ": ", err or "(nil)", ": ", cd or "(nil)", "\n" }))
      flush(stderr)
      exit(1)

    end, err.pcall(execp, prog, opts))

  end

end

local function run_parent_loop (opts, children, fds, fd_child)

  local active = #children
  local done = {}

  local function helper ()

    if active == 0 then
      return
    end

    if opts.execute then
      local c = children[1]
      local _, reason, status = wait(c.pid)
      active = 0
      return "exit", c.pid, reason, status
    end

    poll(fds)

    for fd, cfg in pairs(fds) do
      if cfg.revents.IN then
        local res = read(fd, opts.bufsize)
        local c = fd_child[fd]
        return
          (fd == c.sr and "stdout") or
          (fd == c.er and "stderr") or "unknown",
            c.pid, res
      elseif cfg.revents.HUP then
        local c = fd_child[fd]
        if not done[c] then
          c.closed = (c.closed or 0) + 1
          if c.closed >= 2 then
            active = active - 1
            done[c] = true
            local _, reason, status = wait(c.pid)
            return "exit", c.pid, reason, status
          end
        end
      end
    end

    return helper()

  end

  return helper

end

local function run_parent (opts, children)

  local fds = {}
  local fd_child = {}

  for i = 1, #children do
    local c = children[i]
    if opts.stdout then
      close(c.sw)
      fds[c.sr] = { events = { IN = true } }
      fd_child[c.sr] = c
    end
    if opts.stderr then
      close(c.ew)
      fds[c.er] = { events = { IN = true } }
      fd_child[c.er] = c
    end
  end

  return run_parent_loop(opts, children, fds, fd_child)

end

return function (opts)

  opts.bufsize = opts.bufsize or BUFSIZ

  flush()
  local children = {}

  opts.jobs = (opts.execute or not opts.jobs) and 1 or opts.jobs
  opts.stderr = opts.stderr == true
  opts.stdout = opts.stdout ~= false

  for job = 1, opts.jobs or 1 do
    local sr, sw, er, ew
    if opts.stdout then
      sr, sw = pipe()
    end
    if opts.stderr then
      er, ew = pipe()
    end
    local pid = fork()
    if pid == 0 then
      return run_child(opts, job, sr, sw, er, ew)
    else
      arr.push(children, {
        pid = pid,
        sr = sr, sw = sw,
        er = er, ew = ew,
      })
    end
  end

  return run_parent(opts, children)

end
