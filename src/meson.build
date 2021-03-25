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
  install : true,
  install_dir : extensiondir
)
