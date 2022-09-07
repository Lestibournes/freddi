freddi_sources = [
  'main.c',
  'freddi-window.c',
  'freddi-application.c',
]

freddi_deps = [
  dependency('libadwaita-1', version: '>= 1.0'),
  dependency('flatpak'),
]

gnome = import('gnome')

freddi_sources += gnome.compile_resources('freddi-resources',
  'freddi.gresource.xml',
  c_name: 'freddi'
)

executable('freddi', freddi_sources,
  dependencies: freddi_deps,
  install: true,
)
