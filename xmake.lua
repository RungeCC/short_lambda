set_project("short-lambda")
set_version("0.0.1")
set_allowedmodes("debug", "release")
set_toolchains("clang") -- FIXME: do not set it here
set_languages("c++26")
set_warnings("allextra", "error")

add_rules("mode.release")

add_cxxflags("-Wno-experimental-header-units") -- FIXME: only for clang 
add_cxxflags("-Wno-c++20-compat")
add_cxxflags("-xc++") -- FIXME: need to be removed

add_rules("plugin.compile_commands.autoupdate", { outputdir = "." })

target("short-lambda.header")
do
  set_kind("headeronly")
  add_includedirs("include", { interface = true })
  add_headerfiles("include/(*.hpp)")
end

target("short-lambda.module")
do
  set_kind("moduleonly")
  add_files("module/short-lambda.cppm", { public = true })
  add_headerfiles("module/*.hpp")
  set_policy("build.c++.modules", true)
end

function make_example(name)
  target("example." .. name) do 
    set_kind("binary")
    add_deps("short-lambda.module")
    add_files("example/" .. name .. ".cpp")
  end
end

make_example("moduleonly")
make_example("forwarding-noexcept")
make_example("fmap")
make_example("new-delete")

target("example.headeronly", {
  kind = "binary",
  deps = "short-lambda.header",
  files = "example/headeronly.cpp" })