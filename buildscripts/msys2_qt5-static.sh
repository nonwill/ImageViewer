#!/bin/bash -e
PROJECT="ImageViewer"
BUILDDIR="build_msys2_qt5-static_${MSYSTEM,,?}"
SUFFIX="_qt5-static_${MSYSTEM,,?}"
APP_PATH="src/${PROJECT}"

CMD_QMAKE="${MSYSTEM_PREFIX}/qt5-static/bin/qmake.exe"

MSYSTEM_PKG_PREFIX="mingw-w64"
if [ "${MSYSTEM}" == "UCRT64" ] ; then
    MSYSTEM_PKG_PREFIX="${MSYSTEM_PKG_PREFIX}-ucrt-x86_64"
elif [ "${MSYSTEM}" == "MINGW64" ] ; then
    MSYSTEM_PKG_PREFIX="${MSYSTEM_PKG_PREFIX}-x86_64"
elif [ "${MSYSTEM}" == "CLANG64" ] ; then
    MSYSTEM_PKG_PREFIX="${MSYSTEM_PKG_PREFIX}-clang-x86_64"
elif [ "${MSYSTEM}" == "CLANGARM64" ] ; then
    MSYSTEM_PKG_PREFIX="${MSYSTEM_PKG_PREFIX}-clang-aarch64"
else
    echo "Unknown or broken MSYSTEM: ${MSYSTEM}"
    exit 1
fi
pacman -S --needed --noconfirm \
    base-devel \
    zip \
    ${MSYSTEM_PKG_PREFIX}-toolchain \
    ${MSYSTEM_PKG_PREFIX}-qt5-static

cd "$(dirname $0)"/..
SOURCE_PATH="${PWD}"
DIST_PREFIX="${PROJECT}${SUFFIX}"

function stripAll() {
    find "${DIST_PREFIX}" \( -name '*.exe' -o -name '*.dll' \) | while IFS= read -r item ; do
        strip --strip-all "${item}" || strip "${item}" || true
    done
}

rm -rf "${BUILDDIR}"
mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
mkdir -p lib
find "${MSYSTEM_PREFIX}/lib" \( -name 'libz.a' -o -name 'libzstd.a' \) | while IFS= read -r item ; do
    cp -a "${item}" "lib/lib$(basename "${item}").a"
done
${CMD_QMAKE} -r CONFIG+="release" \
    CONFIG+="hide_symbols" \
    QTPLUGIN.imageformats="qgif qicns qico qsvg qtga qtiff qwbmp" \
    QTPLUGIN.platforms="qwindows" \
    CONFIG+="enable_update_checking" \
    LIBS+="-L\"${MSYSTEM_PREFIX}/qt5-static/lib\"" \
    LIBS+="-L\"${MSYSTEM_PREFIX}/lib\"" \
    LIBS+="-L\"${PWD}/lib\"" \
    "${SOURCE_PATH}/${PROJECT}.pro"
make -j$(getconf _NPROCESSORS_ONLN)
mkdir "${DIST_PREFIX}"
cp -a "${APP_PATH}/release/${PROJECT}.exe" "${DIST_PREFIX}/"
stripAll
zip -9r "../${DIST_PREFIX}.zip" "${DIST_PREFIX}"
cd ..
