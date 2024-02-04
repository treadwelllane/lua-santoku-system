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
      bufsize = 500
    })

    assert(teq({
      { "stdout", "a\n" },
      { "stderr", "b\n" },
      { "exit", "exited", 1 },
    }, icollect(imap(apack, iter))))

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

end)

test("should setenv", function ()

    local iter = sys.pread({
      "sh", "-c", "echo $HELLO",
      env = { HELLO = "Hello, World!" }, bufsize = 500
    })

    assert(teq({
      { "stdout", "Hello, World!\n" },
      { "exit", "exited", 0 },
    }, icollect(imap(apack, iter))))

end)
