/*
   Copyright (C) 2021 Peter S. Zhigalov <peter.zhigalov@gmail.com>

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

#include <windows.h>
#include <wincodec.h>

#include <QImage>
#include <QFileInfo>
#include <QDebug>
#include <QSettings>
#include <QVariant>
#include <QLibrary>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QFunctionPointer>
#else
typedef void* QFunctionPointer;
#endif

#include "Utils/Global.h"

#include "../IDecoder.h"
#include "Internal/DecoderAutoRegistrator.h"
#include "Internal/GraphicsItemsFactory.h"
#include "Internal/ImageData.h"
#include "Internal/ImageMetaData.h"
#include "Internal/Utils/LibraryUtils.h"

//#pragma comment(lib, "ole32.lib") // CoInitialize, CoUninitialize, CoCreateInstance
//#pragma comment(lib, "windowscodecs.lib")

namespace {

// ====================================================================================================

class OLE32
{
public:
    static OLE32 *instance()
    {
        static OLE32 _;
        if(!_.isValid())
        {
            qWarning() << "Failed to load ole32.dll";
            return Q_NULLPTR;
        }
        return &_;
    }

    HRESULT CoInitialize(LPVOID pvReserved)
    {
        typedef HRESULT(*CoInitialize_t)(LPVOID);
        CoInitialize_t CoInitialize_f = (CoInitialize_t)m_CoInitialize;
        return CoInitialize_f(pvReserved);
    }

    HRESULT CoUninitialize()
    {
        typedef HRESULT(*CoUninitialize_t)();
        CoUninitialize_t CoUninitialize_f = (CoUninitialize_t)m_CoUninitialize;
        return CoUninitialize_f();
    }

    HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
    {
        typedef HRESULT(*CoCreateInstance_t)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
        CoCreateInstance_t CoCreateInstance_f = (CoCreateInstance_t)m_CoCreateInstance;
        return CoCreateInstance_f(rclsid, pUnkOuter, dwClsContext, riid, ppv);
    }

    HRESULT IIDFromString(LPCOLESTR lpsz, LPIID lpiid)
    {
        typedef HRESULT(*IIDFromString_t)(LPCOLESTR, LPIID);
        IIDFromString_t IIDFromString_f = (IIDFromString_t)m_IIDFromString;
        return IIDFromString_f(lpsz, lpiid);
    }

private:
    OLE32()
        : m_CoInitialize(Q_NULLPTR)
        , m_CoUninitialize(Q_NULLPTR)
        , m_CoCreateInstance(Q_NULLPTR)
        , m_IIDFromString(Q_NULLPTR)
    {
        if(!LibraryUtils::LoadQLibrary(m_library, "ole32"))
            return;

        m_CoInitialize = m_library.resolve("CoInitialize");
        m_CoUninitialize = m_library.resolve("CoUninitialize");
        m_CoCreateInstance = m_library.resolve("CoCreateInstance");
        m_IIDFromString = m_library.resolve("IIDFromString");
    }

    ~OLE32()
    {}

    bool isValid() const
    {
        return m_library.isLoaded() && m_CoInitialize && m_CoUninitialize
                && m_CoCreateInstance && m_IIDFromString;
    }

    QLibrary m_library;
    QFunctionPointer m_CoInitialize;
    QFunctionPointer m_CoUninitialize;
    QFunctionPointer m_CoCreateInstance;
    QFunctionPointer m_IIDFromString;
};

HRESULT CoInitialize_WRAP(LPVOID pvReserved)
{
    if(OLE32 *ole32 = OLE32::instance())
        return ole32->CoInitialize(pvReserved);
    qWarning() << "Failed to load ole32.dll";
    return E_FAIL;
}
#define CoInitialize CoInitialize_WRAP

HRESULT CoUninitialize_WRAP()
{
    if(OLE32 *ole32 = OLE32::instance())
        return ole32->CoUninitialize();
    qWarning() << "Failed to load ole32.dll";
    return E_FAIL;
}
#define CoUninitialize CoUninitialize_WRAP

HRESULT CoCreateInstance_WRAP(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
    if(OLE32 *ole32 = OLE32::instance())
        return ole32->CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
    qWarning() << "Failed to load ole32.dll";
    return E_FAIL;
}
#define CoCreateInstance CoCreateInstance_WRAP

HRESULT IIDFromString_WRAP(LPCOLESTR lpsz, LPIID lpiid)
{
    if(OLE32 *ole32 = OLE32::instance())
        return ole32->IIDFromString(lpsz, lpiid);
    qWarning() << "Failed to load ole32.dll";
    return E_FAIL;
}
#define IIDFromString IIDFromString_WRAP

// ====================================================================================================

IID GetIID(LPCOLESTR lpsz)
{
    IID result;
    memset(&result, 0, sizeof(IID));
    HRESULT hr = IIDFromString(lpsz, &result);
    if(!SUCCEEDED(hr))
        qWarning() << "Can't get IID for" << QString::fromStdWString(lpsz);
    return result;
}

#define IID_IWICImagingFactory GetIID(L"{ec5ec8a9-c395-4314-9c77-54d7a935ff70}")

// ====================================================================================================

class DecoderWIC : public IDecoder
{
public:
    QString name() const Q_DECL_OVERRIDE
    {
        return QString::fromLatin1("DecoderWIC");
    }

    QStringList supportedFormats() const Q_DECL_OVERRIDE
    {
        if(!isAvailable())
            return QStringList();

        QStringList result = QStringList()
                // https://docs.microsoft.com/en-us/windows/win32/wic/bmp-format-overview
                << QString::fromLatin1("bmp")
                << QString::fromLatin1("dib")
                // https://docs.microsoft.com/en-us/windows/win32/wic/dds-format-overview
                << QString::fromLatin1("dds")
                // https://docs.microsoft.com/en-us/windows/win32/wic/dng-format-overview
                << QString::fromLatin1("dng")
                // https://docs.microsoft.com/en-us/windows/win32/wic/gif-format-overview
                << QString::fromLatin1("gif")
                // https://docs.microsoft.com/en-us/windows/win32/wic/hdphoto-format-overview
                << QString::fromLatin1("wdp")
                // https://docs.microsoft.com/en-us/windows/win32/wic/ico-format-overview
                << QString::fromLatin1("ico")
                // https://docs.microsoft.com/en-us/windows/win32/wic/jpeg-format-overview
                << QString::fromLatin1("jpe")
                << QString::fromLatin1("jpeg")
                << QString::fromLatin1("jpg")
                // https://docs.microsoft.com/en-us/windows/win32/wic/jpeg-xr-codec
                << QString::fromLatin1("jxr")
                << QString::fromLatin1("wdp")
                // https://docs.microsoft.com/en-us/windows/win32/wic/png-format-overview
                << QString::fromLatin1("png")
                // https://docs.microsoft.com/en-us/windows/win32/wic/tiff-format-overview
                << QString::fromLatin1("tiff")
                << QString::fromLatin1("tif");

        // https://github.com/ReneSlijkhuis/gimp-wic-plugin/blob/6afd01211334007963ff9a33f75d3b64759c5062/wic_library/Utilities/WicUtilities.cpp
        const QSettings settings(QString::fromLatin1("HKEY_CLASSES_ROOT\\CLSID\\{7ED96837-96F0-4812-B211-F13C24117ED3}\\Instance"), QSettings::NativeFormat);
        const QStringList wicCodecs = settings.childGroups();
        for(QStringList::ConstIterator it = wicCodecs.constBegin(); it != wicCodecs.constEnd(); ++it)
        {
            const QSettings settings(QString::fromLatin1("HKEY_CLASSES_ROOT\\CLSID\\%1").arg(*it), QSettings::NativeFormat);
            result.append(
                        settings.value(QString::fromLatin1("FileExtensions"))
                        .toString()
                        .toLower()
                        .remove(QChar::fromLatin1('.'))
                        .remove(QChar::fromLatin1(' '))
                        .split(QChar::fromLatin1(','))
                        );
        }

        result.removeDuplicates();
        return result;
    }

    QStringList advancedFormats() const Q_DECL_OVERRIDE
    {
        return QStringList();
    }

    bool isAvailable() const Q_DECL_OVERRIDE
    {
        return !!OLE32::instance();
    }

    QSharedPointer<IImageData> loadImage(const QString &filePath) Q_DECL_OVERRIDE
    {
        const QFileInfo fileInfo(filePath);
        if(!fileInfo.exists() || !fileInfo.isReadable())
            return QSharedPointer<IImageData>();

        CoInitialize(Q_NULLPTR);

        IWICImagingFactory *factory = Q_NULLPTR;
        if(FAILED(CoCreateInstance(CLSID_WICImagingFactory, Q_NULLPTR, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<LPVOID*>(&factory))))
        {
            qWarning() << "[DecoderWIC] Error: CoCreateInstance failed";
            CoUninitialize();
            return QSharedPointer<IImageData>();
        }

        IWICBitmapDecoder *decoder = Q_NULLPTR;
        if(FAILED(factory->CreateDecoderFromFilename(reinterpret_cast<LPCWSTR>(filePath.constData()), Q_NULLPTR, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder)))
        {
            qWarning() << "[DecoderWIC] Error: factory->CreateDecoderFromFilename failed";
            factory->Release();
            CoUninitialize();
            return QSharedPointer<IImageData>();
        }

        IWICBitmapFrameDecode *frame = Q_NULLPTR;
        if(FAILED(decoder->GetFrame(0, &frame)))
        {
            qWarning() << "[DecoderWIC] Error: decoder->GetFrame failed";
            decoder->Release();
            factory->Release();
            CoUninitialize();
            return QSharedPointer<IImageData>();
        }

        UINT width = 0;
        UINT height = 0;
        if(FAILED(frame->GetSize(&width, &height)))
        {
            qWarning() << "[DecoderWIC] Error: frame->GetSize failed";
            frame->Release();
            decoder->Release();
            factory->Release();
            CoUninitialize();
            return QSharedPointer<IImageData>();
        }

        IWICFormatConverter *formatConverter = Q_NULLPTR;
        if(FAILED(factory->CreateFormatConverter(&formatConverter)))
        {
            qWarning() << "[DecoderWIC] Error: factory->CreateFormatConverter failed";
            frame->Release();
            decoder->Release();
            factory->Release();
            CoUninitialize();
            return QSharedPointer<IImageData>();
        }

        if(FAILED(formatConverter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, Q_NULLPTR, 0.0f, WICBitmapPaletteTypeCustom)))
        {
            qWarning() << "[DecoderWIC] Error: formatConverter->Initialize failed";
            formatConverter->Release();
            frame->Release();
            decoder->Release();
            factory->Release();
            CoUninitialize();
            return QSharedPointer<IImageData>();
        }

        QImage image(width, height, QImage::Format_ARGB32);
        if(image.isNull())
        {
            qWarning() << "[DecoderWIC] Error: invalid image size";
            formatConverter->Release();
            frame->Release();
            decoder->Release();
            factory->Release();
            CoUninitialize();
            return QSharedPointer<IImageData>();
        }

        if(FAILED(formatConverter->CopyPixels(Q_NULLPTR, static_cast<UINT>(image.bytesPerLine()), static_cast<UINT>(image.bytesPerLine() * image.height()), reinterpret_cast<BYTE*>(image.bits()))))
        {
            qWarning() << "[DecoderWIC] Error: formatConverter->CopyPixels failed";
            formatConverter->Release();
            frame->Release();
            decoder->Release();
            factory->Release();
            CoUninitialize();
            return QSharedPointer<IImageData>();
        }

        formatConverter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        CoUninitialize();

        ImageMetaData *metaData = ImageMetaData::createMetaData(filePath);
        if(metaData)
            metaData->applyExifOrientation(&image);

        QGraphicsItem *item = GraphicsItemsFactory::instance().createImageItem(image);
        return QSharedPointer<IImageData>(new ImageData(item, filePath, name(), metaData));
    }
};

DecoderAutoRegistrator registrator(new DecoderWIC, false);

// ====================================================================================================

} // namespace