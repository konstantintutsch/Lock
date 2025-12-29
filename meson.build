#
# Project
#

project(
     'Lock',
            'c',
        version: '1.9.3',
        license: 'MIT',
  meson_version: '>= 1.3'
)
base_id = 'com.konstantintutsch.Lock'

conf = configuration_data()

profile = ''
id_suffix = ''
version_suffix = ''

if get_option('profile') == 'development'
  profile = 'devel'

  id_suffix = '.Devel'
  version_suffix = '-devel'
endif

project_name = meson.project_name()
project_id = base_id + id_suffix
project_root = '/' + project_id.replace('.', '/')
project_version = meson.project_version()
project_build = meson.project_version() + version_suffix
project_exec = project_id

conf.set('project_name', project_name)
conf.set('project_id', project_id)
conf.set('project_root', project_root)
conf.set('project_version', project_version)
conf.set('project_build', project_build)
conf.set('project_exec', project_exec)

#
# Paths
#

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
localedir = prefix / get_option('localedir')

conf.set('bindir', bindir)
conf.set('datadir', datadir)
conf.set('localedir', localedir)

#
# Specific
#

gettext_package = project_id
gresource_file = datadir / project_id / project_id + '.gresource'
ui_resource = project_root / 'ui'

conf.set('gettext_package', gettext_package)
conf.set('gresource_file', gresource_file)
conf.set('ui_resource', ui_resource)

#
# External
#

i18n = import('i18n')
gnome = import('gnome')

adwaita_dep = dependency('libadwaita-1', version: '>=1.8')
gtk_dep = dependency('gtk4', version: '>=4.14')
gdk_dep = dependency('gdk-pixbuf-2.0', version: '>=2.42')
glib_dep = dependency('glib-2.0', version: '>=2.84')
gio_dep = dependency('gio-2.0', version: '>=2.84')

gpgme_required_version = '1.24.0' # compatible and tested up to 2.0.1
conf.set('gpgme_required_version', gpgme_required_version)
gpgme_dep = dependency('gpgme', version: '>=' + gpgme_required_version)

#
# Subdirectories
#

internal_inc = include_directories('.')

subdir('po')
subdir('data')
subdir('src')

#
# GNOME
#
gnome.post_install(
  gtk_update_icon_cache: true,
  update_desktop_database: true
)
