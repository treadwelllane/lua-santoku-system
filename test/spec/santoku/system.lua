local test = require("santoku.test")
local serialize = require("santoku.serialize") -- luacheck: ignore

local err = require("santoku.error")
local assert = err.assert


local tbl = require("santoku.table")
local teq = tbl.equals

local iter = require("santoku.iter")
local icollect = iter.collect
local imap = iter.map

local arr = require("santoku.array")
local apack = arr.pack

local sys = require("santoku.system")

test("pread", function ()

  test("should provide a chunked iterator for a forked processes stout and stderr", function ()

    local iter = sys.pread({
      "sh", "-c", "echo a; sleep 1; echo b >&2; exit 1",
      bufsize = 500,
      stderr = true,
    })

    assert(teq({
      { "stdout", "a\n" },
      { "stderr", "b\n" },
      { "exit", "exited", 1 },
    }, icollect(imap(function (t, _, ...)
      return { t, ... }
    end, iter))))

  end)

end)

test("sh", function ()

  test("should provide a line-buffered iterator for a forked processes stout", function ()

    local iter = sys.sh({ "sh", "-c", "echo a; echo b; exit 0" })

    assert(teq({
      { "a" },
      { "b" },
    }, icollect(imap(apack, iter))))

  end)

  test("should work with longer outputs", function ()

    local iter = sys.sh({ "sh", "-c", "echo the quick brown fox; echo jumped over the lazy dog; exit 0" })

    assert(teq({
      { "the quick brown fox" },
      { "jumped over the lazy dog" },
    }, icollect(imap(apack, iter))))

  end)

  test("should support multi-processing", function ()

    local iter = sys.pread({
      jobs = 4, job_var = "JOB",
      "sh", "-c", "echo $JOB"
    })

    local r = arr.sort(icollect(imap(function (t, _, ...)
      return { t, ... }
    end, iter)), function (a, b)
      return a[2] < b[2]
    end)

    assert(teq({
      { "stdout", "1\n" },
      { "stdout", "2\n" },
      { "stdout", "3\n" },
      { "stdout", "4\n" },
      { "exit", "exited", 0 },
      { "exit", "exited", 0 },
      { "exit", "exited", 0 },
      { "exit", "exited", 0 }
    }, r))

  end)

  test("should support multi-processing with line chunking", function ()

    local iter = sys.sh({
      jobs = 4,
      "sh", "-c", "echo 1"
    })

    local r = arr.sort(icollect(imap(apack, iter)), function (a, b)
      return a[1] > b[1]
    end)

    assert(teq({
      { "1" },
      { "1" },
      { "1" },
      { "1" },
    }, r))

  end)

  test("should suppport multi-processing without exec", function ()

    local iter = sys.sh({
      jobs = 4, fn = function (job)
        print(job)
      end
    })

    local r = arr.sort(icollect(imap(apack, iter)), function (a, b)
      return a[1] < b[1]
    end)

    assert(teq({
      { "1" },
      { "2" },
      { "3" },
      { "4" },
    }, r))

  end)

end)

test("should setenv", function ()

  local iter = sys.pread({
    "sh", "-c", "echo $HELLO",
    env = { HELLO = "Hello, World!" }, bufsize = 500
  })

  assert(teq({
    { "stdout", "Hello, World!\n" },
    { "exit", "exited", 0 },
  }, icollect(imap(function (t, _, ...)
    return { t, ... }
  end, iter))))

end)
