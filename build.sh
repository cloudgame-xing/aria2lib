#!/usr/bin/env bash
# Build aria2 and aria2_c_api for a given OS and arch.
# Usage: build.sh <os> <arch>
# Example: build.sh windows x64
# Requires: VERSION_NAME in environment (for tarball names).
# For windows-*: expects LLVM_MINGW_DIR set (by workflow after extracting llvm-mingw).
# Requires: MUSL_TOOLCHAIN_DIR set (by workflow after extracting musl toolchain).
# For android: expects NDK extracted under build/android-ndk-*.

set -e

OS="${1:?Usage: build.sh <os> <arch>}"
ARCH="${2:?Usage: build.sh <os> <arch>}"
VERSION_NAME="${VERSION_NAME:?VERSION_NAME must be set}"

ROOT_DIR="${PWD}"
BUILD_DIR="${ROOT_DIR}/build"
DEPS_DIR="${BUILD_DIR}/deps"
OUT_DIR="${ROOT_DIR}/out"
PREFIX="${DEPS_DIR}/out"

case "$(uname -s)" in
  Darwin) NPROC=$(sysctl -n hw.ncpu) ;;
  *)      NPROC=$(nproc 2>/dev/null || echo 2) ;;
esac

build_windows_mingw() {
  local host="$1"
  local suffix="$2"
  local cmake_preset_debug="$3"
  local cmake_preset_release="$4"
  local apply_patch="${5:-}"

  export LLVM_MINGW="${LLVM_MINGW_DIR:?LLVM_MINGW_DIR not set}"
  export PATH="$LLVM_MINGW/bin:$PATH"
  export HOST="$host"
  export CC=$HOST-clang
  export CXX=$HOST-clang++
  export AR=llvm-ar
  export RANLIB=llvm-ranlib
  export STRIP=llvm-strip
  export CFLAGS="-O2 -fno-gnu-tm"
  export CXXFLAGS="-O2 -fno-gnu-tm"
  export LDFLAGS="-L$PREFIX/lib"

  mkdir -p "$DEPS_DIR" "$PREFIX" "$OUT_DIR"
  cd "$DEPS_DIR"

  export ZLIB_VERSION=1.3.1
  export ZLIB_ARCHIVE=zlib-$ZLIB_VERSION.tar.gz
  export ZLIB_URI=https://github.com/madler/zlib/releases/download/v1.3.1/$ZLIB_ARCHIVE
  export LIBEXPAT_VERSION=2.5.0
  export LIBEXPAT_ARCHIVE=expat-$LIBEXPAT_VERSION.tar.bz2
  export LIBEXPAT_URI=https://github.com/libexpat/libexpat/releases/download/R_2_5_0/$LIBEXPAT_ARCHIVE
  export GMP_VERSION=6.3.0
  export GMP_ARCHIVE=gmp-$GMP_VERSION.tar.xz
  export GMP_URI=https://ftpmirror.gnu.org/gmp/$GMP_ARCHIVE
  export SQLITE_VERSION=3430100
  export SQLITE_ARCHIVE=sqlite-autoconf-$SQLITE_VERSION.tar.gz
  export SQLITE_URI=https://www.sqlite.org/2023/$SQLITE_ARCHIVE
  export CARES_VERSION=1.21.0
  export CARES_ARCHIVE=c-ares-$CARES_VERSION.tar.gz
  export CARES_URI=https://github.com/c-ares/c-ares/releases/download/cares-1_21_0/$CARES_ARCHIVE
  export LIBSSH2_VERSION=1.11.0
  export LIBSSH2_ARCHIVE=libssh2-$LIBSSH2_VERSION.tar.bz2
  export LIBSSH2_URI=https://libssh2.org/download/$LIBSSH2_ARCHIVE

  echo "-----build zlib-----"
  curl -L -O $ZLIB_URI && tar xf $ZLIB_ARCHIVE && rm $ZLIB_ARCHIVE
  pushd zlib-$ZLIB_VERSION
  CC=$CC AR=$AR RANLIB=$RANLIB STRIP=$STRIP LD=$CC ./configure --prefix=$PREFIX --libdir=$PREFIX/lib --includedir=$PREFIX/include --static
  make -j$NPROC install
  popd

  echo "-----build libexpat-----"
  curl -L -O $LIBEXPAT_URI && tar xf $LIBEXPAT_ARCHIVE && rm $LIBEXPAT_ARCHIVE
  pushd expat-$LIBEXPAT_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared
  make -j$NPROC install
  popd

  echo "-----build gmp-----"
  curl -L -O $GMP_URI && tar xf $GMP_ARCHIVE && rm $GMP_ARCHIVE
  pushd gmp-$GMP_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared --disable-cxx --enable-static
  make -j$NPROC install
  popd

  echo "-----build sqlite3-----"
  curl -L -O $SQLITE_URI && tar xf $SQLITE_ARCHIVE && rm $SQLITE_ARCHIVE
  pushd sqlite-autoconf-$SQLITE_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared --enable-static
  make -j$NPROC install
  popd

  echo "-----build c-ares-----"
  curl -L -O $CARES_URI && tar xf $CARES_ARCHIVE && rm $CARES_ARCHIVE
  pushd c-ares-$CARES_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared --without-random ac_cv_func_if_indextoname=no LIBS="-lws2_32"
  make -j$NPROC install
  popd

  echo "-----build libssh2 (wincng)-----"
  curl -L -O $LIBSSH2_URI && tar xf $LIBSSH2_ARCHIVE && rm $LIBSSH2_ARCHIVE
  pushd libssh2-$LIBSSH2_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared --with-crypto=wincng LIBS="-lws2_32"
  make -j$NPROC install
  popd

  echo "-----build aria2-----"
  cd "$ROOT_DIR/aria2"
  if [[ -n "$apply_patch" ]]; then
    patch -p1 < "$ROOT_DIR/fix_mingw32_size_max.diff"
  fi
  autoreconf -i
  ./configure \
    --prefix=$OUT_DIR/aria2 \
    --host=$HOST \
    --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) \
    --disable-nls \
    --without-gnutls \
    --without-openssl \
    --with-wintls \
    --with-sqlite3 \
    --without-libxml2 \
    --with-libexpat \
    --with-libcares \
    --with-libz \
    --with-libgmp \
    --with-libssh2 \
    --without-libgcrypt \
    --without-libnettle \
    --enable-libaria2 \
    --enable-static \
    --disable-shared \
    CPPFLAGS="-I$PREFIX/include" \
    LDFLAGS="-L$PREFIX/lib" \
    PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig" \
    ARIA2_STATIC=yes
  make -j$NPROC
  make install

  cd "$ROOT_DIR"
  tar -czvf "aria2-${suffix}-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2

  echo "-----build aria2_c_api-----"
  export LLVM_MINGW="$LLVM_MINGW_DIR"
  export OUT_DIR="$ROOT_DIR/out"
  cmake --preset "$cmake_preset_debug"
  cmake --build --preset "$cmake_preset_debug"
  cmake --install build/Debug
  cmake --preset "$cmake_preset_release"
  cmake --build --preset "$cmake_preset_release"
  cmake --install build/Release --strip
  tar -czvf "aria2_c_api-${suffix}-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2lib
}

build_linux_x64() {
  build_linux_musl x86_64-linux-musl linux-x64
}

build_linux_arm64() {
  build_linux_musl aarch64-linux-musl linux-arm64
}

build_linux_musl() {
  local HOST="$1"
  local SUFFIX="$2"
  export MUSL_TOOLCHAIN_DIR="${MUSL_TOOLCHAIN_DIR:?MUSL_TOOLCHAIN_DIR not set}"
  export PATH="$MUSL_TOOLCHAIN_DIR/bin:$PATH"
  export CC=$HOST-gcc
  export CXX=$HOST-g++
  export AR=$HOST-ar
  export RANLIB=$HOST-ranlib
  export STRIP=$HOST-strip
  export CFLAGS="-O2 -fPIC -fno-gnu-tm"
  export CXXFLAGS="-O2 -fPIC -fno-gnu-tm"
  mkdir -p "$DEPS_DIR" "$PREFIX" "$OUT_DIR"
  cd "$DEPS_DIR"

  export OPENSSL_VERSION=1.1.1w
  export OPENSSL_ARCHIVE=openssl-$OPENSSL_VERSION.tar.gz
  export OPENSSL_URI=https://www.openssl.org/source/$OPENSSL_ARCHIVE
  export ZLIB_VERSION=1.3.1
  export ZLIB_ARCHIVE=zlib-$ZLIB_VERSION.tar.gz
  export ZLIB_URI=https://github.com/madler/zlib/releases/download/v1.3.1/$ZLIB_ARCHIVE
  export LIBEXPAT_VERSION=2.5.0
  export LIBEXPAT_ARCHIVE=expat-$LIBEXPAT_VERSION.tar.bz2
  export LIBEXPAT_URI=https://github.com/libexpat/libexpat/releases/download/R_2_5_0/$LIBEXPAT_ARCHIVE
  export CARES_VERSION=1.21.0
  export CARES_ARCHIVE=c-ares-$CARES_VERSION.tar.gz
  export CARES_URI=https://github.com/c-ares/c-ares/releases/download/cares-1_21_0/$CARES_ARCHIVE
  export LIBSSH2_VERSION=1.11.0
  export LIBSSH2_ARCHIVE=libssh2-$LIBSSH2_VERSION.tar.bz2
  export LIBSSH2_URI=https://libssh2.org/download/$LIBSSH2_ARCHIVE
  export SQLITE_VERSION=3430100
  export SQLITE_ARCHIVE=sqlite-autoconf-$SQLITE_VERSION.tar.gz
  export SQLITE_URI=https://www.sqlite.org/2023/$SQLITE_ARCHIVE

  echo "-----build openssl-----"
  curl -L -O $OPENSSL_URI && tar xf $OPENSSL_ARCHIVE && rm $OPENSSL_ARCHIVE
  pushd openssl-$OPENSSL_VERSION
  if [[ "$HOST" == "x86_64-linux-musl" ]]; then
    ./Configure no-shared --prefix=$PREFIX linux-x86_64
  else
    ./Configure no-shared --prefix=$PREFIX linux-aarch64
  fi
  make -j$NPROC
  make install_sw
  popd

  echo "-----build zlib-----"
  curl -L -O $ZLIB_URI && tar xf $ZLIB_ARCHIVE && rm $ZLIB_ARCHIVE
  pushd zlib-$ZLIB_VERSION
  CC=$CC AR=$AR RANLIB=$RANLIB ./configure --prefix=$PREFIX --libdir=$PREFIX/lib --includedir=$PREFIX/include --static
  make -j$NPROC install
  popd

  echo "-----build libexpat-----"
  curl -L -O $LIBEXPAT_URI && tar xf $LIBEXPAT_ARCHIVE && rm $LIBEXPAT_ARCHIVE
  pushd expat-$LIBEXPAT_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared
  make -j$NPROC install
  popd

  echo "-----build sqlite3-----"
  curl -L -O $SQLITE_URI && tar xf $SQLITE_ARCHIVE && rm $SQLITE_ARCHIVE
  pushd sqlite-autoconf-$SQLITE_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared --enable-static
  make -j$NPROC install
  popd

  echo "-----build c-ares-----"
  curl -L -O $CARES_URI && tar xf $CARES_ARCHIVE && rm $CARES_ARCHIVE
  pushd c-ares-$CARES_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared
  make -j$NPROC install
  popd

  echo "-----build libssh2-----"
  curl -L -O $LIBSSH2_URI && tar xf $LIBSSH2_ARCHIVE && rm $LIBSSH2_ARCHIVE
  pushd libssh2-$LIBSSH2_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared
  make -j$NPROC install
  popd

  echo "-----build aria2-----"
  cd "$ROOT_DIR/aria2"
  autoreconf -i
  ./configure \
    --prefix=$OUT_DIR/aria2 \
    --host=$HOST \
    --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) \
    --disable-nls \
    --without-gnutls \
    --with-openssl \
    --without-libxml2 \
    --with-libexpat \
    --with-libcares \
    --with-libz \
    --with-sqlite3 \
    --with-libssh2 \
    --enable-libaria2 \
    --enable-static \
    --disable-shared \
    CPPFLAGS="-I$PREFIX/include" \
    LDFLAGS="-L$PREFIX/lib" \
    PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig" \
    ARIA2_STATIC=yes
  make -j$NPROC
  make install

  cd "$ROOT_DIR"
  tar -czvf "aria2-${SUFFIX}-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2

  echo "-----build aria2_c_api-----"
  export OUT_DIR="$ROOT_DIR/out"
  cmake --preset "${SUFFIX}-debug"
  cmake --build --preset "${SUFFIX}-debug"
  cmake --install build/Debug
  cmake --preset "${SUFFIX}-release"
  cmake --build --preset "${SUFFIX}-release"
  cmake --install build/Release --strip
  tree "$OUT_DIR" || true
  tar -czvf "aria2_c_api-${SUFFIX}-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2lib
}

build_macos_arm64() {
  mkdir -p "$OUT_DIR"
  export OUT_DIR
  export CC=clang
  export CXX=clang++
  pushd aria2
  autoreconf -i
  ./configure --prefix="$OUT_DIR/aria2" --without-openssl --without-gnutls --with-appletls --disable-nls --enable-libaria2 --enable-static --disable-shared ARIA2_STATIC=yes
  make -j$NPROC
  make install
  popd
  tar -czvf "aria2-macos-arm64-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2

  export OUT_DIR="$ROOT_DIR/out"
  cmake --preset macos-arm64-debug
  cmake --build --preset macos-arm64-debug
  cmake --install build/Debug
  cmake --preset macos-arm64-release
  cmake --build --preset macos-arm64-release
  cmake --install build/Release --strip
  tar -czvf "aria2_c_api-macos-arm64-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2lib
}

build_android_arm64() {
  export NDK_VERSION=r25c
  export NDK="$BUILD_DIR/android-ndk-$NDK_VERSION"
  export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/linux-x86_64
  export HOST=aarch64-linux-android
  export API=33
  export AR=$TOOLCHAIN/bin/llvm-ar
  export CC=$TOOLCHAIN/bin/$HOST$API-clang
  export CXX=$TOOLCHAIN/bin/$HOST$API-clang++
  export LD=$TOOLCHAIN/bin/ld
  export RANLIB=$TOOLCHAIN/bin/llvm-ranlib
  export STRIP=$TOOLCHAIN/bin/llvm-strip
  export CFLAGS="-fPIC"
  export CXXFLAGS="-fPIC"
  export LDFLAGS="-fPIC"
  mkdir -p "$DEPS_DIR" "$PREFIX" "$OUT_DIR"
  cd "$DEPS_DIR"

  export OPENSSL_VERSION=1.1.1w
  export OPENSSL_ARCHIVE=openssl-$OPENSSL_VERSION.tar.gz
  export OPENSSL_URI=https://www.openssl.org/source/$OPENSSL_ARCHIVE
  export LIBEXPAT_VERSION=2.5.0
  export LIBEXPAT_ARCHIVE=expat-$LIBEXPAT_VERSION.tar.bz2
  export LIBEXPAT_URI=https://github.com/libexpat/libexpat/releases/download/R_2_5_0/$LIBEXPAT_ARCHIVE
  export ZLIB_VERSION=1.3.1
  export ZLIB_ARCHIVE=zlib-$ZLIB_VERSION.tar.gz
  export ZLIB_URI=https://github.com/madler/zlib/releases/download/v1.3.1/$ZLIB_ARCHIVE
  export CARES_VERSION=1.21.0
  export CARES_ARCHIVE=c-ares-$CARES_VERSION.tar.gz
  export CARES_URI=https://github.com/c-ares/c-ares/releases/download/cares-1_21_0/$CARES_ARCHIVE
  export LIBSSH2_VERSION=1.11.0
  export LIBSSH2_ARCHIVE=libssh2-$LIBSSH2_VERSION.tar.bz2
  export LIBSSH2_URI=https://libssh2.org/download/$LIBSSH2_ARCHIVE

  echo "-----build openssl-----"
  curl -L -O $OPENSSL_URI && tar xf $OPENSSL_ARCHIVE && rm $OPENSSL_ARCHIVE
  pushd openssl-$OPENSSL_VERSION
  export ANDROID_NDK_HOME=$NDK
  export PATH=$TOOLCHAIN/bin:$PATH
  ./Configure no-shared --prefix=$PREFIX android-arm64
  make -j$NPROC
  make install_sw
  popd

  echo "-----build libexpat-----"
  curl -L -O $LIBEXPAT_URI && tar xf $LIBEXPAT_ARCHIVE && rm $LIBEXPAT_ARCHIVE
  pushd expat-$LIBEXPAT_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared
  make -j$NPROC install
  popd

  echo "-----build zlib-----"
  curl -L -O $ZLIB_URI && tar xf $ZLIB_ARCHIVE && rm $ZLIB_ARCHIVE
  pushd zlib-$ZLIB_VERSION
  ./configure --prefix=$PREFIX --libdir=$PREFIX/lib --includedir=$PREFIX/include --static
  make -j$NPROC install
  popd

  echo "-----build c-ares-----"
  curl -L -O $CARES_URI && tar xf $CARES_ARCHIVE && rm $CARES_ARCHIVE
  pushd c-ares-$CARES_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared
  make -j$NPROC install
  popd

  echo "-----build libssh2-----"
  curl -L -O $LIBSSH2_URI && tar xf $LIBSSH2_ARCHIVE && rm $LIBSSH2_ARCHIVE
  pushd libssh2-$LIBSSH2_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared
  make -j$NPROC install
  popd

  echo "-----build aria2-----"
  cd "$ROOT_DIR/aria2"
  autoreconf -i
  ./configure \
    --prefix=$OUT_DIR/aria2 \
    --host=$HOST \
    --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) \
    --with-pic \
    --disable-nls \
    --without-gnutls \
    --with-openssl \
    --without-sqlite3 \
    --without-libxml2 \
    --with-libexpat \
    --with-libcares \
    --with-libz \
    --with-libssh2 \
    --enable-libaria2 \
    --enable-static \
    --disable-shared \
    CXXFLAGS="-Os" \
    CFLAGS="-Os" \
    CPPFLAGS="-fPIE" \
    LDFLAGS="-fPIE -pie -L$PREFIX/lib -static-libstdc++" \
    PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig"
  make -j$NPROC
  make install

  cd "$ROOT_DIR"
  tar -czvf "aria2-android-arm64-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2

  export OUT_DIR="$ROOT_DIR/out"
  cmake --preset android-arm64-debug
  cmake --build --preset android-arm64-debug
  cmake --install build/Debug
  cmake --preset android-arm64-release
  cmake --build --preset android-arm64-release
  cmake --install build/Release --strip
  tree "$OUT_DIR" || true
  tar -czvf "aria2_c_api-android-arm64-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2lib
}

build_android_x64() {
  export NDK_VERSION=r25c
  export NDK="$BUILD_DIR/android-ndk-$NDK_VERSION"
  export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/linux-x86_64
  export HOST=x86_64-linux-android
  export API=33
  export AR=$TOOLCHAIN/bin/llvm-ar
  export CC=$TOOLCHAIN/bin/$HOST$API-clang
  export CXX=$TOOLCHAIN/bin/$HOST$API-clang++
  export LD=$TOOLCHAIN/bin/ld
  export RANLIB=$TOOLCHAIN/bin/llvm-ranlib
  export STRIP=$TOOLCHAIN/bin/llvm-strip
  export CFLAGS="-fPIC"
  export CXXFLAGS="-fPIC"
  export LDFLAGS="-fPIC"
  mkdir -p "$DEPS_DIR" "$PREFIX" "$OUT_DIR"
  cd "$DEPS_DIR"

  export OPENSSL_VERSION=1.1.1w
  export OPENSSL_ARCHIVE=openssl-$OPENSSL_VERSION.tar.gz
  export OPENSSL_URI=https://www.openssl.org/source/$OPENSSL_ARCHIVE
  export LIBEXPAT_VERSION=2.5.0
  export LIBEXPAT_ARCHIVE=expat-$LIBEXPAT_VERSION.tar.bz2
  export LIBEXPAT_URI=https://github.com/libexpat/libexpat/releases/download/R_2_5_0/$LIBEXPAT_ARCHIVE
  export ZLIB_VERSION=1.3.1
  export ZLIB_ARCHIVE=zlib-$ZLIB_VERSION.tar.gz
  export ZLIB_URI=https://github.com/madler/zlib/releases/download/v1.3.1/$ZLIB_ARCHIVE
  export CARES_VERSION=1.21.0
  export CARES_ARCHIVE=c-ares-$CARES_VERSION.tar.gz
  export CARES_URI=https://github.com/c-ares/c-ares/releases/download/cares-1_21_0/$CARES_ARCHIVE
  export LIBSSH2_VERSION=1.11.0
  export LIBSSH2_ARCHIVE=libssh2-$LIBSSH2_VERSION.tar.bz2
  export LIBSSH2_URI=https://libssh2.org/download/$LIBSSH2_ARCHIVE

  echo "-----build openssl-----"
  curl -L -O $OPENSSL_URI && tar xf $OPENSSL_ARCHIVE && rm $OPENSSL_ARCHIVE
  pushd openssl-$OPENSSL_VERSION
  export ANDROID_NDK_HOME=$NDK
  export PATH=$TOOLCHAIN/bin:$PATH
  ./Configure no-shared --prefix=$PREFIX android-x86_64
  make -j$NPROC
  make install_sw
  popd

  echo "-----build libexpat-----"
  curl -L -O $LIBEXPAT_URI && tar xf $LIBEXPAT_ARCHIVE && rm $LIBEXPAT_ARCHIVE
  pushd expat-$LIBEXPAT_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared
  make -j$NPROC install
  popd

  echo "-----build zlib-----"
  curl -L -O $ZLIB_URI && tar xf $ZLIB_ARCHIVE && rm $ZLIB_ARCHIVE
  pushd zlib-$ZLIB_VERSION
  ./configure --prefix=$PREFIX --libdir=$PREFIX/lib --includedir=$PREFIX/include --static
  make -j$NPROC install
  popd

  echo "-----build c-ares-----"
  curl -L -O $CARES_URI && tar xf $CARES_ARCHIVE && rm $CARES_ARCHIVE
  pushd c-ares-$CARES_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared
  make -j$NPROC install
  popd

  echo "-----build libssh2-----"
  curl -L -O $LIBSSH2_URI && tar xf $LIBSSH2_ARCHIVE && rm $LIBSSH2_ARCHIVE
  pushd libssh2-$LIBSSH2_VERSION
  ./configure --host=$HOST --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) --prefix=$PREFIX --disable-shared
  make -j$NPROC install
  popd

  echo "-----build aria2-----"
  cd "$ROOT_DIR/aria2"
  autoreconf -i
  ./configure \
    --prefix=$OUT_DIR/aria2 \
    --host=$HOST \
    --build=$(dpkg-architecture -qDEB_BUILD_GNU_TYPE) \
    --with-pic \
    --disable-nls \
    --without-gnutls \
    --with-openssl \
    --without-sqlite3 \
    --without-libxml2 \
    --with-libexpat \
    --with-libcares \
    --with-libz \
    --with-libssh2 \
    --enable-libaria2 \
    --enable-static \
    --disable-shared \
    CXXFLAGS="-Os" \
    CFLAGS="-Os" \
    CPPFLAGS="-fPIE" \
    LDFLAGS="-fPIE -pie -L$PREFIX/lib -static-libstdc++" \
    PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig"
  make -j$NPROC
  make install

  cd "$ROOT_DIR"
  tar -czvf "aria2-android-x64-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2

  export OUT_DIR="$ROOT_DIR/out"
  cmake --preset android-x64-debug
  cmake --build --preset android-x64-debug
  cmake --install build/Debug
  cmake --preset android-x64-release
  cmake --build --preset android-x64-release
  cmake --install build/Release --strip
  tree "$OUT_DIR" || true
  tar -czvf "aria2_c_api-android-x64-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2lib
}

build_ios_arm64() {
  export IOS_MIN_VERSION="12.0"
  export ARCH="arm64"
  export PLATFORM="iPhoneOS"
  export XCODE_ROOT=$(xcode-select -p)
  export SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
  export CC=$(xcrun --sdk iphoneos -f clang)
  export CXX=$(xcrun --sdk iphoneos -f clang++)
  export AR=$(xcrun --sdk iphoneos -f ar)
  export RANLIB=$(xcrun --sdk iphoneos -f ranlib)
  export STRIP=$(xcrun --sdk iphoneos -f strip)
  export CFLAGS="-arch ${ARCH} -isysroot ${SDK_PATH} -mios-version-min=${IOS_MIN_VERSION} -fPIC -O2"
  export HOST="arm-apple-darwin"
  export CXXFLAGS="${CFLAGS}"
  export LDFLAGS="-arch ${ARCH} -isysroot ${SDK_PATH}"
  export PKG_CONFIG_LIBDIR="${PREFIX}/lib/pkgconfig"
  mkdir -p "$DEPS_DIR" "$PREFIX" "$OUT_DIR"

  cd "$DEPS_DIR"

  echo "-----build zlib-----"
  curl -LO https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
  tar xzf zlib-1.3.1.tar.gz
  cd zlib-1.3.1
  ./configure --prefix=${PREFIX} --static
  make -j$NPROC
  make install
  cd "$DEPS_DIR"

  echo "-----build openssl-----"
  curl -LO https://www.openssl.org/source/openssl-1.1.1w.tar.gz
  tar xzf openssl-1.1.1w.tar.gz
  cd openssl-1.1.1w
  ./Configure ios64-xcrun \
    --prefix=${PREFIX} \
    no-shared \
    no-tests \
    no-ui-console \
    -fembed-bitcode \
    -mios-version-min=${IOS_MIN_VERSION}
  make -j$NPROC
  make install_sw
  cd "$DEPS_DIR"

  echo "-----build c-ares-----"
  curl -LO https://github.com/c-ares/c-ares/releases/download/v1.34.6/c-ares-1.34.6.tar.gz
  tar xzf c-ares-1.34.6.tar.gz
  cd c-ares-1.34.6
  ./configure --prefix=${PREFIX} --host=${HOST} --enable-static --disable-shared --with-pic
  make -j$NPROC
  make install
  cd "$DEPS_DIR"

  echo "-----build expat-----"
  curl -LO https://github.com/libexpat/libexpat/releases/download/R_2_5_0/expat-2.5.0.tar.gz
  tar xzf expat-2.5.0.tar.gz
  cd expat-2.5.0
  ./configure --prefix=${PREFIX} --host=${HOST} --enable-static --disable-shared --with-pic --without-docbook --without-tests --without-examples
  make -j$NPROC
  make install
  cd "$DEPS_DIR"

  echo "-----build libssh2-----"
  curl -LO https://libssh2.org/download/libssh2-1.11.0.tar.gz
  tar xzf libssh2-1.11.0.tar.gz
  cd libssh2-1.11.0
  ./configure --prefix=${PREFIX} --host=${HOST} --enable-static --disable-shared --with-pic --with-libssl-prefix=${PREFIX} --with-crypto=openssl --disable-examples-build
  make -j$NPROC
  make install

  echo "-----build aria2-----"
  cd "$ROOT_DIR/aria2"
  autoreconf -i
  ./configure \
    --prefix=${OUT_DIR}/aria2 \
    --host=${HOST} \
    --with-pic \
    --disable-nls \
    --without-gnutls \
    --with-openssl \
    --without-appletls \
    --without-sqlite3 \
    --without-libxml2 \
    --with-libexpat \
    --with-libcares \
    --with-libz \
    --with-libssh2 \
    --enable-libaria2 \
    --enable-static \
    --disable-shared \
    OPENSSL_CFLAGS="-I${PREFIX}/include" \
    OPENSSL_LIBS="-L${PREFIX}/lib -lssl -lcrypto -framework CoreFoundation -framework Security" \
    LIBCARES_CFLAGS="-I${PREFIX}/include" \
    LIBCARES_LIBS="-L${PREFIX}/lib -lcares" \
    EXPAT_CFLAGS="-I${PREFIX}/include" \
    EXPAT_LIBS="-L${PREFIX}/lib -lexpat" \
    LIBSSH2_CFLAGS="-I${PREFIX}/include" \
    LIBSSH2_LIBS="-L${PREFIX}/lib -lssh2" \
    ZLIB_CFLAGS="-I${PREFIX}/include" \
    ZLIB_LIBS="-L${PREFIX}/lib -lz" \
    ARIA2_STATIC=yes
  make -j$NPROC
  make install

  cd "$ROOT_DIR"
  tar -czvf "aria2-ios-arm64-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2

  export OUT_DIR="$ROOT_DIR/out"
  cmake --preset ios-arm64-debug
  cmake --build --preset ios-arm64-debug
  cmake --install build/Debug
  cmake --preset ios-arm64-release
  cmake --build --preset ios-arm64-release
  cmake --install build/Release --strip
  tar -czvf "aria2_c_api-ios-arm64-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2lib
}

build_macos_x64() {
  export ARCH="x86_64"
  export SDK_PATH=$(xcrun --sdk macosx --show-sdk-path)
  export CC=$(xcrun --sdk macosx -f clang)
  export CXX=$(xcrun --sdk macosx -f clang++)
  export AR=$(xcrun --sdk macosx -f ar)
  export RANLIB=$(xcrun --sdk macosx -f ranlib)
  export STRIP=$(xcrun --sdk macosx -f strip)
  export CFLAGS="-arch ${ARCH} -isysroot ${SDK_PATH} -fPIC -O2"
  export HOST="x86_64-apple-darwin"
  export CXXFLAGS="${CFLAGS}"
  export LDFLAGS="-arch ${ARCH} -isysroot ${SDK_PATH}"
  export PKG_CONFIG_LIBDIR="${PREFIX}/lib/pkgconfig"
  mkdir -p "$DEPS_DIR" "$PREFIX" "$OUT_DIR"

  cd "$DEPS_DIR"

  echo "-----build zlib-----"
  curl -LO https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
  tar xzf zlib-1.3.1.tar.gz
  cd zlib-1.3.1
  ./configure --prefix=${PREFIX} --static
  make -j$NPROC
  make install
  cd "$DEPS_DIR"

  echo "-----build openssl-----"
  curl -LO https://www.openssl.org/source/openssl-1.1.1w.tar.gz
  tar xzf openssl-1.1.1w.tar.gz
  cd openssl-1.1.1w
  ./Configure darwin64-x86_64-cc --prefix=${PREFIX} no-shared no-tests no-ui-console
  make -j$NPROC
  make install_sw
  cd "$DEPS_DIR"

  echo "-----build c-ares-----"
  curl -LO https://github.com/c-ares/c-ares/releases/download/v1.34.6/c-ares-1.34.6.tar.gz
  tar xzf c-ares-1.34.6.tar.gz
  cd c-ares-1.34.6
  ./configure --prefix=${PREFIX} --host=${HOST} --enable-static --disable-shared --with-pic
  make -j$NPROC
  make install
  cd "$DEPS_DIR"

  echo "-----build expat-----"
  curl -LO https://github.com/libexpat/libexpat/releases/download/R_2_5_0/expat-2.5.0.tar.gz
  tar xzf expat-2.5.0.tar.gz
  cd expat-2.5.0
  ./configure --prefix=${PREFIX} --host=${HOST} --enable-static --disable-shared --with-pic --without-docbook --without-tests --without-examples
  make -j$NPROC
  make install
  cd "$DEPS_DIR"

  echo "-----build libssh2-----"
  curl -LO https://libssh2.org/download/libssh2-1.11.0.tar.gz
  tar xzf libssh2-1.11.0.tar.gz
  cd libssh2-1.11.0
  ./configure --prefix=${PREFIX} --host=${HOST} --enable-static --disable-shared --with-pic --with-libssl-prefix=${PREFIX} --with-crypto=openssl --disable-examples-build
  make -j$NPROC
  make install

  echo "-----build aria2-----"
  cd "$ROOT_DIR/aria2"
  autoreconf -i
  ./configure \
    --prefix=${OUT_DIR}/aria2 \
    --host=${HOST} \
    --with-pic \
    --disable-nls \
    --without-gnutls \
    --with-openssl \
    --without-sqlite3 \
    --without-libxml2 \
    --with-libexpat \
    --with-libcares \
    --with-libz \
    --with-libssh2 \
    --enable-libaria2 \
    --enable-static \
    --disable-shared \
    OPENSSL_CFLAGS="-I${PREFIX}/include" \
    OPENSSL_LIBS="-L${PREFIX}/lib -lssl -lcrypto -framework CoreFoundation -framework Security" \
    LIBCARES_CFLAGS="-I${PREFIX}/include" \
    LIBCARES_LIBS="-L${PREFIX}/lib -lcares" \
    EXPAT_CFLAGS="-I${PREFIX}/include" \
    EXPAT_LIBS="-L${PREFIX}/lib -lexpat" \
    LIBSSH2_CFLAGS="-I${PREFIX}/include" \
    LIBSSH2_LIBS="-L${PREFIX}/lib -lssh2" \
    ZLIB_CFLAGS="-I${PREFIX}/include" \
    ZLIB_LIBS="-L${PREFIX}/lib -lz" \
    ARIA2_STATIC=yes
  make -j$NPROC
  make install

  cd "$ROOT_DIR"
  tar -czvf "aria2-macos-x64-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2

  export OUT_DIR="$ROOT_DIR/out"
  cmake --preset macos-x64-debug
  cmake --build --preset macos-x64-debug
  cmake --install build/Debug
  cmake --preset macos-x64-release
  cmake --build --preset macos-x64-release
  cmake --install build/Release --strip
  tar -czvf "aria2_c_api-macos-x64-${VERSION_NAME}.tar.gz" -C "$OUT_DIR" aria2lib
}

# Dispatch
case "${OS}-${ARCH}" in
  windows-x64)
    build_windows_mingw x86_64-w64-mingw32 windows-x64 windows-x64-debug windows-x64-release "yes"
    ;;
  windows-arm64)
    build_windows_mingw aarch64-w64-mingw32 windows-arm64 windows-arm64-debug windows-arm64-release ""
    ;;
  linux-x64)
    build_linux_x64
    ;;
  linux-arm64)
    build_linux_arm64
    ;;
  macos-arm64)
    build_macos_arm64
    ;;
  macos-x64)
    build_macos_x64
    ;;
  android-arm64)
    build_android_arm64
    ;;
  android-x64)
    build_android_x64
    ;;
  ios-arm64)
    build_ios_arm64
    ;;
  *)
    echo "Unknown or unsupported os/arch: $OS-$ARCH" >&2
    exit 1
    ;;
esac

echo "Build completed: ${OS}-${ARCH}"
