local env = {

  name = "santoku-system",
  version = "0.0.5-1",
  variable_prefix = "TK_SYSTEM",
  license = "MIT",
  public = true,

  dependencies = {
    "lua >= 5.1",
    "santoku >= 0.0.153-1",
  },

  test_dependencies = {
    "santoku-test >= 0.0.4-1",
    "luassert >= 1.9.0-1",
    "luacov >= scm-1",
  },

}

env.homepage = "https://github.com/treadwelllane/lua-" .. env.name
env.tarball = env.name .. "-" .. env.version .. ".tar.gz"
env.download = env.homepage .. "/releases/download/" .. env.version .. "/" .. env.tarball

return {
  env = env,
}
