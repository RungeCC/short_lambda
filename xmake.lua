set_project("short-lambda")
set_version("0.0.1")
set_allowedmodes("debug", "release")

set_warnings("allextra", "error")

add_rules("mode.release", "mode.debug")

rule("set-flags")
on_config(function(target)
    if target:has_tool("cxx", "cl") then
        target:set("languages", "c++latest")
        target:add("cxflags", "/Zc:__cplusplus")
    else
        if target:has_tool("cxx", "clang", "gcc") then
            target:add("cxflags", "-std=c++26")
        else
            assert(false)
        end
    end
    target:add("cxflags", "clang::-Wno-experimental-header-units")
    target:add("cxflags", "clang::-Wno-c++20-compat")
    target:add("cxflags", "clang::-xc++")
end)
rule_end()

add_rules("set-flags")

add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})

option("enable_module")
do
    set_default(false)
    set_showmenu(true)
end
option_end()

if has_config("enable_module") then
    target("short-lambda.module")
    do
        set_kind("moduleonly")
        add_files("module/short-lambda.cppm", {public = true})
        add_headerfiles("module/*.hpp")
        set_policy("build.c++.modules", true)
    end
    target_end()

    function make_example(name)
        target("example." .. name)
        do
            set_kind("binary")
            add_deps("short-lambda.module")
            add_files("example/" .. name .. ".cpp")
        end
        target_end()
    end

    make_example("moduleonly")
    make_example("forwarding-noexcept")
    make_example("fmap")
    make_example("new-delete")
    make_example("tuple")
end

target("short-lambda.header")
do
    set_kind("headeronly")
    add_includedirs("include", {interface = true})
    add_headerfiles("include/(*.hpp)")
end
target_end()

target("example.headeronly")
do
    set_kind("binary")
    add_deps("short-lambda.header")
    add_files("example/headeronly.cpp")
end
target_end()
