#!/bin/bash

DEPS_DIR="$(pwd)/deps"

mkdir "$DEPS_DIR/lua53" 2>/dev/null
cd "$DEPS_DIR"

curl -R -O http://www.lua.org/ftp/lua-5.3.4.tar.gz
tar zxf lua-5.3.4.tar.gz
cd lua-5.3.4

patch -p1 -i ../liblua.so.patch
make MYCFLAGS="$CFLAGS -fPIC -DLUA_COMPAT_5_2 -DLUA_COMPAT_5_1" \
    MYLDFLAGS="$LDFLAGS" \
    linux

make INSTALL_TOP="$DEPS_DIR/lua53" \
    TO_LIB="liblua.a liblua.so.5.3" \
    install

cd "$DEPS_DIR"
rm -rf lua-5.3.4 lua-5.3.4.tar.gz
