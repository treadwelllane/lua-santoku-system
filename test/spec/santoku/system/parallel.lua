-- local sys = require("santoku.system")
-- local str = require("santoku.string")
-- local atom = sys.atom(1, 1)

-- local vals = {}
-- for i = 1, 100 do
--   vals[i] = i
-- end

-- for data in sys.sh({
--   jobs = 1,
--   fn = function (job)
--     while true do
--       local a = atom()
--       io.stderr:write("  A: " .. a .. "\n");
--       io.stderr:flush()
--       if a > #vals then
--         break
--       else
--         print(job, a, vals[a])
--         io.stdout:flush()
--       end
--     end
--   end
-- }) do
--   print("  result", data)
-- end


-- local sys = require("santoku.system")
-- print(sys.pid(), sys.ppid())
