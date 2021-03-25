sources = [
  'extension.c',
  'nw-api-impl.h',
  'nw-compat.h',
  'nw-delete-operation.c',
  'nw-delete-operation.h',
  'nw-extension.c',
  'nw-extension.h',
  'nw-fill-operation.c',
  'nw-fill-operation.h',
  'nw-operation-manager.c',
  'nw-operation-manager.h',
  'nw-operation.c',
  'nw-operation.h',
  'nw-path-list.c',
  'nw-path-list.h',
  'nw-progress-dialog.c',
  'nw-progress-dialog.h',
  'nw-type-utils.h'
]

libnemo_wipe = shared_library(
  'nemo-wipe', sources,
  dependencies : deps,
  include_directories : rootdir,
  c_args : [
    '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
    '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
    '-DLOCALEDIR="@0@"'.format(localedir)
  ],
  install : true,
  install_dir : extensiondir
)
