local err = require("santoku.err")
local tup = require("santoku.tuple")
local posix = require("santoku.system.posix")

local M = {}

M.setenv = posix.setenv
M.pread = require("santoku.system.pread")
M.sh = require("santoku.system.sh")

M.execute = function (opts, ...)
  local args = tup(...)
  if type(opts) == "table" then
    opts.execute = true
  else
    args = tup(opts, args())
    opts = { execute = true }
  end
  return err.pwrap(function (check)
    check(M.sh(opts, args())):map(check):each(print)
  end)
end

return M
