test_env = environment()
test_env.set('G_DEBUG', 'fatal-warnings')
inc = include_directories('../src')
# The TOD library leaves fp_* to the host application; test programs have to provide them.
test_deps = deps + [dependency('libfprint-2')]

test('quality',
  executable('test-quality', 'test-quality.c', 'synth.c', pure_sources,
    include_directories: inc, dependencies: dependency('glib-2.0')),
  env: test_env)

test('guard',
  executable('test-guard', 'test-guard.c', pure_sources,
    include_directories: inc, dependencies: dependency('glib-2.0')),
  env: test_env)

test('errors',
  executable('test-errors', 'test-errors.c', '../src/elan0c4d-error.c', '../src/elan0c4d-match-stub.c',
    include_directories: inc, dependencies: test_deps),
  env: test_env)

test('module',
  executable('test-module', 'test-module.c', include_directories: inc, dependencies: test_deps),
  args: [driver.full_path()], env: test_env, depends: driver)

# The real driver code against a scripted sensor: the executable provides fpi_usb_transfer_submit.
sim_sources = files('sim-image.c', 'sim-run.c', 'sim-sensor.c', 'sim-transport.c', 'synth.c')
sim_driver = driver_sources + sim_sources
foreach name : ['open', 'capture', 'limits', 'stop']
  test('sim-' + name,
    executable('test-sim-' + name, 'test-sim-' + name + '.c', sim_driver,
      include_directories: inc, dependencies: test_deps),
    env: test_env, timeout: 60)
endforeach

# nm -D must list the entry point the TOD loader looks for.
test('exports-entry-point', find_program('sh'),
  args: ['-c', 'nm -D --defined-only "$1" | grep -q " T fpi_tod_shared_driver_get_type$"', 'sh',
         driver.full_path()],
  depends: driver)

# RUNIAP (42 01) and WDTRST (40 27) must not exist as byte lists in the sources.
test('no-forbidden-bytes', find_program('sh'),
  args: ['-c', '! grep -rEn "0x42, *0x01|0x40, *0x27" "$1"', 'sh', meson.project_source_root() / 'src'])
