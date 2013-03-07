srcs_wmutil = [
    'icon_convert.c',
]

headers_wmutil = [
    'icon_convert.h',
]

install_headers(
    headers_wmutil,
    subdir: 'XWinWMUtil')

xwin_wmutil = static_library(
    'XWinWMUtil',
    srcs_wmutil,
    c_args: '-DHAVE_XWIN_CONFIG_H',
    include_directories: [inc, include_directories('../')],
    install: true,
)
