local sys = require("santoku.system")

local iterations = 4
local jobs = math.ceil(sys.get_num_cores() / 2)

for _ = 1, iterations do
  local iter = sys.sh({
    jobs = jobs,
    fn = function (job)
      print(job)
    end
  })
  local o = {}
  for res in iter do
    o[tonumber(res)] = true
  end
  for i = 1, jobs do
    assert(o[i], "Missing: " .. i)
  end
end
