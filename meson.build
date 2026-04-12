project(
  'wqimage',
  ['cpp'],
  meson_version: '>=1.3.0',
  version: '0.2.0',
  default_options: ['cpp_std=c++23', 'warning_level=3'],
)

is_top_level = not meson.is_subproject()
cpp = meson.get_compiler('cpp')
simd_enabled = get_option('simd').enabled()
tests_enabled = get_option('tests').enabled()
install_enabled = is_top_level and get_option('install')
install_bool = install_enabled ? 'true' : 'false'
install_feature = install_enabled ? 'enabled' : 'disabled'

wqimage_includes = include_directories('headers', 'sources')
wqimage_public_includes = include_directories('headers')
wqimage_public_args = []

wqpng_dep = dependency(
  'wqpng',
  fallback: ['wqpng', 'wqpng_dep'],
  default_options: ['install=' + install_bool],
  required: true,
)
wqcolor_dep = dependency(
  'wqcolor',
  version: '>=0.1.1',
  fallback: ['wqcolor', 'wqcolor_dep'],
  default_options: ['install=' + install_feature],
  required: true,
)
wqfile_dep = dependency(
  'wqfile',
  fallback: ['wqfile', 'wqfile_dep'],
  default_options: ['install=' + install_feature],
  required: true,
)
wqparallel_dep = dependency(
  'wqparallel',
  version: '>=0.2.2',
  fallback: ['wqparallel', 'wqparallel_dep'],
  default_options: ['tests=disabled', 'install=' + install_bool],
  required: true,
)
wqerror_dep = dependency(
  'wqerror',
  version: '>=0.1.0',
  fallback: ['wqerror', 'wqerror_dep'],
  default_options: ['tests=disabled', 'install=' + install_feature],
  required: true,
)

dependencies = [wqpng_dep, wqcolor_dep, wqfile_dep, wqparallel_dep, wqerror_dep]
compile_flags = []
wqimage_link_args = []
wqimage_pkg_requires = ['wqcolor >= 0.1.1', 'wqparallel >= 0.2.2', 'wqerror >= 0.1.0']
wqimage_pkg_private_requires = ['wqpng', 'wqfile']

if get_option('default_library') != 'static'
  version_script_arg = cpp.first_supported_link_argument('-Wl,--version-script=' + meson.current_source_dir() / 'wqimage.version')
  if version_script_arg.length() > 0
    wqimage_link_args += version_script_arg
  endif
endif

if simd_enabled
  hwy_dep = dependency(
    'libhwy',
    fallback: ['highway', 'hwy_dep'],
    default_options: [
      'contrib=disabled',
      'examples=disabled',
      'test_standalone=true',
      'tests=disabled',
    ],
    required: true,
  )
  dependencies += [hwy_dep]
  wqimage_pkg_private_requires += ['libhwy']
endif

compile_flags += ['-DWQIMAGE_SIMD=' + (simd_enabled ? '1' : '0')]
compile_flags += ['-DWQIMAGE_DEBUG=' + (get_option('debug') ? '1' : '0')]
wqimage_public_args += compile_flags
wqimage_private_args = compile_flags + ['-fvisibility-inlines-hidden']

if get_option('default_library') == 'static'
  wqimage_public_args += ['-DWQIMAGE_STATIC_DEFINE']
endif

wqimage_lib = library(
  'wqimage',
  'sources/color_conversion.cc',
  'sources/crop.cc',
  'sources/blending.cc',
  'sources/decoding.cc',
  'sources/encoding.cc',
  include_directories: wqimage_includes,
  dependencies: dependencies,
  cpp_args: wqimage_private_args,
  cpp_shared_args: ['-DWQIMAGE_EXPORTS'],
  cpp_static_args: ['-DWQIMAGE_STATIC_DEFINE'],
  gnu_symbol_visibility: 'hidden',
  link_args: wqimage_link_args,
  install: install_enabled,
)

wqimage_dep = declare_dependency(
  include_directories: wqimage_public_includes,
  link_with: wqimage_lib,
  compile_args: wqimage_public_args,
  dependencies: dependencies,
)

meson.override_dependency('wqimage', wqimage_dep)

if install_enabled
  install_headers(
    'headers/weqeqq/image/blending.h',
    'headers/weqeqq/image/buffer.h',
    'headers/weqeqq/image/color_conversion.h',
    'headers/weqeqq/image/config.h',
    'headers/weqeqq/image/crop.h',
    'headers/weqeqq/image/decoding.h',
    'headers/weqeqq/image/encoding.h',
    'headers/weqeqq/image/error.h',
    'headers/weqeqq/image/export.h',
    'headers/weqeqq/image.h',
    subdir: 'weqeqq/image',
  )

  pkgconfig = import('pkgconfig')
  pkgconfig.generate(
    libraries: ['-L${libdir}', '-lwqimage'],
    name: 'wqimage',
    description: 'Image processing library for C++23',
    extra_cflags: wqimage_public_args,
    requires: wqimage_pkg_requires,
    requires_private: wqimage_pkg_private_requires,
    subdirs: '.',
    version: meson.project_version(),
  )
endif

if tests_enabled
  gtest_dep = dependency('gtest', fallback: ['gtest', 'gtest_dep'], required: true)
  gtest_main_dep = dependency(
    'gtest_main',
    fallback: ['gtest', 'gtest_main_dep'],
    required: true,
  )

  run_tests = executable(
    'run-tests',
    'tests/blending_test.cc',
    'tests/buffer_test.cc',
    'tests/codec_error_test.cc',
    'tests/crop_test.cc',
    dependencies: [wqimage_dep, gtest_dep, gtest_main_dep],
  )

  test('wqimage', run_tests, protocol: 'gtest')
endif
