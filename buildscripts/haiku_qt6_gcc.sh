#!/bin/bash -e
PROJECT="ImageViewer"
BUILDDIR="build_haiku_qt6_gcc"
SUFFIX="_qt6_$(gcc -dumpmachine | sed 's|-unknown-|-| ; s|-pc-|-|')"
APP_PATH="src/${PROJECT}"
RDEF_PATH="src/${PROJECT}/resources/platform/haiku/${PROJECT}.rdef"

export PATH="/bin/x86:${PATH}"
CMD_QMAKE="qmake6"

cd "$(dirname $0)"/..
rm -rf "${BUILDDIR}"
mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
${CMD_QMAKE} -spec haiku-g++ -r CONFIG+="release" CONFIG+="enable_update_checking" "../${PROJECT}.pro"
make
strip --strip-all "${APP_PATH}/${PROJECT}"
cp -a "${APP_PATH}/${PROJECT}" ../"${PROJECT}${SUFFIX}"
rc -o "${PROJECT}.rsrc" ../"${RDEF_PATH}"
xres -o ../"${PROJECT}${SUFFIX}" "${PROJECT}.rsrc"
cd ..

