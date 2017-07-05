#!/bin/bash

DEPS_DIR="$(pwd)/deps"


# lua53
function install_lua53() {
    mkdir -p "$DEPS_DIR/lua53" 2>/dev/null
    cd "$DEPS_DIR"

    curl -R -O http://www.lua.org/ftp/lua-5.3.4.tar.gz
    tar zxf lua-5.3.4.tar.gz
    cd lua-5.3.4

    patch -p1 -i ../../liblua.so.patch
    make MYCFLAGS="$CFLAGS -fPIC -DLUA_COMPAT_5_2 -DLUA_COMPAT_5_1" \
        MYLDFLAGS="$LDFLAGS" \
        linux

    make INSTALL_TOP="$DEPS_DIR/lua53" \
        TO_LIB="liblua.a liblua.so liblua.so.5.3" \
        install

    cd "$DEPS_DIR"
    rm -rf lua-5.3.4 lua-5.3.4.tar.gz
}


# luajit
function install_luajit() {
    mkdir -p "$DEPS_DIR/luajit" 2>/dev/null
    cd "$DEPS_DIR"

    curl -R -O http://luajit.org/download/LuaJIT-2.0.4.tar.gz
    tar zxf LuaJIT-2.0.4.tar.gz
    cd LuaJIT-2.0.4

    make amalg PREFIX="$DEPS_DIR/luajit"
    make install PREFIX="$DEPS_DIR/luajit"

    cd "$DEPS_DIR"
    rm -rf LuaJIT-2.0.4 LuaJIT-2.0.4.tar.gz
}


install_lua53
install_luajit
