project('xpm-pixbuf', 'c',
                version: '1.0.0',
          meson_version: '>=1.0',
                license: 'LGPL-2.1-or-later',
        default_options: [
                           'buildtype=debugoptimized',
                           'c_std=gnu11',
                           'warning_level=3',
                         ],
)

glib_min_ver = '2.78'
glib_sym_ver = 'GLIB_VERSION_@0@'.format(glib_min_ver.replace('.', '_'))

xpm_c_args = [
  '-DG_LOG_DOMAIN="@0@"'.format('Xpm'),
  '-DG_LOG_USE_STRUCTURED',
  '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_sym_ver),
  '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_sym_ver),
]

cc = meson.get_compiler('c')

test_c_args = [
  '-Wdeclaration-after-statement',
  ['-Werror=format-security', '-Werror=format=2'],
  '-Werror=missing-declarations',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Winit-self',
  '-Wmaybe-uninitialized',
  '-Wno-strict-aliasing',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wshadow',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wunused-function',
]

if get_option('buildtype') != 'plain'
  test_c_args += [
    '-fstack-protector-strong',
    '-fno-omit-frame-pointer',
    '-mno-omit-leaf-frame-pointer',
    '-fasynchronous-unwind-tables',
  ]
endif

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    xpm_c_args += arg
  endif
endforeach

glib_dep = dependency('glib-2.0', version: '>= @0@'.format(glib_min_ver))
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.40')

xpm_deps = [
  glib_dep,
  gdk_pixbuf_dep,
]

install_headers('xpm-pixbuf.h')

xpm_lib = library(
                         meson.project_name(),
                         [
                           'xpm-pixbuf.c',
                           'xpm-pixbuf.h',
                           'xpm-colour-table.h',
                         ],
              soversion: 0,
                 c_args: xpm_c_args,
           dependencies: xpm_deps,
                install: true,
  gnu_symbol_visibility: 'hidden',
)

xpm_dep = declare_dependency(
           dependencies: xpm_deps,
              link_with: xpm_lib,
    include_directories: include_directories('.'),
)

meson.override_dependency(meson.project_name(), xpm_dep)

pkg = import('pkgconfig')

pkg.generate(
              libraries: [xpm_lib],
                version: meson.project_version(),
                   name: 'XPM Pixbuf',
               filebase: meson.project_name(),
            description: 'Generate GdkPixbufs for XPM data',
               requires: xpm_deps,
)
