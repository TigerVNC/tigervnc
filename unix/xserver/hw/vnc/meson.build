add_languages('cpp')

tigervnc_srcdir = get_option('tigervnc_srcdir')
tigervnc_builddir = get_option('tigervnc_builddir')

if tigervnc_srcdir == ''
    tigervnc_srcdir = '../../../..'
endif
if tigervnc_builddir == ''
    tigervnc_builddir = '../../../..'
endif

tigervnc_cmake_module_path = []
tigervnc_cmake_module_path += [ tigervnc_builddir / 'common/core' ]
tigervnc_cmake_module_path += [ tigervnc_builddir / 'common/rdr' ]
tigervnc_cmake_module_path += [ tigervnc_builddir / 'common/network' ]
tigervnc_cmake_module_path += [ tigervnc_builddir / 'common/rfb' ]

core_dep = dependency('core', method: 'cmake',
                      cmake_module_path: tigervnc_cmake_module_path)
rdr_dep = dependency('rdr', method: 'cmake',
                     cmake_module_path: tigervnc_cmake_module_path)
network_dep = dependency('network', method: 'cmake',
                         cmake_module_path: tigervnc_cmake_module_path)
rfb_dep = dependency('rfb', method: 'cmake',
                     cmake_module_path: tigervnc_cmake_module_path)
rfbserver_dep = dependency('rfbserver', method: 'cmake',
                           cmake_module_path: tigervnc_cmake_module_path)

tigervnc_cmake_module_path += [ tigervnc_builddir / 'unix/common' ]

unixcommon_dep = dependency('unixcommon', method: 'cmake',
                            cmake_module_path: tigervnc_cmake_module_path)

libvnccommon_inc = include_directories(
    tigervnc_builddir,
    tigervnc_srcdir / 'unix/vncconfig',
)

libvnccommon_deps = [
    unixcommon_dep,
    rfbserver_dep,
    rfb_dep,
    network_dep,
    rdr_dep,
    core_dep,
]

srcs_libvnccommon = [
    'qnum_to_xorgevdev.c',
    'qnum_to_xorgkbd.c',
    'RandrGlue.c',
    'RFBGlue.cc',
    'XorgGlue.c',
    'XserverDesktop.cc',
    'vncBlockHandler.c',
    'vncExt.c',
    'vncExtInit.cc',
    'vncHooks.c',
    'vncInput.c',
    'vncInputXKB.c',
    'vncPresent.c',
    'vncSelection.c',
]

libvnccommon = static_library(
    'vnccommon',
    srcs_libvnccommon,
    include_directories: [inc, libvnccommon_inc],
    dependencies: [common_dep, libvnccommon_deps],
    override_options : ['cpp_std=c++11'],
)

srcs_xvnc = [
    'xvnc.c',
    '../../mi/miinitext.c',
    '../../mi/miinitext.h',
]

if build_dri3
    srcs_xvnc += [
        'vncDRI3.c',
        'vncDRI3Draw.c',
    ]
endif

xvnc_inc = include_directories(
    tigervnc_builddir,
)

xvnc_deps = [
    libxcvt_dep,
    gbm_dep,
    unixcommon_dep,
]

xvnc_server = executable(
    'Xvnc',
    srcs_xvnc,
    include_directories: [inc, xvnc_inc],
    dependencies: [common_dep, xvnc_deps],
    link_with: [
        libxserver_fb,
        libxserver,
        libxserver_xkb_stubs,
        libxserver_xi_stubs,
        libxserver_glx,
        libglxvnd,
        libvnccommon,
    ],
    install: true,
)

# FIXME: We don't actually replace anything in the file, but we need
#        it to be renamed to keep Meson happy
install_man(configure_file(
    input: 'Xvnc.man',
    output: 'Xvnc.1',
    configuration: manpage_config,
))

srcs_libvnc = [
    'vncModule.c',
]

libvnc_inc = include_directories(
    '../xfree86/common',
)

libvnc_deps = [
    unixcommon_dep,
]

shared_module(
    'vnc',
    srcs_libvnc,
    include_directories: [inc, libvnc_inc],
    dependencies: [common_dep, libvnc_deps],
    link_with: [
        libvnccommon,
    ],
    install: true,
    install_dir: module_dir / 'extensions',
)
