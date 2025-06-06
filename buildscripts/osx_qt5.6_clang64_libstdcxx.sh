#!/bin/bash -e
PROJECT=ImageViewer
BUILDDIR=build_osx_qt5.6_clang64_libstdcxx
APPNAME="Image Viewer"
DMGNAME="${PROJECT}_qt5.6_clang64_libstdcxx"
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
MAC_TARGET="10.6"
ALL_SDK_VERSIONS="$(xcodebuild -showsdks | grep '\-sdk macosx' | sed 's|.*-sdk macosx||')"
for SDK_VERSION in ${ALL_SDK_VERSIONS} ; do
    SDK_PATH="$(xcode-select -p)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${SDK_VERSION}.sdk"
    if [[ $(find "${SDK_PATH}/usr/lib" -name 'libstdc++*' -maxdepth 1 | wc -l | xargs) > 0 ]] ; then
        MAC_SDK="macosx${SDK_VERSION}"
    fi
done

QT_PATH="${QT_PATH:=/opt/Qt/5.6.3/clang_64_libstdc++_sdk10.10}"
QTPLUGINS_PATH="${QT_PATH}/plugins"
CMD_QMAKE="${QT_PATH}/bin/qmake"
CMD_DEPLOY="${QT_PATH}/bin/macdeployqt"

echo "Using MAC_SDK=${MAC_SDK}"

cd "$(dirname $0)"/..
SOURCE_PATH="${PWD}"
rm -rf "${BUILDDIR}"
mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
BUILD_PATH="${PWD}"
arch -x86_64 ${CMD_QMAKE} -r CONFIG+="release" LIBS+=-dead_strip QMAKE_MAC_SDK=${MAC_SDK} CONFIG+="disable_cxx11" CONFIG+="disable_embed_translations" "../${PROJECT}.pro"
arch -x86_64 make -j$(getconf _NPROCESSORS_ONLN)
cd "${OUT_PATH}"
plutil -replace LSMinimumSystemVersion -string "${MAC_TARGET}" "${APPNAME}.app/Contents/Info.plist"
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
/usr/bin/python3 "${SOURCE_PATH}/buildscripts/helpers/dylibresolver.py" "${APPNAME}.app" "${QT_PATH}/lib"
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

function fix_sdk() {
    if [ -z "${NO_SIGN+x}" ] ; then
        local binary="${1}"
        local sdk="${MAC_SDK:6}"
        for SDK_VERSION in ${ALL_SDK_VERSIONS} ; do
            sdk="${SDK_VERSION}"
        done
        echo "Override LC_VERSION_MIN_MACOSX sdk to ${sdk} in ${binary}"
        vtool -set-version-min "macos" "${MAC_TARGET}" "${sdk}" -replace -output "${binary}" "${binary}"
    fi
}
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
find "${INSTALL_PATH}/${APPNAME}.app/Contents/MacOS" -type f -print0 | while IFS= read -r -d '' item ; do fix_sdk "${item}" ; done
find "${INSTALL_PATH}/${APPNAME}.app/Contents/Frameworks" \( -name '*.framework' -or -name '*.dylib' \) -print0 | while IFS= read -r -d '' item ; do sign "${item}" ; done
find "${INSTALL_PATH}/${APPNAME}.app/Contents/PlugIns"       -name '*.dylib'                            -print0 | while IFS= read -r -d '' item ; do sign "${item}" ; done
sign "${INSTALL_PATH}/${APPNAME}.app"
notarize "${INSTALL_PATH}/${APPNAME}.app" "$(plutil -extract CFBundleIdentifier xml1 -o - "${INSTALL_PATH}/${APPNAME}.app/Contents/Info.plist" | sed -n 's|.*<string>\(.*\)<\/string>.*|\1|p')"

hdiutil create -format UDBZ -fs HFS+ -srcfolder "${INSTALL_PATH}" -volname "${APPNAME}" "${ARTIFACTS_PATH}/${DMGNAME}.dmg"
sign "${ARTIFACTS_PATH}/${DMGNAME}.dmg"
