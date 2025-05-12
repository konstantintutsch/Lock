#
# Subdirectories
#

subdir('ui')
subdir('icons')

#
# Desktop Entry
#

desktop_file = i18n.merge_file(
  input: configure_file(
    input: base_id + '.desktop.in.in',
    output: project_id + '.desktop.in',
    configuration: conf
  ),
  output: project_id + '.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: datadir / 'applications'
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test('Validate desktop file', desktop_file_validate, args: [desktop_file])
endif

#
# Appstream
#

appstream_file = i18n.merge_file(
  input: configure_file(
    input: base_id + '.metainfo.xml.in.in',
    output: project_id + '.metainfo.xml.in',
    configuration: conf
  ),
  output: project_id + '.metainfo.xml',
  type: 'xml',
  po_dir: '../po',
  install: true,
  install_dir: datadir / 'metainfo'
)

appstreamcli = find_program('appstreamcli', required: false)
if desktop_file_validate.found()
  test('Validate appstream file', appstreamcli, args: ['validate', '--no-net', appstream_file])
endif

#
# Resources
#

resources = configure_file(
  input: base_id + '.gresource.xml.in',
  output: project_id + '.gresource.xml',
  configuration: conf
)

gnome.compile_resources(
  project_id,
  resources,
  c_name: project_id,
  dependencies: [appstream_file, ui],
  gresource_bundle: true,
  install: true,
  install_dir: datadir / project_id
)

