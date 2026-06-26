target("tests")
set_kind("binary")
set_default(false)
set_policy("build.c++.modules", true)

add_files("buffer_test.cpp", "color_conversion_test.cpp")

add_deps("weqeqq.image")
add_packages("weqeqq.color", "weqeqq.error", "weqeqq.parallel")
add_packages("weqeqq.test", { components = { "core", "main" } })

if has_config("io") then
	add_files("io_test.cpp")
	add_deps("weqeqq.image.io")
	add_packages("weqeqq.png", "weqeqq.avif")
end

if has_config("processing") then
	add_files("crop_test.cpp", "adjustments_test.cpp", "blending_test.cpp")
	add_deps("weqeqq.image.processing")
end

add_tests("default")
