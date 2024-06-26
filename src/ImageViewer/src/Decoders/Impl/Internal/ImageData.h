/*
   Copyright (C) 2018-2021 Peter S. Zhigalov <peter.zhigalov@gmail.com>

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

#if !defined(IMAGEDATA_H_INCLUDED)
#define IMAGEDATA_H_INCLUDED

#include <QtGlobal>
#include <QString>
#include <QSize>

#include "Utils/Global.h"

#include "../../IImageData.h"

class ImageData : public IImageData
{
    Q_DISABLE_COPY(ImageData)

public:
    ImageData();
    ImageData(QGraphicsItem *graphicsItem, const QString &filePath, const QString &decoderName, IImageMetaData *metaData = Q_NULLPTR);
    ~ImageData();

    bool isEmpty() const Q_DECL_OVERRIDE;

    QGraphicsItem *graphicsItem() const Q_DECL_OVERRIDE;
    QString decoderName() const Q_DECL_OVERRIDE;
    QString filePath() const Q_DECL_OVERRIDE;
    QSize size() const Q_DECL_OVERRIDE;
    QPair<qreal, qreal> dpi() const Q_DECL_OVERRIDE;
    IImageMetaData *metaData() const Q_DECL_OVERRIDE;

private:
    QGraphicsItem * const m_graphicsItem;
    const QString m_decoderName;
    const QString m_filePath;
    const QSize m_size;
    const QPair<qreal, qreal> m_dpi;
    IImageMetaData * const m_metaData;
};

#endif // IMAGEDATA_H_INCLUDED
