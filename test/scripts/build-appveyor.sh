#!/bin/bash -x

export PREFIX=/usr

meson setup --prefix=$PREFIX -Dxv=false -Dxf86bigfont=true -Ddmx=true -Dxephyr=true -Dxnest=true -Dxvfb=true -Dxwin=true -Dxorg=true -Dhal=false -Dudev=false -Dpciaccess=false -Dint10=false build/
meson configure build/
DESTDIR=$(pwd)/staging ninja -C build/ install
ninja -C build/ test
