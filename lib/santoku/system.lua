local iter = require("santoku.iter")
local ieach = iter.ieach

local tbl = require("santoku.table")
local tassign = tbl.assign

local validate = require("santoku.validate")
local hasindex = validate.hasindex

local posix = require("santoku.system.posix")
local pread = require("santoku.system.pread")
local sh = require("santoku.system.sh")

local function execute (opts)
  assert(hasindex(opts))
  opts.execute = true
  return ieach(print, sh(opts))
end

return tassign({}, {
  execute = execute,
  pread = pread,
  sh = sh
}, posix)
