tigervnc_srcdir = join_paths(meson.source_root(), '../..')
tigervnc_builddir = tigervnc_srcdir

cpp = meson.get_compiler('cpp')

vnccommon_deps = [
    declare_dependency(
        dependencies: cpp.find_library('rfb', dirs: join_paths(tigervnc_builddir, 'common/rfb'))),
    declare_dependency(
        dependencies: cpp.find_library('rdr', dirs: join_paths(tigervnc_builddir, 'common/rdr'))),
    declare_dependency(
        dependencies: cpp.find_library('os', dirs: join_paths(tigervnc_builddir, 'common/os'))),
    declare_dependency(
        dependencies: cpp.find_library('network', dirs: join_paths(tigervnc_builddir, 'common/network'))),
    declare_dependency(
        dependencies: cpp.find_library('unixcommon', dirs: join_paths(tigervnc_builddir, 'unix/common'))),
    dependency('zlib'),
    dependency('pam'),
    dependency('gnutls'),
    dependency('nettle'),
    dependency('hogweed'),
    dependency('gmp'),
    dependency('libjpeg'),
]

srcs_vnccommon = [
    'qnum_to_xorgevdev.c',
    'qnum_to_xorgkbd.c',
    'RandrGlue.c',
    'RFBGlue.cc',
    'RFBGlue.h',
    'vncBlockHandler.c',
    'vncBlockHandler.h',
    'vncExt.c',
    'vncExtInit.cc',
    'vncHooks.c',
    'vncInput.c',
    'vncInput.h',
    'vncInputXKB.c',
    'vncSelection.c',
    'vncSelection.h',
    'xorg-version.h',
    'XorgGlue.c',
    'XorgGlue.h',
    'XserverDesktop.cc',
    'XserverDesktop.h',
]

vnccommon_inc = include_directories(
    '../../../../common',
    '../../../common',
    '../../../vncconfig',
)

libvnccommon = static_library(
    'vnccommon',
    srcs_vnccommon,
    include_directories : [inc, vnccommon_inc],
    dependencies: [common_dep, vnccommon_deps],
    install: false,
)

srcs_xvnc = [
    'xvnc.c',
    'buildtime.c',
    '../../fb/fbcmap_mi.c',
    '../../mi/miinitext.c',
    '../../Xi/stubs.c',
]

xvnc_inc = include_directories(
    '../../../../common',
    '../../../common',
)

xvnc = executable(
    'Xvnc',
    srcs_xvnc,
    include_directories : [inc, xvnc_inc],
    dependencies: common_dep,
    c_args: ['-DTIGERVNC', '-DNO_MODULE_EXTS'],
    link_with: [
        libxserver,
        libxserver_fb,
        libxserver_main,
        libxserver_glx,
        libglxvnd,
        libxserver_xi_stubs,
        libxserver_xkb_stubs,
        libvnccommon,
    ],
    install: true,
)

libvnc_inc = include_directories(
    '../xfree86/common',
    '../xfree86/os-support',
    '../xfree86/os-support/bus'
)

libvnc = shared_module(
    'vnc',
    'vncModule.c',
    include_directories : [inc, xvnc_inc, libvnc_inc],
    dependencies: common_dep,
    link_with: libvnccommon,
    install: true,
    install_dir: join_paths(module_dir, 'extensions')
)

install_man(configure_file(
    input: 'Xvnc.man',
    output: 'Xvnc.1',
    configuration: manpage_config,
))
