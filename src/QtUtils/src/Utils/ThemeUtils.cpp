/*
   Copyright (C) 2011-2023 Peter S. Zhigalov <peter.zhigalov@gmail.com>

   This file is part of the `QtUtils' library.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ThemeUtils.h"
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QPixmap>
#include <QImage>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#if defined (Q_OS_WIN)
#include <windows.h>
#endif

#include "InfoUtils.h"

namespace ThemeUtils {

namespace {

void AddPixmapToIcon(QIcon &icon, const QPixmap &pixmap)
{
    QStyleOption opt(0);
    opt.palette = QApplication::palette();
#define ADD_PIXMAP(MODE) icon.addPixmap(QApplication::style()->generatedIconPixmap(MODE, pixmap, &opt), MODE)
    ADD_PIXMAP(QIcon::Normal);
    ADD_PIXMAP(QIcon::Disabled);
    ADD_PIXMAP(QIcon::Active);
    ADD_PIXMAP(QIcon::Selected);
#undef ADD_PIXMAP
}

int Lightness(const QColor &color)
{
#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
    return color.lightness();
#else
    const QColor rgb = color.toRgb();
    const int r = rgb.red();
    const int g = rgb.green();
    const int b = rgb.blue();
    const int cmax = qMax(qMax(r, g), b);
    const int cmin = qMin(qMin(r, g), b);
    const int l = (cmax + cmin) / 2;
    return l;
#endif
}

bool IsDarkTheme(const QPalette &palette, QPalette::ColorRole windowRole, QPalette::ColorRole windowTextRole)
{
    // https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5
    return Lightness(palette.color(windowTextRole)) > Lightness(palette.color(windowRole));
}

} // namespace

/// @brief Считать стилизацию из файла и применить ее к QApplication
/// @param[in] filePath - Путь до QSS файла со стилями
/// @return true если стилизация успешно применена, false - иначе
bool LoadStyleSheet(const QString &filePath)
{
    return LoadStyleSheet(QStringList() << filePath);
}

/// @brief Считать стилизацию из файлов и применить ее к QApplication
/// @param[in] filePaths - Список путей до QSS файлов со стилями
/// @return true если стилизация успешно применена, false - иначе
bool LoadStyleSheet(const QStringList &filePaths)
{
    bool status = true;
    QString allStyles;
    for(QStringList::ConstIterator it = filePaths.constBegin(), itEnd = filePaths.constEnd(); it != itEnd; ++it)
    {
        QFile styleFile(*it);
        if(styleFile.open(QIODevice::ReadOnly))
            allStyles.append(QTextStream(&styleFile).readAll());
        else
            status = false;
    }
    qApp->setStyleSheet(allStyles);
    return status;
}

/// @brief Функция для определения темная используемая тема виджета или нет
/// @param[in] widget - Виджет, для которого выполняется эта проверка
bool WidgetHasDarkTheme(const QWidget *widget)
{
    return IsDarkTheme(widget->palette(), widget->backgroundRole(), widget->foregroundRole());
}

#if !defined (Q_OS_MAC)
/// @brief Функция для определения темная используемая тема системы или нет
bool SystemHasDarkTheme()
{
#if defined (Q_OS_WIN)
    // https://github.com/ysc3839/win32-darkmode/blob/master/win32-darkmode
    HMODULE library = ::LoadLibraryA("uxtheme.dll");
    if(library)
    {
        // https://github.com/mintty/mintty/issues/983
        // https://github.com/mintty/mintty/pull/984
        // https://github.com/mintty/mintty/blob/eeaaed8/src/winmain.c
//        // 1903 18362
//        if(InfoUtils::WinVersionGreatOrEqual(10, 0, 18362))
//        {
//            typedef bool(WINAPI *ShouldSystemUseDarkMode_t)(); // ordinal 138
//            ShouldSystemUseDarkMode_t ShouldSystemUseDarkMode_f = (ShouldSystemUseDarkMode_t)(::GetProcAddress(library, MAKEINTRESOURCEA(138)));
//            if(ShouldSystemUseDarkMode_f)
//            {
//                bool result = !!ShouldSystemUseDarkMode_f();
//                ::FreeLibrary(library);
//                return result;
//            }
//        }

        // 1809 17763
        if(InfoUtils::WinVersionGreatOrEqual(10, 0, 17763))
        {
            typedef bool(WINAPI *ShouldAppsUseDarkMode_t)(); // ordinal 132
            ShouldAppsUseDarkMode_t ShouldAppsUseDarkMode_f = (ShouldAppsUseDarkMode_t)(::GetProcAddress(library, MAKEINTRESOURCEA(132)));
            if(ShouldAppsUseDarkMode_f)
            {
                bool result = !!ShouldAppsUseDarkMode_f();
                ::FreeLibrary(library);
                return result;
            }
        }

        ::FreeLibrary(library);
    }
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    const QPalette::ColorRole backgroundRole = QPalette::Window;
    const QPalette::ColorRole foregroundRole = QPalette::WindowText;
#else
    const QPalette::ColorRole backgroundRole = QPalette::Background;
    const QPalette::ColorRole foregroundRole = QPalette::Foreground;
#endif
    return qApp ? IsDarkTheme(qApp->palette(), backgroundRole, foregroundRole) : false;
}
#endif

/// @brief Создать масштабируемую иконку из нескольких разного размера
/// @param[in] defaultImagePath - Путь к иконке по умолчанию (может быть SVG)
/// @param[in] scaledImagePaths - Список путей иконкам разного размера (растр)
/// @return Масштабируемая иконка
QIcon CreateScalableIcon(const QString &defaultImagePath, const QStringList &scaledImagePaths)
{
    QIcon result(defaultImagePath);
    for(QStringList::ConstIterator it = scaledImagePaths.begin(), itEnd = scaledImagePaths.end(); it != itEnd; ++it)
    {
        const QPixmap pixmap(*it);
        if(!pixmap.isNull())
            AddPixmapToIcon(result, pixmap);
        else
            qWarning() << "[ThemeUtils::CreateScalableIcon]: Unable to load pixmap" << *it;
    }
    return result;
}

/// @brief Создать масштабируемую иконку из нескольких растровых разного размера
/// @param[in] scaledImagePaths - Список путей иконкам разного размера (только растр)
/// @param[in] invertPixels - Требуется ли инвертированное изображение вместо обычного
/// @return Масштабируемая иконка
QIcon CreateScalableIcon(const QStringList &scaledImagePaths, bool invertPixels)
{
    QIcon result;
    for(QStringList::ConstIterator it = scaledImagePaths.begin(), itEnd = scaledImagePaths.end(); it != itEnd; ++it)
    {
        QImage image(*it);
        if(!image.isNull())
        {
            if(invertPixels)
                image.invertPixels(QImage::InvertRgb);
            AddPixmapToIcon(result, QPixmap::fromImage(image));
        }
        else
        {
            qWarning() << "[ThemeUtils::CreateScalableIcon]: Unable to load image" << *it;
        }
    }
    return result;
}

/// @brief Функция для получения иконки
/// @param[in] type - Тип иконки (см. enum IconTypes)
/// @param[in] darkBackground - true, если иконка располагается на темном фоне
QIcon GetIcon(IconTypes type, bool darkBackground)
{
    const QString iconNameTemplate = QString::fromLatin1(":/icons/modern/%1_%2.png");
    switch(type)
    {
#define ADD_NAMED_ICON_CASE(ICON_TYPE, ICON_NAME) \
    case ICON_TYPE: \
    { \
        const QString iconName = QString::fromLatin1(ICON_NAME).toLower(); \
        const QString iconTemplate = iconNameTemplate.arg(iconName); \
        const QStringList rasterPixmaps = QStringList() << iconTemplate.arg(16) << iconTemplate.arg(32); \
        return CreateScalableIcon(rasterPixmaps, darkBackground); \
    }
#define ADD_ICON_CASE(ICON_TYPE) ADD_NAMED_ICON_CASE(ICON_TYPE, #ICON_TYPE)
    ADD_ICON_CASE(ICON_QT)
    ADD_NAMED_ICON_CASE(ICON_ABOUT, "icon_info")
    ADD_ICON_CASE(ICON_HELP)
    ADD_NAMED_ICON_CASE(ICON_AUTHORS, "icon_people")
    ADD_ICON_CASE(ICON_TEXT)
    ADD_ICON_CASE(ICON_SAVE)
    ADD_ICON_CASE(ICON_SAVE_AS)
    ADD_ICON_CASE(ICON_CLOSE)
    ADD_ICON_CASE(ICON_EXIT)
    ADD_ICON_CASE(ICON_PRINT)
    ADD_ICON_CASE(ICON_NEW)
    ADD_ICON_CASE(ICON_NEW_WINDOW)
    ADD_ICON_CASE(ICON_OPEN)
    ADD_ICON_CASE(ICON_CUT)
    ADD_ICON_CASE(ICON_COPY)
    ADD_ICON_CASE(ICON_PASTE)
    ADD_ICON_CASE(ICON_DELETE)
    ADD_ICON_CASE(ICON_ZOOM_IN)
    ADD_ICON_CASE(ICON_ZOOM_OUT)
    ADD_ICON_CASE(ICON_ZOOM_IDENTITY)
    ADD_ICON_CASE(ICON_ZOOM_EMPTY)
    ADD_ICON_CASE(ICON_ZOOM_CUSTOM)
    ADD_ICON_CASE(ICON_LEFT)
    ADD_ICON_CASE(ICON_RIGHT)
    ADD_ICON_CASE(ICON_ROTATE_CLOCKWISE)
    ADD_ICON_CASE(ICON_ROTATE_COUNTERCLOCKWISE)
    ADD_ICON_CASE(ICON_FLIP_HORIZONTAL)
    ADD_ICON_CASE(ICON_FLIP_VERTICAL)
    ADD_ICON_CASE(ICON_SETTINGS)
    ADD_ICON_CASE(ICON_FULLSCREEN)
    ADD_ICON_CASE(ICON_PLAY)
    ADD_ICON_CASE(ICON_STOP)
    ADD_ICON_CASE(ICON_RESET)
#undef ADD_ICON_CASE
#undef ADD_NAMED_ICON_CASE
    }
    return QIcon();
}

} // namespace ThemeUtils

