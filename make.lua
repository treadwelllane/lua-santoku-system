local env = {

  name = "santoku-system",
  version = "0.0.45-1",
  variable_prefix = "TK_SYSTEM",
  license = "MIT",
  public = true,

  cflags = { "-pthread" },
  ldflags = { "-pthread" },

  dependencies = {
    "lua >= 5.1",
    "santoku >= 0.0.245-1",
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
