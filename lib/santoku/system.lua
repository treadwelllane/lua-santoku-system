local iter = require("santoku.iter")
local each = iter.each

local tbl = require("santoku.table")
local assign = tbl.assign

local validate = require("santoku.validate")
local hasindex = validate.hasindex

local posix = require("santoku.system.posix")
local pread = require("santoku.system.pread")
local sh = require("santoku.system.sh")

local function execute (opts)
  assert(hasindex(opts))
  opts.execute = true
  return each(print, sh(opts))
end

return assign({}, {
  execute = execute,
  pread = pread,
  sh = sh
}, posix)
