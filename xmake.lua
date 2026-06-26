set_project("weqeqq.image")
set_version("0.3.0")

add_rules("mode.release", "mode.debug")
add_rules("plugin.compile_commands.autoupdate")
set_policy("build.c++.modules.std", false)

set_languages("c++23")

add_repositories("weqeqq.repo https://github.com/weqeqq/xmake-repo.git")

option("io")
set_default(true)
set_showmenu(true)
set_description("Build the weqeqq.image.io component")
option_end()

option("processing")
set_default(true)
set_showmenu(true)
set_description("Build the weqeqq.image.processing component")
option_end()

option("simd")
set_default(true)
set_showmenu(true)
set_description("Enable SIMD in the weqeqq.image.processing component")
option_end()

option("tests")
set_default(false)
set_showmenu(true)
set_description("Build tests")
option_end()

add_requires("weqeqq.error ~0.2.0")
add_requires("weqeqq.color ~0.1.0")
add_requires("weqeqq.parallel")

if has_config("io") then
	add_requires("weqeqq.png ~0.1.0")
	add_requires("weqeqq.avif ~0.1.0")
end

if has_config("tests") then
	add_requires("weqeqq.test ~0.3.6")
end

includes("image")

if has_config("io") then
	includes("io")
end

if has_config("processing") then
	includes("processing")
end

if has_config("tests") then
	includes("tests")
end
