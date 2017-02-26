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

#if !defined(IDECODER_H_INCLUDED)
#define IDECODER_H_INCLUDED

#include <QGraphicsItem>
#include <QString>
#include <QList>

struct DecoderFormatInfo
{
    QString format;
    int decoderPriority;
};

class IDecoder
{
public:
    virtual ~IDecoder() {}
    virtual QString name() const = 0;
    virtual QList<DecoderFormatInfo> supportedFormats() const = 0;
    virtual QGraphicsItem *loadImage(const QString &filename) = 0;
};

#endif