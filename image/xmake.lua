target("weqeqq.image")
set_kind("$(kind)")

add_files("sources/*.cppm", { public = true })

add_packages("weqeqq.color", "weqeqq.error", "weqeqq.parallel", { public = true })
