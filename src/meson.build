#
# Build
#

configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: conf
)

src = files(
  'main.c',
  'application.c',
  'window.c',
  'filerow.c',
  'selectiondialog.c',
  'managementdialog.c',
  'keyrow.c',
  'cryptography.c',
  'threading.c',
  'text.c'
)

internal_inc = include_directories('.')

executable(
          project_exec,
                   src,
   include_directories: [internal_inc],
          dependencies: [adwaita_dep, gtk_dep, gdk_dep, glib_dep, gio_dep, gpgme_dep],
               install: true
)
