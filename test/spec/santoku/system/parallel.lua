local sys = require("santoku.system")
local counter = sys.atom(1, 0.1)

local vals = {}
for i = 1, 100 do
  vals[i] = i
end

local mutex = sys.mutex()

for data in sys.sh({
  jobs = 10,
  fn = function (job)
    while true do
      if a > #vals then
        break
      else
        sys.sleep(1)
        print(job, a, vals[a])
        io.stdout:flush()
      end
    end
  end
}) do
  print("  result", data)
end
