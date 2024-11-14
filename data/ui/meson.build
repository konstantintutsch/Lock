#
# Blueprints
#

blp = files(
  'window.blp',
  'encryptiondialog.blp',
  'keydialog.blp',
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
