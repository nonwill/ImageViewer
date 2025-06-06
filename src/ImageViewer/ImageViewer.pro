#
#  Copyright (C) 2017-2025 Peter S. Zhigalov <peter.zhigalov@gmail.com>
#
#  This file is part of the `ImageViewer' program.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

TEMPLATE = app
TARGET = ImageViewer

INCLUDEPATH += src

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += warn_on

DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_KEYWORDS

include(../Features.pri)
include(../QtUtils/QtUtils.pri)
include(../ThirdParty/ThirdParty.pri)

*g++*|*clang*|*llvm*|*xcode* {
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE *= -O3
    QMAKE_CXXFLAGS_RELEASE *= -DNDEBUG
    QMAKE_CXXFLAGS_RELEASE *= -DQT_NO_DEBUG_OUTPUT
#    QMAKE_LFLAGS += -Wl,--whole-archive
}

*msvc* {
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE *= -Ox
    QMAKE_CXXFLAGS_RELEASE -= -GS
    QMAKE_CXXFLAGS_RELEASE *= -GS-
    QMAKE_CXXFLAGS_RELEASE *= -DQT_NO_DEBUG_OUTPUT
    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += _CRT_SECURE_NO_DEPRECATE
    DEFINES += _USE_MATH_DEFINES
}

win32 {
    DEFINES += NOMINMAX
}

enable_update_checking {
    DEFINES += ENABLE_UPDATE_CHECKING
}

SOURCES += \
    src/GUI/FileManager.cpp \
    src/GUI/GUISettings.cpp \
    src/GUI/MainController.cpp \
    src/GUI/Dialogs/AboutDialog.cpp \
    src/GUI/Dialogs/InfoDialog.cpp \
    src/GUI/Dialogs/SettingsDialog.cpp \
    src/GUI/MainWindow/EffectsStorage.cpp \
    src/GUI/MainWindow/ImageViewerWidget.cpp \
    src/GUI/MainWindow/MainWindow.cpp \
    src/GUI/MainWindow/MenuBar.cpp \
    src/Decoders/DecodersManager.cpp \
    src/Decoders/Impl/DecoderQImage.cpp \
    src/Decoders/Impl/DecoderQMovie.cpp \
    src/Decoders/Impl/Internal/Animation/AnimationObject.cpp \
    src/Decoders/Impl/Internal/Animation/AnimationWidget.cpp \
    src/Decoders/Impl/Internal/Animation/DelayCalculator.cpp \
    src/Decoders/Impl/Internal/Animation/FramesCompositor.cpp \
    src/Decoders/Impl/Internal/Animation/AbstractAnimationProvider.cpp \
    src/Decoders/Impl/Internal/GraphicsItems/GraphicsItemUtils.cpp \
    src/Decoders/Impl/Internal/GraphicsItems/ProgressiveResampledImageGraphicsItem.cpp \
    src/Decoders/Impl/Internal/GraphicsItems/RasterizedImageGraphicsItem.cpp \
    src/Decoders/Impl/Internal/GraphicsItems/ResampledImageGraphicsItem.cpp \
    src/Decoders/Impl/Internal/Scaling/AbstractScalingManager.cpp \
    src/Decoders/Impl/Internal/Scaling/AbstractScalingWorker.cpp \
    src/Decoders/Impl/Internal/Scaling/AbstractScalingWorkerHandler.cpp \
    src/Decoders/Impl/Internal/Scaling/AutoUpdatedScalingWorkerHandler.cpp \
    src/Decoders/Impl/Internal/Utils/CmsUtils.cpp \
    src/Decoders/Impl/Internal/Utils/DataProcessing.cpp \
    src/Decoders/Impl/Internal/Utils/LibraryUtils.cpp \
    src/Decoders/Impl/Internal/Utils/MappedBuffer.cpp \
    src/Decoders/Impl/Internal/Utils/XmlStreamReader.cpp \
    src/Decoders/Impl/Internal/GraphicsItemsFactory.cpp \
    src/Decoders/Impl/Internal/ImageData.cpp \
    src/Decoders/Impl/Internal/ImageMetaData.cpp \
    src/main.cpp

HEADERS += \
    src/GUI/FileManager.h \
    src/GUI/GUISettings.h \
    src/GUI/MainController.h \
    src/GUI/UIState.h \
    src/GUI/Dialogs/AboutDialog.h \
    src/GUI/Dialogs/AboutDialog_p.h \
    src/GUI/Dialogs/Contributors.h \
    src/GUI/Dialogs/InfoDialog.h \
    src/GUI/Dialogs/InfoDialog_p.h \
    src/GUI/Dialogs/SettingsDialog.h \
    src/GUI/Dialogs/SettingsDialog_p.h \
    src/GUI/MainWindow/EffectsStorage.h \
    src/GUI/MainWindow/IControlsContainer.h \
    src/GUI/MainWindow/ImageViewerWidget.h \
    src/GUI/MainWindow/MainWindow.h \
    src/GUI/MainWindow/MainWindow_p.h \
    src/GUI/MainWindow/MenuBar.h \
    src/Decoders/DecodersManager.h \
    src/Decoders/IDecoder.h \
    src/Decoders/IImageData.h \
    src/Decoders/IImageMetaData.h \
    src/Decoders/GraphicsItemFeatures/IGrabImage.h \
    src/Decoders/GraphicsItemFeatures/IGrabScaledImage.h \
    src/Decoders/GraphicsItemFeatures/ITransformationMode.h \
    src/Decoders/Impl/Internal/Animation/AnimationObject.h \
    src/Decoders/Impl/Internal/Animation/AnimationWidget.h \
    src/Decoders/Impl/Internal/Animation/DelayCalculator.h \
    src/Decoders/Impl/Internal/Animation/FramesCompositor.h \
    src/Decoders/Impl/Internal/Animation/IAnimationProvider.h \
    src/Decoders/Impl/Internal/Animation/AbstractAnimationProvider.h \
    src/Decoders/Impl/Internal/Animation/MovieAnimationProvider.h \
    src/Decoders/Impl/Internal/GraphicsItems/GraphicsItemUtils.h \
    src/Decoders/Impl/Internal/GraphicsItems/ProgressiveResampledImageGraphicsItem.h \
    src/Decoders/Impl/Internal/GraphicsItems/RasterizedImageGraphicsItem.h \
    src/Decoders/Impl/Internal/GraphicsItems/ResampledImageGraphicsItem.h \
    src/Decoders/Impl/Internal/Scaling/AbstractProgressiveImageProvider.h \
    src/Decoders/Impl/Internal/Scaling/AbstractScalingManager.h \
    src/Decoders/Impl/Internal/Scaling/AbstractScalingWorker.h \
    src/Decoders/Impl/Internal/Scaling/AbstractScalingWorkerHandler.h \
    src/Decoders/Impl/Internal/Scaling/AutoUpdatedScalingWorkerHandler.h \
    src/Decoders/Impl/Internal/Scaling/IScaledImageProvider.h \
    src/Decoders/Impl/Internal/Utils/CmsUtils.h \
    src/Decoders/Impl/Internal/Utils/DataProcessing.h \
    src/Decoders/Impl/Internal/Utils/LibraryUtils.h \
    src/Decoders/Impl/Internal/Utils/MappedBuffer.h \
    src/Decoders/Impl/Internal/Utils/XmlStreamReader.h \
    src/Decoders/Impl/Internal/DecoderAutoRegistrator.h \
    src/Decoders/Impl/Internal/GraphicsItemsFactory.h \
    src/Decoders/Impl/Internal/ImageData.h \
    src/Decoders/Impl/Internal/ImageMetaData.h \
    src/Decoders/Impl/Internal/PayloadWithMetaData.h \

!disable_zlib {
    SOURCES += \
        src/Decoders/Impl/Internal/Utils/ZLibUtils.cpp
    HEADERS += \
        src/Decoders/Impl/Internal/Utils/ZLibUtils.h
}

!disable_stb {
    SOURCES += \
        src/Decoders/Impl/DecoderSTB.cpp
}

!disable_qtimageformats {
    SOURCES += \
        src/Decoders/Impl/DecoderQtImageFormatsImage.cpp \
        src/Decoders/Impl/DecoderQtImageFormatsMovie.cpp
}

!disable_kimageformats {
    SOURCES += \
        src/Decoders/Impl/DecoderKImageFormatsImage.cpp \
        src/Decoders/Impl/DecoderKImageFormatsMovie.cpp
}

!disable_libjpeg {
    SOURCES += \
        src/Decoders/Impl/DecoderLibJpeg.cpp
}

!disable_libmng {
    SOURCES += \
        src/Decoders/Impl/DecoderLibMng.cpp
}

!disable_libpng {
    SOURCES += \
        src/Decoders/Impl/DecoderLibPng.cpp
}

!disable_libjasper {
    SOURCES += \
        src/Decoders/Impl/DecoderLibJasPer.cpp
}

!disable_jbigkit {
    SOURCES += \
        src/Decoders/Impl/DecoderJbigKit.cpp
}

!disable_lerc {
    SOURCES += \
        src/Decoders/Impl/DecoderLERC.cpp
}

!disable_libtiff {
    SOURCES += \
        src/Decoders/Impl/DecoderLibTiff.cpp
}

!disable_libwebp {
    SOURCES += \
        src/Decoders/Impl/DecoderLibWebP.cpp
}

!disable_libbpg {
    SOURCES += \
        src/Decoders/Impl/DecoderLibBpg.cpp
}

!disable_libwmf {
    SOURCES += \
        src/Decoders/Impl/DecoderLibWmf.cpp
}

!disable_openjpeg {
    SOURCES += \
        src/Decoders/Impl/DecoderOpenJPEG.cpp
}

!disable_giflib {
    SOURCES += \
        src/Decoders/Impl/DecoderGifLib.cpp
}

!disable_libraw {
    SOURCES += \
        src/Decoders/Impl/DecoderLibRaw.cpp
}

!disable_librsvg {
    SOURCES += \
        src/Decoders/Impl/DecoderLibRSVG.cpp
}

!disable_resvg {
    SOURCES += \
        src/Decoders/Impl/DecoderReSVGLt001100.cpp \
        src/Decoders/Impl/DecoderReSVGLt001300.cpp \
        src/Decoders/Impl/DecoderReSVGLt003300.cpp \
        src/Decoders/Impl/DecoderReSVG.cpp
}

!disable_libheif {
    SOURCES += \
        src/Decoders/Impl/DecoderLibHEIF.cpp
}

!disable_openexr {
    SOURCES += \
        src/Decoders/Impl/DecoderOpenEXR.cpp
}

!disable_libavif {
    SOURCES += \
        src/Decoders/Impl/DecoderLibAvif.cpp
}

!disable_flif {
    SOURCES += \
        src/Decoders/Impl/DecoderFLIF.cpp
}

!disable_jxrlib {
    SOURCES += \
        src/Decoders/Impl/DecoderJxrLib.cpp
}

!disable_libjxl {
    SOURCES += \
        src/Decoders/Impl/DecoderLibJxl.cpp
}

!disable_qtsvg {
    QT += svg
    greaterThan(QT_MAJOR_VERSION, 5): QT += svgwidgets
    SOURCES += \
        src/Decoders/Impl/DecoderQtSVG.cpp
}

!disable_wic {
    SOURCES += \
        src/Decoders/Impl/DecoderWIC.cpp
    *msvc* {
        QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
        QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
        QMAKE_CFLAGS -= -Zc:strictStrings
        QMAKE_CXXFLAGS -= -Zc:strictStrings
    }
}

!disable_nsimage {
    OBJECTIVE_SOURCES += \
        src/Decoders/Impl/DecoderNSImage.mm
}

!disable_mactoolbar {
    DEFINES += HAS_MAC_TOOLBAR
    OBJECTIVE_SOURCES += \
        src/GUI/MainWindow/MacToolBar.mm
    HEADERS += \
        src/GUI/MainWindow/MacToolBar.h
} else {
    SOURCES += \
        src/GUI/MainWindow/ToolBar.cpp \
        src/GUI/MainWindow/QtToolBar.cpp
    HEADERS += \
        src/GUI/MainWindow/ToolBar.h \
        src/GUI/MainWindow/QtToolBar.h
}

!disable_mactouchbar {
    DEFINES += HAS_MAC_TOUCHBAR
    OBJECTIVE_SOURCES += \
        src/GUI/MainWindow/MacTouchBar.mm
    HEADERS += \
        src/GUI/MainWindow/MacTouchBar.h
}

!disable_printsupport {
    greaterThan(QT_MAJOR_VERSION, 4): QT += printsupport
    DEFINES += ENABLE_PRINT_SUPPORT
    SOURCES += \
        src/GUI/Dialogs/PrintDialog.cpp
    HEADERS += \
        src/GUI/Dialogs/PrintDialog.h \
        src/GUI/Dialogs/PrintDialog_p.h
}

!disable_fallback_iccprofiles {
    DEFINES += HAS_FALLBACK_ICCPROFILES
    RESOURCES += \
        resources/iccprofiles/iccprofiles.qrc
}

!disable_qtcore5compat {
    QT += core5compat
}

lupdate_only {
    SOURCES += $${OBJECTIVE_SOURCES}
    OBJECTIVE_SOURCES =
}

TRANSLATIONS += \
    resources/translations/imageviewer_af.ts \
    resources/translations/imageviewer_ar.ts \
    resources/translations/imageviewer_ast.ts \
    resources/translations/imageviewer_be.ts \
    resources/translations/imageviewer_bg.ts \
    resources/translations/imageviewer_bn.ts \
    resources/translations/imageviewer_bn_IN.ts \
    resources/translations/imageviewer_bs.ts \
    resources/translations/imageviewer_ca.ts \
    resources/translations/imageviewer_cs.ts \
    resources/translations/imageviewer_da.ts \
    resources/translations/imageviewer_de.ts \
    resources/translations/imageviewer_el.ts \
    resources/translations/imageviewer_en.ts \
    resources/translations/imageviewer_en_GB.ts \
    resources/translations/imageviewer_eo.ts \
    resources/translations/imageviewer_es.ts \
    resources/translations/imageviewer_et.ts \
    resources/translations/imageviewer_eu.ts \
    resources/translations/imageviewer_fa.ts \
    resources/translations/imageviewer_fi.ts \
    resources/translations/imageviewer_fo.ts \
    resources/translations/imageviewer_fr.ts \
    resources/translations/imageviewer_gl.ts \
    resources/translations/imageviewer_he.ts \
    resources/translations/imageviewer_hr.ts \
    resources/translations/imageviewer_hu.ts \
    resources/translations/imageviewer_id.ts \
    resources/translations/imageviewer_is.ts \
    resources/translations/imageviewer_it.ts \
    resources/translations/imageviewer_ja.ts \
    resources/translations/imageviewer_kk.ts \
    resources/translations/imageviewer_kn.ts \
    resources/translations/imageviewer_ko.ts \
    resources/translations/imageviewer_lg.ts \
    resources/translations/imageviewer_lt.ts \
    resources/translations/imageviewer_ms.ts \
    resources/translations/imageviewer_nb.ts \
    resources/translations/imageviewer_nl.ts \
    resources/translations/imageviewer_nn.ts \
    resources/translations/imageviewer_pl.ts \
    resources/translations/imageviewer_pt_BR.ts \
    resources/translations/imageviewer_pt_PT.ts \
    resources/translations/imageviewer_ro.ts \
    resources/translations/imageviewer_ru.ts \
    resources/translations/imageviewer_sk.ts \
    resources/translations/imageviewer_sl.ts \
    resources/translations/imageviewer_sr.ts \
    resources/translations/imageviewer_sr_Latn.ts \
    resources/translations/imageviewer_sv.ts \
    resources/translations/imageviewer_te.ts \
    resources/translations/imageviewer_th.ts \
    resources/translations/imageviewer_tr.ts \
    resources/translations/imageviewer_tt_RU.ts \
    resources/translations/imageviewer_ug.ts \
    resources/translations/imageviewer_uk.ts \
    resources/translations/imageviewer_ur.ts \
    resources/translations/imageviewer_vi.ts \
    resources/translations/imageviewer_zh_CN.ts \
    resources/translations/imageviewer_zh_TW.ts

win32 {
    RC_FILE += resources/platform/windows/resources.rc
}

macx {
    LIBS += -framework AppKit
    LIBS += -framework Foundation

    QMAKE_INFO_PLIST = resources/platform/macosx/Info.plist
    ICON = resources/icon/icon.icns
    TARGET = "Image Viewer"
    *clang* {
        QMAKE_CXXFLAGS += -Wno-invalid-constexpr
    }
}

RESOURCES += \
    resources/icon/icon.qrc \
    resources/style/style.qrc

disable_embed_translations {
    DEFINES += \
        DISABLE_EMBED_TRANSLATIONS
} else {
    RESOURCES += \
        resources/translations/imageviewer_translations.qrc
}

# qmake CONFIG+=use_static_qjpeg
use_static_qjpeg {
    QTPLUGIN += qjpeg
    DEFINES += USE_STATIC_QJPEG
}

# qmake CONFIG+=use_static_qtiff
use_static_qtiff {
    QTPLUGIN += qtiff
    DEFINES += USE_STATIC_QTIFF
}

# qmake CONFIG+=use_static_qico
use_static_qico {
    QTPLUGIN += qico
    DEFINES += USE_STATIC_QICO
}

# qmake CONFIG+=use_static_qgif
use_static_qgif {
    QTPLUGIN += qgif
    DEFINES += USE_STATIC_QGIF
}

# qmake CONFIG+=use_static_qmng
use_static_qmng {
    QTPLUGIN += qmng
    DEFINES += USE_STATIC_QMNG
}

# qmake CONFIG+=use_static_qsvg
use_static_qsvg {
    QTPLUGIN += qsvg
    DEFINES += USE_STATIC_QSVG
}
