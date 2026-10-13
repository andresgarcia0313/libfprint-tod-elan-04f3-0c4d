test_env = environment()
test_env.set('G_DEBUG', 'fatal-warnings')
inc = include_directories('../src')
# The TOD library leaves fp_* to the host application; test programs have to provide them.
test_deps = deps + [dependency('libfprint-2'), opencv_dep]
pure_deps = [dependency('glib-2.0'), m_dep]

test('quality',
  executable('test-quality', 'test-quality.c', 'synth.c', pure_sources,
    include_directories: inc, dependencies: pure_deps),
  env: test_env)

test('guard',
  executable('test-guard', 'test-guard.c', pure_sources,
    include_directories: inc, dependencies: pure_deps),
  env: test_env)

test('errors',
  executable('test-errors', 'test-errors.c', '../src/elan0c4d-error.c', '../src/elan0c4d-match-stub.c',
    include_directories: inc, dependencies: test_deps),
  env: test_env)

test('normalize',
  executable('test-normalize', 'test-normalize.c', 'synth.c', pure_sources,
    include_directories: inc, dependencies: pure_deps),
  env: test_env)

match_test_sources = files('synth.c', 'synth-texture.c', 'test-match-enroll.c') + match_sources + pure_sources
test('match',
  executable('test-match', 'test-match.c', match_test_sources,
    include_directories: inc, dependencies: test_deps),
  env: test_env, timeout: 60)

test('match-data',
  executable('test-match-data', 'test-match-data.c', match_test_sources,
    include_directories: inc, dependencies: test_deps),
  env: test_env, timeout: 60)

test('print',
  executable('test-print', 'test-print.c', '../src/elan0c4d-print.c',
    include_directories: inc, dependencies: test_deps),
  env: test_env)

test('module',
  executable('test-module', 'test-module.c', include_directories: inc, dependencies: test_deps),
  args: [driver.full_path()], env: test_env, depends: driver)

# The real driver code against a scripted sensor: the executable provides fpi_usb_transfer_submit.
sim_sources = files('sim-image.c', 'sim-progress.c', 'sim-run.c', 'sim-sensor.c', 'sim-touches.c',
  'sim-transport.c', 'synth.c')
sim_driver = driver_sources + sim_sources
foreach name : ['open', 'capture', 'limits', 'stop', 'enroll', 'lift']
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

# Local measurement on real touches (not a test: needs captures that never leave the machine).
executable('match-eval', 'real/match-eval.c', match_sources, pure_sources,
  include_directories: inc, dependencies: test_deps, build_by_default: false)
