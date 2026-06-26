target("weqeqq.image.io")
set_kind("$(kind)")

add_files("sources/*.cppm", { public = true })
add_files("sources/*.cpp")

add_deps("weqeqq.image", { public = true })
add_packages("weqeqq.error", { public = true })
add_packages("weqeqq.png", "weqeqq.avif")
