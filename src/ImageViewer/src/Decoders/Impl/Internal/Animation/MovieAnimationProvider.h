/*
   Copyright (C) 2017-2024 Peter S. Zhigalov <peter.zhigalov@gmail.com>

   This file is part of the `ImageViewer' program.

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

#if !defined(MOVIE_ANIMATION_PROVIDER_H_INCLUDED)
#define MOVIE_ANIMATION_PROVIDER_H_INCLUDED

#include <QImage>
#include <QPixmap>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
#include <QColorSpace>
#endif

#include "Utils/Global.h"

#include "IAnimationProvider.h"
#include "../Utils/CmsUtils.h"

template<typename Movie>
class MovieAnimationProvider : public IAnimationProvider
{
    Q_DISABLE_COPY(MovieAnimationProvider)

public:
    explicit MovieAnimationProvider(Movie *movie)
        : m_movie(movie)
    {
        m_movie->setParent(Q_NULLPTR);
        m_movie->start();
        m_movie->stop();
    }

    ~MovieAnimationProvider()
    {
        m_movie->deleteLater();
    }

    bool isValid() const Q_DECL_OVERRIDE
    {
        return m_movie->isValid();
    }

    bool isSingleFrame() const Q_DECL_OVERRIDE
    {
        return m_movie->frameCount() == 1;
    }

    int nextImageDelay() const Q_DECL_OVERRIDE
    {
        return m_movie->nextFrameDelay();
    }

    bool jumpToNextImage() Q_DECL_OVERRIDE
    {
        return m_movie->jumpToNextFrame();
    }

    QPixmap currentPixmap() const Q_DECL_OVERRIDE
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        return QPixmap::fromImage(currentImage());
#else
        QPixmap result = m_movie->currentPixmap();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        /// @note Supress '@2x' logic: https://github.com/qt/qtbase/blob/v5.9.8/src/gui/image/qimagereader.cpp#L1364
        result.setDevicePixelRatio(1);
#endif
        return result;
#endif
    }

    QImage currentImage() const Q_DECL_OVERRIDE
    {
        QImage result = m_movie->currentImage();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        /// @note Supress '@2x' logic: https://github.com/qt/qtbase/blob/v5.9.8/src/gui/image/qimagereader.cpp#L1364
        result.setDevicePixelRatio(1);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
        if(result.colorSpace().isValid())
            result.convertToColorSpace(QColorSpace::SRgb, result.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32);
        if(result.format() == QImage::Format_CMYK8888)
            ICCProfile(ICCProfile::defaultCmykProfileData()).applyToImage(&result);
#elif (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        if(result.colorSpace().isValid())
            result.convertToColorSpace(QColorSpace::SRgb);
#endif
        return result;
    }

protected:
    Movie *m_movie;
};

#endif // MOVIE_ANIMATION_PROVIDER_H_INCLUDED
