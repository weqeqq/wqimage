add_repositories("weqeqq.repo https://github.com/weqeqq/xmake-repo.git")

option("simd")
set_default(false)
set_description("Enable SIMD")
option_end()

if has_config("simd") and is_config("simd", true) then
	add_requires("highway")
end

target("weqeqq.image.processing.simd")
set_kind("object")
set_policy("build.c++.modules", true)

add_files("sources/**.cpp")
add_includedirs("sources")
add_includedirs("headers", { public = true })

if is_mode("debug") then
	add_defines("WQIMAGE_DEBUG=1")
end

if is_kind("static") then
	add_defines("WQIMAGE_STATIC_DEFINE")
end

if has_config("simd") and is_config("simd", true) then
	add_defines("WQIMAGE_SIMD")
	add_packages("highway")
end
