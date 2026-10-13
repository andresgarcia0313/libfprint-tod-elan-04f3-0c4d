project('tod-elan0c4d', 'c',
  version: '0.1.0',
  license: 'LGPL-2.1-or-later',
  meson_version: '>= 1.0',
  default_options: ['c_std=gnu11', 'warning_level=1', 'werror=true'])

add_project_arguments('-Wall', '-Werror', '-DG_LOG_DOMAIN="libfprint-elan0c4d"', language: 'c')

deps = [
  dependency('libfprint-2-tod-1'),
  dependency('gusb'),
  dependency('gio-2.0'),
  dependency('gmodule-2.0'),
]

# Pure logic (no libfprint calls at run time): shared by the driver and the tests.
pure_sources = files('src/elan0c4d-quality.c', 'src/elan0c4d-guard.c', 'src/elan0c4d-codes.c')

driver_sources = pure_sources + files(
  'src/elan0c4d-actions.c', 'src/elan0c4d-arm.c', 'src/elan0c4d-calibrate.c',
  'src/elan0c4d-capture.c', 'src/elan0c4d-cmd.c', 'src/elan0c4d-driver.c',
  'src/elan0c4d-error.c', 'src/elan0c4d-frame.c', 'src/elan0c4d-image.c',
  'src/elan0c4d-init.c', 'src/elan0c4d-lifecycle.c', 'src/elan0c4d-log.c',
  'src/elan0c4d-prepare.c', 'src/elan0c4d-print.c', 'src/elan0c4d-recover.c',
  'src/elan0c4d-retry.c', 'src/elan0c4d-script.c', 'src/elan0c4d-try.c',
  'src/elan0c4d-wait.c', 'src/elan0c4d-wait-read.c',
  # The comparator is not decided: swap this file for a real implementation.
  'src/elan0c4d-match-stub.c')

# The TOD loader opens lib*.so files and looks for fpi_tod_shared_driver_get_type.
driver = shared_module('elan0c4d-tod', driver_sources,
  name_prefix: 'lib',
  dependencies: deps,
  gnu_symbol_visibility: 'hidden',
  install: true,
  install_dir: get_option('libdir') / 'libfprint-2' / 'tod-1')

subdir('tests')
