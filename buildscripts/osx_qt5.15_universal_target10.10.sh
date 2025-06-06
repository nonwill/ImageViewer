#!/bin/bash -e
PROJECT=ImageViewer
BUILDDIR=build_osx_qt5.15_universal_target10.10
APPNAME="Image Viewer"
DMGNAME="${PROJECT}_qt5.15_universal_target10.10"
SCRIPT_PATH="src/${PROJECT}/resources/platform/macosx/set_associations.sh"
QM_FILES_PATH="src/${PROJECT}/resources/translations"
QTUTILS_QM_FILES_PATH="src/QtUtils/resources/translations"
LICENSE_PATH="LICENSE.GPLv3"
OUT_PATH="src/${PROJECT}"
ENTITLEMENTS_PATH="src/${PROJECT}/resources/platform/macosx/${PROJECT}.entitlements"
APP_CERT="Developer ID Application: Petr Zhigalov (48535TNTA7)"
NOTARIZE_USERNAME="peter.zhigalov@gmail.com"
NOTARIZE_PASSWORD="@keychain:Notarize: ${NOTARIZE_USERNAME}"
NOTARIZE_ASC_PROVIDER="${APP_CERT: -11:10}"
MAC_TARGET="10.10"
MAC_SDK="$(xcodebuild -showsdks | grep '\-sdk macosx' | tail -1 | sed 's|.*-sdk ||')"

QT_PATH="${QT_PATH:=/opt/Qt/5.15.16/clang_universal_target10.10}"
QTPLUGINS_PATH="${QT_PATH}/plugins"
CMD_QMAKE="${QT_PATH}/bin/qmake"
CMD_DEPLOY="${QT_PATH}/bin/macdeployqt"

RESVG_X86_64_PATH="$(cd "$(dirname "${0}")" && pwd)/resvg/x86_64-apple-darwin"
RESVG_ARM64_PATH="$(cd "$(dirname "${0}")" && pwd)/resvg/aarch64-apple-darwin"
function make_universal_resvg() {
    local RESVG_PATH="${1}"
    rm -rf "${RESVG_PATH}"
    mkdir -p "${RESVG_PATH}"
    lipo "${RESVG_X86_64_PATH}/libresvg.dylib" "${RESVG_ARM64_PATH}/libresvg.dylib" -create -output "${RESVG_PATH}/libresvg.dylib"
    cp -a "${RESVG_X86_64_PATH}/resvg.h" "${RESVG_PATH}/resvg.h"
}

echo "Using MAC_SDK=${MAC_SDK}"

cd "$(dirname $0)"/..
SOURCE_PATH="${PWD}"
rm -rf "${BUILDDIR}"
mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
BUILD_PATH="${PWD}"
RESVG_PATH="${BUILD_PATH}/resvg/universal-apple-darwin"
make_universal_resvg "${RESVG_PATH}"
${CMD_QMAKE} -r CONFIG+="release" CONFIG+="hide_symbols" LIBS+=-dead_strip QMAKE_MAC_SDK=${MAC_SDK} QMAKE_MAC_SDK_VERSION=${MAC_SDK:6} QMAKE_MACOSX_DEPLOYMENT_TARGET=${MAC_TARGET} QMAKE_APPLE_DEVICE_ARCHS="x86_64 arm64" CONFIG+="enable_update_checking" CONFIG+="disable_embed_translations" CONFIG+="system_resvg" INCLUDEPATH+="\"${RESVG_PATH}\"" LIBS+="-L\"${RESVG_PATH}\"" "../${PROJECT}.pro"
make -j$(getconf _NPROCESSORS_ONLN)
cd "${OUT_PATH}"
plutil -replace LSMinimumSystemVersion -string "${MAC_TARGET}" "${APPNAME}.app/Contents/Info.plist"
plutil -replace LSArchitecturePriority -json '["arm64","x86_64"]' "${APPNAME}.app/Contents/Info.plist"
RES_PATH="${APPNAME}.app/Contents/Resources"
rm -rf "${RES_PATH}/empty.lproj"
for locale in $(find "${SOURCE_PATH}/${QM_FILES_PATH}/" -maxdepth 1 -mindepth 1 -type f -name '*.qm' | sed 's|.*/|| ; s|[^_]*_|| ; s|\..*||' | xargs) ; do
    mkdir -p "${RES_PATH}/${locale}.lproj"
done
cp -a "${SOURCE_PATH}/${SCRIPT_PATH}" "${RES_PATH}/"
PLUGINS_PATH="${APPNAME}.app/Contents/PlugIns"
mkdir -p "${PLUGINS_PATH}/iconengines"
for iconengines_plugin in libqsvgicon.dylib ; do
    cp -a "${QTPLUGINS_PATH}/iconengines/${iconengines_plugin}" "${PLUGINS_PATH}/iconengines/"
done
arch -x86_64 ${CMD_DEPLOY} "${APPNAME}.app" -verbose=2
for unused_plugins_subdir in virtualkeyboard platforminputcontexts ; do
    rm -r "${PLUGINS_PATH}/${unused_plugins_subdir}"
done
FRAMEWORKS_PATH="${APPNAME}.app/Contents/Frameworks"
for unused_framework in QtQml.framework QtQmlModels.framework QtQuick.framework QtVirtualKeyboard.framework ; do
    rm -r "${FRAMEWORKS_PATH}/${unused_framework}"
done
/usr/bin/python3 "${SOURCE_PATH}/buildscripts/helpers/dylibresolver.py" "${APPNAME}.app" "${RESVG_PATH}" "${QT_PATH}/lib"
TRANSLATIONS_PATH="${RES_PATH}/translations"
mkdir -p "${TRANSLATIONS_PATH}"
for lang in $(find "${RES_PATH}" -name '*.lproj' | sed 's|.*/|| ; s|\..*||') ; do
    if [ -f "${QT_PATH}/translations/qtbase_${lang}.qm" ] ; then
        cp -a "${QT_PATH}/translations/qtbase_${lang}.qm" "${TRANSLATIONS_PATH}/qt_${lang}.qm"
    elif [ -f "${QT_PATH}/translations/qt_${lang}.qm" ] ; then
        cp -a "${QT_PATH}/translations/qt_${lang}.qm" "${TRANSLATIONS_PATH}/qt_${lang}.qm"
    fi
done
find "${SOURCE_PATH}/${QM_FILES_PATH}" -mindepth 1 -maxdepth 1 -type f -name '*.qm' -exec cp -a \{\} "${TRANSLATIONS_PATH}/" \;
find "${SOURCE_PATH}/${QTUTILS_QM_FILES_PATH}" -mindepth 1 -maxdepth 1 -type f -name '*.qm' -exec cp -a \{\} "${TRANSLATIONS_PATH}/" \;
echo 'Translations = Resources/translations' >> "${RES_PATH}/qt.conf"
cd "${BUILD_PATH}"

INSTALL_PATH="${PWD}/install"
ARTIFACTS_PATH="${PWD}/artifacts"
rm -rf "${INSTALL_PATH}" "${ARTIFACTS_PATH}"
mkdir -p "${INSTALL_PATH}" "${ARTIFACTS_PATH}"
mv "${OUT_PATH}/${APPNAME}.app" "${INSTALL_PATH}/"
cd "${INSTALL_PATH}"
ln -s /Applications ./Applications
cp -a "${SOURCE_PATH}/${LICENSE_PATH}" "./"
find "${APPNAME}.app/Contents/PlugIns" -name "*_debug.dylib" -delete
find "${APPNAME}.app/Contents" -type f -name '*.prl' -delete
cd "${BUILD_PATH}"

function sign() {
    if [ -z "${NO_SIGN+x}" ] ; then
        local max_retry=10
        local last_retry=$((${max_retry}-1))
        for ((i=0; i<${max_retry}; i++)) ; do
            if arch -x86_64 /usr/bin/codesign \
                    --sign "${APP_CERT}" \
                    --deep \
                    --force \
                    --timestamp \
                    --options runtime \
                    --entitlements "${SOURCE_PATH}/${ENTITLEMENTS_PATH}" \
                    --verbose \
                    --strict \
                    "${1}" ; then
                if [ ${i} != 0 ] ; then
                    echo "Sign completed at ${i} retry"
                fi
                break
            else
                if [ ${i} != ${last_retry} ] ; then
                    echo "Signing failed, retry ..."
                    sleep 5
                else
                    exit 2
                fi
            fi
        done
    fi
}
function notarize() {
    if [ -z "${NO_SIGN+x}" ] ; then
        /usr/bin/python3 "${SOURCE_PATH}/buildscripts/helpers/MacNotarizer.py" \
            --application "${1}" \
            --primary-bundle-id "${2}" \
            --username "${NOTARIZE_USERNAME}" \
            --password "${NOTARIZE_PASSWORD}" \
            --asc-provider "${NOTARIZE_ASC_PROVIDER}"
    fi
}
find "${INSTALL_PATH}/${APPNAME}.app/Contents/Frameworks" \( -name '*.framework' -or -name '*.dylib' \) -print0 | while IFS= read -r -d '' item ; do sign "${item}" ; done
find "${INSTALL_PATH}/${APPNAME}.app/Contents/PlugIns"       -name '*.dylib'                            -print0 | while IFS= read -r -d '' item ; do sign "${item}" ; done
sign "${INSTALL_PATH}/${APPNAME}.app"
notarize "${INSTALL_PATH}/${APPNAME}.app" "$(plutil -extract CFBundleIdentifier xml1 -o - "${INSTALL_PATH}/${APPNAME}.app/Contents/Info.plist" | sed -n 's|.*<string>\(.*\)<\/string>.*|\1|p')"

hdiutil create -format UDBZ -fs HFS+ -srcfolder "${INSTALL_PATH}" -volname "${APPNAME}" "${ARTIFACTS_PATH}/${DMGNAME}.dmg"
sign "${ARTIFACTS_PATH}/${DMGNAME}.dmg"
