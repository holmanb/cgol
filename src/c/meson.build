project('cgol', 'c',
  meson_version: '>=0.49.0',
  license: 'MIT',
  default_options: [
        'optimization=3'
  ],
  version : '1.0.0')

add_project_arguments([
  '-pedantic',
  '-Wundef',
  '-Wconversion',
  '-Wshadow',
  '-Wlogical-op',
  '-Winit-self',
  '-Wstrict-prototypes',
  '-Wendif-labels',
  '-Woverflow',
  '-Wno-missing-braces',
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter',
  '-g',
], language: 'c')

# validate memory issues
add_test_setup('valgrind',
  exe_wrapper : ['valgrind', '--error-exitcode=1', '--leak-check=full', '--track-origins=yes', '--exit-on-first-error=yes'],
  is_default : true)

# generate flamegraphs w/perf
add_test_setup('perf',
  exe_wrapper : ['../benchmark/perf.sh'],
  is_default : false)

exe = executable('cgol',
  'main.c')
test('valgrind', exe)
