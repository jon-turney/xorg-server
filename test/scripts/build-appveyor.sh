#!/bin/bash -x

export PREFIX=/usr

git clone --depth 1 git://anongit.freedesktop.org/git/piglit
export PIGLIT_DIR=$(pwd)/piglit

# currently, the 'xts' test set in piglit expects to be pointed at a built copy
# of xts, not an install (which we could package)
XTS_ARCHIVE="xts-0.99.1-20170616git8809db78-$(uname -m).tar.xz"
wget -nv ftp://cygwin.com/pub/cygwinx/${XTS_ARCHIVE}
tar -xf ${XTS_ARCHIVE}
export XTEST_DIR=$(pwd)/xts

cat > "$PIGLIT_DIR"/piglit.conf << _EOF_
[xts]
path=$XTEST_DIR
_EOF_

# set TET_CONFIG to point to the xts config to be used
export TET_CONFIG=$(pwd)/test/tetexec.cfg

# XTS contains tests which rely on being able to set a fontpath containing this
# directory, but non-existent directories are removed from the fontpath by the
# server, so it must exist
mkdir -p /etc/X11/fontpath.d/

# suppress some bleating about SHM
cygserver-config --yes >/dev/null
cygrunsrv -S cygserver

meson setup --prefix=$PREFIX -Dxv=false -Dxf86bigfont=true -Ddmx=true -Dxephyr=true -Dxnest=true -Dxvfb=true -Dxwin=true -Dxorg=true -Dhal=false -Dudev=false -Dpciaccess=false -Dint10=false build/
DESTDIR=$(pwd)/staging ninja -C build/ install
ninja -C build/ test

status=$?

cat build/meson-logs/testlog.txt
cat build/test/piglit-results/xvfb/long-summary

exit $status
