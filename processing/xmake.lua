includes("simd")

target("weqeqq.image.processing")
set_kind("$(kind)")

add_files("sources/*.cppm", { public = true })

add_headerfiles("simd/headers/(weqeqq/**.h)")

add_deps("weqeqq.image", { public = true })
add_deps("weqeqq.image.processing.simd")
add_packages("weqeqq.parallel", "weqeqq.error", { public = true })
