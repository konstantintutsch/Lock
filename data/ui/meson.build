#
# Blueprints
#

blp = files(
  'shortcuts.blp',
  'window.blp',
  'selectiondialog.blp',
  'managementdialog.blp',
  'keyrow.blp'
)

ui = custom_target(
  'ui',
  input: blp,
  output: '.',
  command: [
    find_program('blueprint-compiler'), 'batch-compile', '@OUTPUT@', '@CURRENT_SOURCE_DIR@', '@INPUT@'
  ]
)
