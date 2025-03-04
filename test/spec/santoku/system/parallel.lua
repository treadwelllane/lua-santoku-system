local sys = require("santoku.system")
local atom = sys.atom(1, 1)

local vals = {}
for i = 1, 10 do
  vals[i] = i
end

for data in sys.sh({
  jobs = 10,
  fn = function (job)
    while true do
      local a = atom()
      if a > #vals then
        break
      else
        print(job, a, vals[a])
        io.stdout:flush()
      end
    end
  end
}) do
  print("  result", data)
end
