/*
   Copyright (C) 2021-2024 Peter S. Zhigalov <peter.zhigalov@gmail.com>

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

#include <cstring>

#include <Lerc_c_api.h>

#include <QFileInfo>
#include <QImage>
#include <QFile>
#include <QByteArray>
#include <QVector>

#include "Utils/Global.h"
#include "Utils/Logging.h"

#include "../IDecoder.h"
#include "Internal/DecoderAutoRegistrator.h"
#include "Internal/GraphicsItemsFactory.h"
#include "Internal/ImageData.h"
#include "Internal/ImageMetaData.h"
#include "Internal/PayloadWithMetaData.h"
#include "Internal/Utils/DataProcessing.h"
#include "Internal/Utils/MappedBuffer.h"

namespace
{

enum ErrCode
{
    LercNS_ErrCode_Ok = 0,
    LercNS_ErrCode_Failed,
    LercNS_ErrCode_WrongParam,
    LercNS_ErrCode_BufferTooSmall,
    LercNS_ErrCode_NaN
};

enum DataType
{
    LercNS_DataType_dt_char = 0,
    LercNS_DataType_dt_uchar,
    LercNS_DataType_dt_short,
    LercNS_DataType_dt_ushort,
    LercNS_DataType_dt_int,
    LercNS_DataType_dt_uint,
    LercNS_DataType_dt_float,
    LercNS_DataType_dt_double
};

enum InfoArrOrder
{
    LercNS_InfoArrOrder_version = 0,
    LercNS_InfoArrOrder_dataType,
    LercNS_InfoArrOrder_nDim,
    LercNS_InfoArrOrder_nCols,
    LercNS_InfoArrOrder_nRows,
    LercNS_InfoArrOrder_nBands,
    LercNS_InfoArrOrder_nValidPixels,  // for 1st band
    LercNS_InfoArrOrder_blobSize,
    LercNS_InfoArrOrder_nMasks  // 0 - all valid, 1 - same mask for all bands, nBands - masks can differ between bands
};

enum DataRangeArrOrder
{
    LercNS_DataRangeArrOrder_zMin = 0,
    LercNS_DataRangeArrOrder_zMax,
    LercNS_DataRangeArrOrder_maxZErrUsed
};

QString GetErrorStrng(lerc_status code)
{
    switch(static_cast<ErrCode>(code))
    {
    case LercNS_ErrCode_Ok:
        return QString::fromLatin1("Ok");
    case LercNS_ErrCode_Failed:
        return QString::fromLatin1("Failed");
    case LercNS_ErrCode_WrongParam:
        return QString::fromLatin1("WrongParam");
    case LercNS_ErrCode_BufferTooSmall:
        return QString::fromLatin1("BufferTooSmall");
    case LercNS_ErrCode_NaN:
        return QString::fromLatin1("NaN");
    default:
        break;
    }
    return QString::fromLatin1("Unknown");
}

template<typename T>
QImage decodeLercBlob(const QVector<unsigned int> &infoArr, const QVector<double> &dataRangeArr, const MappedBuffer &inBuffer)
{
    const unsigned int width = infoArr[LercNS_InfoArrOrder_nCols];
    const unsigned int height = infoArr[LercNS_InfoArrOrder_nRows];
    QImage result(width, height, QImage::Format_ARGB32);
    if(result.isNull())
    {
        LOG_WARNING() << LOGGING_CTX << "Invalid image size";
        return QImage();
    }

    const unsigned int dims = infoArr[LercNS_InfoArrOrder_nDim];
    const unsigned int bands = infoArr[LercNS_InfoArrOrder_nBands];
    QVector<T> zImg3(dims * bands * width * height, static_cast<T>(0));

    const unsigned int masks = infoArr[LercNS_InfoArrOrder_nMasks];
    QByteArray maskByteImg3(std::max<unsigned int>(masks, 1) * width * height, 0);

    const lerc_status decodeStatus = lerc_decode(inBuffer.dataAs<const unsigned char*>(),
                                                 inBuffer.sizeAs<unsigned int>(),
                                                 std::max<unsigned int>(masks, 1),
                                                 reinterpret_cast<unsigned char*>(maskByteImg3.data()),
                                                 dims, width, height, bands,
                                                 infoArr[LercNS_InfoArrOrder_dataType],
                                                 reinterpret_cast<void*>(zImg3.data()));
    if(decodeStatus)
    {
        LOG_WARNING() << LOGGING_CTX << QString::fromLatin1("lerc_decode(...) failed, ErrCode = %1 (%2)")
                .arg(decodeStatus)
                .arg(GetErrorStrng(decodeStatus))
                .toLocal8Bit().data();
        return QImage();
    }

    const double min = dataRangeArr[LercNS_DataRangeArrOrder_zMin];
    const double max = dataRangeArr[LercNS_DataRangeArrOrder_zMax];
    const double scale = (max - min) / 255.0;
    if(bands == 3 && dims == 1)
    {
        const unsigned int offset = width * height;
        for(unsigned int j = 0; j < height; ++j)
        {
            QRgb *rgb = reinterpret_cast<QRgb*>(result.scanLine(j));
            const unsigned int line = j * width;
            for(unsigned int i = 0; i < width; ++i)
            {
                const unsigned int pos = line + i;
                const quint8 r = DataProcessing::clampByte((zImg3[pos] - min) / scale);
                const quint8 g = DataProcessing::clampByte((zImg3[pos + offset] - min) / scale);
                const quint8 b = DataProcessing::clampByte((zImg3[pos + offset * 2] - min) / scale);
                const quint8 a = (masks == 1) ? DataProcessing::extractFromAlignedPtr<quint8>(maskByteImg3.data() + pos) : 0;
                rgb[i] = qRgba(r, g, b, 255 - a);
            }
        }
    }
    else if(bands == 1 && dims == 3)
    {
        for(unsigned int j = 0; j < height; ++j)
        {
            QRgb *rgb = reinterpret_cast<QRgb*>(result.scanLine(j));
            const unsigned int line = j * width;
            for(unsigned int i = 0; i < width; ++i)
            {
                const unsigned int pos = line + i;
                const quint8 r = DataProcessing::clampByte((zImg3[pos * dims] - min) / scale);
                const quint8 g = DataProcessing::clampByte((zImg3[pos * dims + 1] - min) / scale);
                const quint8 b = DataProcessing::clampByte((zImg3[pos * dims + 2] - min) / scale);
                const quint8 a = (masks == 1) ? DataProcessing::extractFromAlignedPtr<quint8>(maskByteImg3.data() + pos) : 0;
                rgb[i] = qRgba(r, g, b, 255 - a);
            }
        }
    }
    else
    {
        for(unsigned int j = 0; j < height; ++j)
        {
            QRgb *rgb = reinterpret_cast<QRgb*>(result.scanLine(j));
            const unsigned int line = j * width;
            for(unsigned int i = 0; i < width; ++i)
            {
                const unsigned int pos = line + i;
                const quint8 c = DataProcessing::clampByte((zImg3[pos * dims] - min) / scale);
                const quint8 a = masks ? DataProcessing::extractFromAlignedPtr<quint8>(maskByteImg3.data() + pos) : 0;
                rgb[i] = qRgba(c, c, c, 255 - a);
            }
        }
    }

    return result;
}

PayloadWithMetaData<QImage> readLercFile(const QString &filePath)
{
    const MappedBuffer inBuffer(filePath);
    if(!inBuffer.isValid())
        return QImage();

    QVector<unsigned int> infoArr(10, 0);
    QVector<double> dataRangeArr(3, 0.0);
    const lerc_status blobInfoStatus = lerc_getBlobInfo(inBuffer.dataAs<const unsigned char*>(),
                                                        inBuffer.sizeAs<unsigned int>(),
                                                        infoArr.data(), dataRangeArr.data(),
                                                        infoArr.size(), dataRangeArr.size());
    if(blobInfoStatus)
    {
        LOG_WARNING() << LOGGING_CTX << QString::fromLatin1("lerc_getBlobInfo(...) failed, ErrCode = %1 (%2)")
                .arg(blobInfoStatus)
                .arg(GetErrorStrng(blobInfoStatus))
                .toLocal8Bit().data();
        return QImage();
    }

    LOG_DEBUG() << LOGGING_CTX << "  version =" << infoArr[LercNS_InfoArrOrder_version];
    LOG_DEBUG() << LOGGING_CTX << "  dataType =" << infoArr[LercNS_InfoArrOrder_dataType];
    LOG_DEBUG() << LOGGING_CTX << "  nDim =" << infoArr[LercNS_InfoArrOrder_nDim];
    LOG_DEBUG() << LOGGING_CTX << "  nCols =" << infoArr[LercNS_InfoArrOrder_nCols];
    LOG_DEBUG() << LOGGING_CTX << "  nRows =" << infoArr[LercNS_InfoArrOrder_nRows];
    LOG_DEBUG() << LOGGING_CTX << "  nBands =" << infoArr[LercNS_InfoArrOrder_nBands];
    LOG_DEBUG() << LOGGING_CTX << "  nValidPixels =" << infoArr[LercNS_InfoArrOrder_nValidPixels];
    LOG_DEBUG() << LOGGING_CTX << "  blobSize =" << infoArr[LercNS_InfoArrOrder_blobSize];
    LOG_DEBUG() << LOGGING_CTX << "  nMasks =" << infoArr[LercNS_InfoArrOrder_nMasks];
    LOG_DEBUG() << LOGGING_CTX << "  zMin =" << dataRangeArr[LercNS_DataRangeArrOrder_zMin];
    LOG_DEBUG() << LOGGING_CTX << "  zMax =" << dataRangeArr[LercNS_DataRangeArrOrder_zMax];
    LOG_DEBUG() << LOGGING_CTX << "  maxZErrUsed =" << dataRangeArr[LercNS_DataRangeArrOrder_maxZErrUsed];

    QImage result;
    switch(infoArr[LercNS_InfoArrOrder_dataType])
    {
    case LercNS_DataType_dt_char:
        result = decodeLercBlob<signed char>(infoArr, dataRangeArr, inBuffer);
        break;
    case LercNS_DataType_dt_uchar:
        result = decodeLercBlob<unsigned char>(infoArr, dataRangeArr, inBuffer);
        break;
    case LercNS_DataType_dt_short:
        result = decodeLercBlob<signed short>(infoArr, dataRangeArr, inBuffer);
        break;
    case LercNS_DataType_dt_ushort:
        result = decodeLercBlob<unsigned short>(infoArr, dataRangeArr, inBuffer);
        break;
    case LercNS_DataType_dt_int:
        result = decodeLercBlob<signed int>(infoArr, dataRangeArr, inBuffer);
        break;
    case LercNS_DataType_dt_uint:
        result = decodeLercBlob<unsigned int>(infoArr, dataRangeArr, inBuffer);
        break;
    case LercNS_DataType_dt_float:
        result = decodeLercBlob<float>(infoArr, dataRangeArr, inBuffer);
        break;
    case LercNS_DataType_dt_double:
        result = decodeLercBlob<double>(infoArr, dataRangeArr, inBuffer);
        break;
    default:
        LOG_WARNING() << LOGGING_CTX << "Unknown dataType";
        return QImage();
    }

    ImageMetaData *metaData = ImageMetaData::createMetaData(inBuffer.dataAsByteArray());
    if(!metaData)
        metaData = new ImageMetaData;

    const QString type = QString::fromLatin1("LERC Info");

    metaData->addCustomEntry(type, QString::fromLatin1("version"),      QString::number(infoArr[LercNS_InfoArrOrder_version]));
    metaData->addCustomEntry(type, QString::fromLatin1("dataType"),     QString::number(infoArr[LercNS_InfoArrOrder_dataType]));
    metaData->addCustomEntry(type, QString::fromLatin1("nDim"),         QString::number(infoArr[LercNS_InfoArrOrder_nDim]));
    metaData->addCustomEntry(type, QString::fromLatin1("nCols"),        QString::number(infoArr[LercNS_InfoArrOrder_nCols]));
    metaData->addCustomEntry(type, QString::fromLatin1("nRows"),        QString::number(infoArr[LercNS_InfoArrOrder_nRows]));
    metaData->addCustomEntry(type, QString::fromLatin1("nBands"),       QString::number(infoArr[LercNS_InfoArrOrder_nBands]));
    metaData->addCustomEntry(type, QString::fromLatin1("nValidPixels"), QString::number(infoArr[LercNS_InfoArrOrder_nValidPixels]));
    metaData->addCustomEntry(type, QString::fromLatin1("blobSize"),     QString::number(infoArr[LercNS_InfoArrOrder_blobSize]));
    metaData->addCustomEntry(type, QString::fromLatin1("nMasks"),       QString::number(infoArr[LercNS_InfoArrOrder_nMasks]));

    metaData->addCustomEntry(type, QString::fromLatin1("zMin"),         QString::number(dataRangeArr[LercNS_DataRangeArrOrder_zMin]));
    metaData->addCustomEntry(type, QString::fromLatin1("zMax"),         QString::number(dataRangeArr[LercNS_DataRangeArrOrder_zMax]));
    metaData->addCustomEntry(type, QString::fromLatin1("maxZErrUsed"),  QString::number(dataRangeArr[LercNS_DataRangeArrOrder_maxZErrUsed]));

    return PayloadWithMetaData<QImage>(result, metaData);
}

class DecoderLERC : public IDecoder
{
public:
    QString name() const Q_DECL_OVERRIDE
    {
        return QString::fromLatin1("DecoderLERC");
    }

    QStringList supportedFormats() const Q_DECL_OVERRIDE
    {
        return QStringList()
                << QString::fromLatin1("lerc1")
                << QString::fromLatin1("lerc2");
    }

    QStringList advancedFormats() const Q_DECL_OVERRIDE
    {
        return QStringList();
    }

    bool isAvailable() const Q_DECL_OVERRIDE
    {
#if (Q_BYTE_ORDER == Q_LITTLE_ENDIAN)
        return true;
#else
        return false;
#endif
    }

    QSharedPointer<IImageData> loadImage(const QString &filePath) Q_DECL_OVERRIDE
    {
        const QFileInfo fileInfo(filePath);
        if(!fileInfo.exists() || !fileInfo.isReadable())
            return QSharedPointer<IImageData>();
        const PayloadWithMetaData<QImage> readData = readLercFile(filePath);
        QGraphicsItem *item = GraphicsItemsFactory::instance().createImageItem(readData);
        return QSharedPointer<IImageData>(new ImageData(item, filePath, name(), readData.metaData()));
    }
};

DecoderAutoRegistrator registrator(new DecoderLERC);

} // namespace
