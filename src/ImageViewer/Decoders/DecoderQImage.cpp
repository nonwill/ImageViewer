/*
   Copyright (C) 2017 Peter S. Zhigalov <peter.zhigalov@gmail.com>

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

#include "DecoderQImage.h"

#include <set>

#include <QImageReader>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QFileInfo>

#include "DecoderAutoRegistrator.h"
#include "ExifUtils.h"

#define DECODER_QIMAGE_PRIORITY 100

namespace {

DecoderAutoRegistrator registrator(new DecoderQImage, 100);

} // namespace

QString DecoderQImage::name() const
{
    return QString::fromLatin1("DecoderQImage");
}

QList<DecoderFormatInfo> DecoderQImage::supportedFormats() const
{
    // https://doc.qt.io/archives/qtextended4.4/qimagereader.html#supportedImageFormats
    const QStringList defaultReaderFormats = QStringList()
            << QString::fromLatin1("bmp")
            << QString::fromLatin1("jpg")
            << QString::fromLatin1("jpeg")
//            << QString::fromLatin1("mng")
            << QString::fromLatin1("png")
            << QString::fromLatin1("pbm")
            << QString::fromLatin1("pgm")
            << QString::fromLatin1("ppm")
//            << QString::fromLatin1("tiff")
            << QString::fromLatin1("xbm")
            << QString::fromLatin1("xpm");
    const QList<QByteArray> readerFormats = QImageReader::supportedImageFormats();
    std::set<QString> allReaderFormats;
    for(QList<QByteArray>::ConstIterator it = readerFormats.constBegin(); it != readerFormats.constEnd(); ++it)
        allReaderFormats.insert(QString::fromLatin1(*it).toLower());
    for(QStringList::ConstIterator it = defaultReaderFormats.constBegin(); it != defaultReaderFormats.constEnd(); ++it)
        allReaderFormats.insert(it->toLower());
    QList<DecoderFormatInfo> result;
    for(std::set<QString>::const_iterator it = allReaderFormats.begin(); it != allReaderFormats.end(); ++it)
    {
        DecoderFormatInfo info;
        info.decoderPriority = DECODER_QIMAGE_PRIORITY;
        info.format = *it;
        result.append(info);
    }
    return result;
}

QGraphicsItem *DecoderQImage::loadImage(const QString &filename)
{
    const QFileInfo fileInfo(filename);
    if(!fileInfo.exists() || !fileInfo.isReadable())
        return NULL;
    QImageReader imageReader(filename);
    imageReader.setBackgroundColor(Qt::transparent);
    imageReader.setQuality(100);
    QImage image;

    quint16 orientation = ExifUtils::GetExifOrientation(filename);
    if(orientation != 1)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
        imageReader.setAutoTransform(false);
#endif
        image = imageReader.read();
        if(!image.isNull())
            ExifUtils::ApplyExifOrientation(&image, orientation);
    }
    else
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
        imageReader.setAutoTransform(true);
#endif
        image = imageReader.read();
    }

    if(image.isNull())
        return NULL;

    return new QGraphicsPixmapItem(QPixmap::fromImage(image));
}