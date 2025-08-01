local env = {

  name = "santoku-system",
  version = "0.0.56-1",
  variable_prefix = "TK_SYSTEM",
  license = "MIT",
  public = true,

  cflags = { "-pthread" },
  ldflags = { "-pthread", "-lrt" },

  dependencies = {
    "lua >= 5.1",
    "santoku >= 0.0.278-1",
  },

  test = {
    dependencies = {
      "luacov >= 0.15.0-1",
    }
  },

}

env.homepage = "https://github.com/treadwelllane/lua-" .. env.name
env.tarball = env.name .. "-" .. env.version .. ".tar.gz"
env.download = env.homepage .. "/releases/download/" .. env.version .. "/" .. env.tarball

return {
  type = "lib",
  env = env,
}
