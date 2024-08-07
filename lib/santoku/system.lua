local iter = require("santoku.iter")
local each = iter.each

local tbl = require("santoku.table")
local merge = tbl.merge

local posix = require("santoku.system.posix")
local pread = require("santoku.system.pread")
local sh = require("santoku.system.sh")

local function execute (opts)
  opts.execute = true
  return each(print, sh(opts))
end

return merge({
  execute = execute,
  pread = pread,
  sh = sh
}, posix, os)
