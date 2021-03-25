project('nemo-wipe', 'c', version : '4.8.0')

conf = configuration_data()

gconf = dependency('gconf', version : '>= 2.0', required : false)
gio = dependency('gio-2.0', version : '>= 2.26')
giounix = dependency('gio-unix-2.0', version : '>= 2.0', required : false)
glib = dependency('glib-2.0', version : '>= 2.1')
gsecuredelete = dependency('gsecuredelete', version : '>= 0.3')
gtk = dependency('gtk+-3.0', version : '>= 3.2')
libnemo = dependency('libnemo-extension', version : '>= 3.0')

deps = [gio, glib, gsecuredelete, gtk, libnemo]

if gconf.found()
  conf.set('HAVE_GCONF', 1)
  deps += [gconf]
endif

if giounix.found()
  conf.set('HAVE_GIO_UNIX', 1)
  deps += [giounix]
endif

extensiondir = libnemo.get_pkgconfig_variable('extensiondir')
localedir = join_paths(get_option('localedir'))
rootdir = include_directories('.')

conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('LOCALEDIR', localedir)
conf.set_quoted('VERSION', meson.project_version())
configure_file(output : 'config.h',
               configuration : conf)

subdir('src')
