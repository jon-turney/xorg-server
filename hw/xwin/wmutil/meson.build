srcs_wmutil = [
    'cursor_convert.c',
    'icon_convert.c',
    'mouse.c',
]

headers_wmutil = [
    'cursor_convert.h',
    'icon_convert.h',
    'mouse.h',
]

install_headers(
    headers_wmutil,
    subdir: 'XWinWMUtil')

xwin_wmutil = static_library(
    'XWinWMUtil',
    srcs_wmutil,
    c_args: '-DHAVE_XWIN_CONFIG_H',
    include_directories: [inc, include_directories('../')],
    dependencies: pixman_dep,
    install: true,
)
