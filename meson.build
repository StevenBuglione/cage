project('cage', 'c',
  version: '0.3.0',
  license: 'MIT',
  meson_version: '>=0.58.1',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'werror=true',
  ],
)

add_project_arguments(
  [
    '-DWLR_USE_UNSTABLE',
    '-Wundef',
    '-Wno-unused-parameter',
  ],
  language: 'c',
)

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : 'c')
endif

cc = meson.get_compiler('c')

is_freebsd = host_machine.system().startswith('freebsd')
if is_freebsd
  add_project_arguments(
    [
      '-Wno-format-extra-args',
      '-Wno-gnu-zero-variadic-macro-arguments',
    ],
    language: 'c'
  )
endif

wlroots        = dependency('wlroots-0.20', fallback: ['wlroots', 'wlroots'])
wayland_server = dependency('wayland-server')
xkbcommon      = dependency('xkbcommon')
math           = cc.find_library('m')

have_xwayland = wlroots.get_variable(pkgconfig: 'have_xwayland', internal: 'have_xwayland') == 'true'

version = '@0@'.format(meson.project_version())
git = find_program('git', native: true, required: false)
if git.found()
  git_commit = run_command([git, 'rev-parse', '--short', 'HEAD'], check: false)
  git_branch = run_command([git, 'rev-parse', '--abbrev-ref', 'HEAD'], check: false)
  if git_commit.returncode() == 0 and git_branch.returncode() == 0
    version = '@0@-@1@ (branch \'@2@\')'.format(
      meson.project_version(),
      git_commit.stdout().strip(),
      git_branch.stdout().strip(),
    )
  endif
endif

conf_data = configuration_data()
conf_data.set10('CAGE_HAS_XWAYLAND', have_xwayland)
conf_data.set_quoted('CAGE_VERSION', version)

scdoc = dependency('scdoc', version: '>=1.9.2', native: true, required: get_option('man-pages'))
if scdoc.found()
  scdoc_prog = find_program(scdoc.get_variable('scdoc'), native: true)
  sh = find_program('sh', native: true)
  mandir = get_option('mandir')
  man_files = [
    'cage.1.scd'
  ]
  foreach filename : man_files
    topic = filename.split('.')[-3].split('/')[-1]
    section = filename.split('.')[-2]
    output = '@0@.@1@'.format(topic, section)

    custom_target(
      output,
      input: filename,
      output: output,
      command: [
        sh, '-c', '@0@ < @INPUT@ > @1@'.format(scdoc_prog.full_path(), output)
      ],
      install: true,
      install_dir: '@0@/man@1@'.format(mandir, section)
    )
  endforeach
endif

cage_sources = [
  'cage.c',
  'idle_inhibit_v1.c',
  'output.c',
  'poc_layout.c',
  'poc_resize.c',
  'seat.c',
  'surface_control_protocol.c',
  'surface_controller.c',
  'surface_registry.c',
  'surface_token_hint.c',
  'surface_view_policy.c',
  'view.c',
  'xdg_shell.c',
  configure_file(input: 'config.h.in',
                 output: 'config.h',
                 configuration: conf_data),
]

if conf_data.get('CAGE_HAS_XWAYLAND', 0) == 1
  cage_sources += 'xwayland.c'
endif

executable(
  meson.project_name(),
  cage_sources,
  dependencies: [
    wayland_server,
    wlroots,
    xkbcommon,
    math,
  ],
  install: true,
)

poc_layout_test = executable(
  'poc-layout-test',
  [
    'poc_layout.c',
    'tests/poc_title_fixture.c',
    'tests/poc_layout_test.c',
  ],
  include_directories: include_directories('.'),
)

test('poc-layout-characterization', poc_layout_test)

poc_layout_socket_test = executable(
  'poc-layout-socket-test',
  [
    'poc_layout.c',
    'poc_layout_socket.c',
    'tests/poc_layout_socket_test.c',
  ],
  include_directories: include_directories('.'),
)

test('poc-layout-socket', poc_layout_socket_test)

poc_layout_controller_test = executable(
  'poc-layout-controller-test',
  [
    'poc_layout.c',
    'poc_layout_controller.c',
    'poc_layout_socket.c',
    'tests/poc_layout_controller_test.c',
  ],
  dependencies: wayland_server,
  include_directories: include_directories('.'),
)

test('poc-layout-controller-lifecycle', poc_layout_controller_test)

poc_resize_test = executable(
  'poc-resize-test',
  [
    'poc_resize.c',
    'tests/poc_resize_test.c',
  ],
  include_directories: include_directories('.'),
)

test('poc-resize-characterization', poc_resize_test)

surface_registry_test = executable(
  'surface-registry-test',
  [
    'surface_registry.c',
    'surface_token_hint.c',
    'tests/surface_registry_test.c',
  ],
  include_directories: include_directories('.'),
)

test('surface-registry', surface_registry_test)

surface_control_protocol_test = executable(
  'surface-control-protocol-test',
  [
    'surface_control_protocol.c',
    'surface_registry.c',
    'tests/surface_control_protocol_test.c',
  ],
  include_directories: include_directories('.'),
)

test('surface-control-protocol', surface_control_protocol_test)

surface_controller_test = executable(
  'surface-controller-test',
  [
    'surface_control_protocol.c',
    'surface_controller.c',
    'surface_registry.c',
    'tests/surface_controller_test.c',
  ],
  dependencies: wayland_server,
  include_directories: include_directories('.'),
)

test('surface-controller-lifecycle', surface_controller_test)

surface_view_policy_test = executable(
  'surface-view-policy-test',
  [
    'surface_registry.c',
    'surface_token_hint.c',
    'surface_view_policy.c',
    'tests/surface_view_policy_test.c',
  ],
  include_directories: include_directories('.'),
)

test('surface-view-policy', surface_view_policy_test)

scene_model_test = executable(
  'scene-model-test',
  [
    'scene_model.c',
    'surface_registry.c',
    'tests/scene_model_test.c',
  ],
  include_directories: include_directories('.'),
)

test('scene-model', scene_model_test)

python = import('python').find_installation()
test(
  'framework-source-policy',
  python,
  args: [files('tests/source_policy_test.py'), meson.project_source_root()],
)

summary = [
  '',
  'Cage @0@'.format(version),
  '',
  '    xwayland: @0@'.format(have_xwayland),
  ''
]
message('\n'.join(summary))
