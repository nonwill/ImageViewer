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

#include "PrintDialog.h"
#include "PrintDialog_p.h"

#include <cmath>

#include <QDebug>
#include <QFileInfo>
#include <QPrintDialog>
#include <QPageSetupDialog>
#include <QPrinter>
#include <QPrinterInfo>

#include "Utils/SignalBlocker.h"

struct PrintDialog::Impl
{
    const QList<QPrinterInfo> availablePrinters;
    QGraphicsItem * const graphicsItem;
    const int rotateAngle;
    const Qt::Orientations flipOrientations;
    const QString filePath;
    QScopedPointer<QPrinter> printer;
    QRectF itemPrintRect;

    Impl(QGraphicsItem *graphicsItem, int rotateAngle, const Qt::Orientations &flipOrientations, const QString &filePath)
        : availablePrinters(QPrinterInfo::availablePrinters())
        , graphicsItem(graphicsItem)
        , rotateAngle(rotateAngle)
        , flipOrientations(flipOrientations)
        , filePath(filePath)
    {}

    QRectF itemBounds() const
    {
        if(!graphicsItem)
            return QRectF();
        const QRectF boundingRect = graphicsItem->boundingRect().normalized();
        const QPointF center = boundingRect.center();
        const QTransform transform = QTransform().translate(center.x(), center.y()).rotate(rotateAngle).translate(-center.x(), -center.y());
        return transform.mapRect(boundingRect);
    }

    Qt::Orientation itemOrientation() const
    {
        if(!graphicsItem)
            return Qt::Vertical;
        const QRectF boundingRect = itemBounds();
        return boundingRect.height() >= boundingRect.width() ? Qt::Vertical : Qt::Horizontal;
    }

    QSizeF itemDpi() const
    {
        /// @todo
        return QSizeF(72, 72);
    }

    QSizeF itemSize(QPrinter::Unit unit) const
    {
        const QSizeF bounds = itemBounds().size();
        const QSizeF dpi = itemDpi();
        return QSizeF(convert(bounds.width() / dpi.width(), QPrinter::Inch, unit), convert(bounds.height() / dpi.height(), QPrinter::Inch, unit));
    }

    QSizeF availableItemSize(QPrinter::Unit unit, bool ignoreMargins, bool keepAspect) const
    {
        if(!printer)
            return QSizeF();
        const QRectF paperRect = printer->paperRect(unit);
        const QRectF pageRect = printer->pageRect(unit);
        const QRectF availableRect = ignoreMargins ? paperRect : pageRect;
        QSizeF availableSize = availableRect.size();
        if(keepAspect)
        {
            QSizeF fixedAvailableSize = itemSize(unit);
            fixedAvailableSize.scale(availableSize, Qt::KeepAspectRatio);
            availableSize = fixedAvailableSize;
        }
        return availableSize;
    }

    QSizeF preferredItemSize(QPrinter::Unit unit, bool ignoreMargins) const
    {
        const QSizeF availableSize = availableItemSize(unit, ignoreMargins, true);
        const QSizeF originalSize = itemSize(unit);
        if(originalSize.width() <= availableSize.width() && originalSize.height() <= availableSize.height())
            return originalSize;
        return availableSize;
    }

    double convert(double value, QPrinter::Unit from, QPrinter::Unit to) const
    {
        const int resolution = printer ? printer->resolution() : 1;
        return value * multiplierForUnit(from, resolution) / multiplierForUnit(to, resolution);
    }

    QPointF convert(QPointF value, QPrinter::Unit from, QPrinter::Unit to) const
    {
        return QPointF(convert(value.x(), from, to), convert(value.y(), from, to));
    }

    QSizeF convert(QSizeF value, QPrinter::Unit from, QPrinter::Unit to) const
    {
        return QSizeF(convert(value.width(), from, to), convert(value.height(), from, to));
    }

    QRectF convert(QRectF value, QPrinter::Unit from, QPrinter::Unit to) const
    {
        return QRectF(convert(value.topLeft(), from, to), convert(value.size(), from, to));
    }

private:
    static double multiplierForUnit(QPrinter::Unit unit, int resolution)
    {
        switch(unit)
        {
        case QPrinter::Millimeter:
            return 2.83464566929;
        case QPrinter::Point:
            return 1.0;
        case QPrinter::Inch:
            return 72.0;
        case QPrinter::Pica:
            return 12;
        case QPrinter::Didot:
            return 1.065826771;
        case QPrinter::Cicero:
            return 12.789921252;
        case QPrinter::DevicePixel:
            return 72.0/resolution;
        }
        return 1.0;
    }
};

PrintDialog::PrintDialog(QGraphicsItem *graphicsItem,
                         int rotateAngle,
                         const Qt::Orientations &flipOrientations,
                         const QString &filePath,
                         QWidget *parent)
    : QDialog(parent)
    , m_ui(new UI(this))
    , m_impl(new Impl(graphicsItem, rotateAngle, flipOrientations, filePath))
{
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
#if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
                   Qt::WindowCloseButtonHint |
#endif
                   Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint);
    setWindowTitle(qApp->translate("PrintDialog", "Print", "Title"));
    setWindowModality(Qt::ApplicationModal);

    m_ui->copiesSpinBox->setMinimum(1);
    m_ui->copiesSpinBox->setMaximum(999);

    m_ui->sizeUnitsComboBox->addItem(qApp->translate("PrintDialog", "Millimeter", "Size unit"), static_cast<int>(QPrinter::Millimeter));
    m_ui->sizeUnitsComboBox->addItem(qApp->translate("PrintDialog", "Point"     , "Size unit"), static_cast<int>(QPrinter::Point));
    m_ui->sizeUnitsComboBox->addItem(qApp->translate("PrintDialog", "Inch"      , "Size unit"), static_cast<int>(QPrinter::Inch));
    m_ui->sizeUnitsComboBox->addItem(qApp->translate("PrintDialog", "Pica"      , "Size unit"), static_cast<int>(QPrinter::Pica));
    m_ui->sizeUnitsComboBox->addItem(qApp->translate("PrintDialog", "Didot"     , "Size unit"), static_cast<int>(QPrinter::Didot));
    m_ui->sizeUnitsComboBox->addItem(qApp->translate("PrintDialog", "Cicero"    , "Size unit"), static_cast<int>(QPrinter::Cicero));
    m_ui->sizeUnitsComboBox->setCurrentIndex(m_ui->sizeUnitsComboBox->findData(static_cast<int>(QPrinter::Millimeter)));

    m_ui->resolutionUnitsComboBox->addItem(qApp->translate("PrintDialog", "Pixels/Millimeter", "Resolution unit"), static_cast<int>(QPrinter::Millimeter));
    m_ui->resolutionUnitsComboBox->addItem(qApp->translate("PrintDialog", "Pixels/Point"     , "Resolution unit"), static_cast<int>(QPrinter::Point));
    m_ui->resolutionUnitsComboBox->addItem(qApp->translate("PrintDialog", "Pixels/Inch"      , "Resolution unit"), static_cast<int>(QPrinter::Inch));
    m_ui->resolutionUnitsComboBox->addItem(qApp->translate("PrintDialog", "Pixels/Pica"      , "Resolution unit"), static_cast<int>(QPrinter::Pica));
    m_ui->resolutionUnitsComboBox->addItem(qApp->translate("PrintDialog", "Pixels/Didot"     , "Resolution unit"), static_cast<int>(QPrinter::Didot));
    m_ui->resolutionUnitsComboBox->addItem(qApp->translate("PrintDialog", "Pixels/Cicero"    , "Resolution unit"), static_cast<int>(QPrinter::Cicero));
    m_ui->resolutionUnitsComboBox->setCurrentIndex(m_ui->resolutionUnitsComboBox->findData(static_cast<int>(QPrinter::Inch)));

    m_ui->centerComboBox->addItem(qApp->translate("PrintDialog", "None"        , "Centering option"), static_cast<int>(0));
    m_ui->centerComboBox->addItem(qApp->translate("PrintDialog", "Horizontally", "Centering option"), static_cast<int>(Qt::Horizontal));
    m_ui->centerComboBox->addItem(qApp->translate("PrintDialog", "Vertically"  , "Centering option"), static_cast<int>(Qt::Vertical));
    m_ui->centerComboBox->addItem(qApp->translate("PrintDialog", "Both"        , "Centering option"), static_cast<int>(Qt::Horizontal | Qt::Vertical));
    m_ui->centerComboBox->setCurrentIndex(m_ui->centerComboBox->findData(static_cast<int>(Qt::Horizontal | Qt::Vertical)));

    m_ui->previewWidget->setGraphicsItem(graphicsItem, rotateAngle, flipOrientations);
    m_ui->keepAspectCheckBox->setChecked(true);
    m_ui->ignorePageMarginsCheckBox->setChecked(false);

    m_ui->widthSpinBox->setDecimals(3);
    m_ui->heightSpinBox->setDecimals(3);
    m_ui->xResolutionSpinBox->setDecimals(3);
    m_ui->yResolutionSpinBox->setDecimals(3);
    m_ui->leftSpinBox->setDecimals(3);
    m_ui->rightSpinBox->setDecimals(3);
    m_ui->topSpinBox->setDecimals(3);
    m_ui->bottomSpinBox->setDecimals(3);

    const QList<QPrinterInfo> printers = m_impl->availablePrinters;
    for(QList<QPrinterInfo>::ConstIterator it = printers.begin(); it != printers.end(); ++it)
    {
        m_ui->printerSelectComboBox->addItem(it->printerName());
        if(it->isDefault())
            m_ui->printerSelectComboBox->setCurrentIndex(m_ui->printerSelectComboBox->count() - 1);
    }
    onCurrentPrinterChanged(m_ui->printerSelectComboBox->currentIndex());

    m_ui->autoRotateCheckBox->setChecked(true);
    onAutoRotateStateChanged();

    connect(m_ui->printerSelectComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentPrinterChanged(int)));
    connect(m_ui->pageSetupButton, SIGNAL(clicked()), this, SLOT(onPageSetupClicked()));
    connect(m_ui->autoRotateCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onAutoRotateStateChanged()));
    connect(m_ui->portraitRadioButton, SIGNAL(toggled(bool)), this, SLOT(onPortraitToggled(bool)));
    connect(m_ui->landscapeRadioButton, SIGNAL(toggled(bool)), this, SLOT(onLandscapeToggled(bool)));
    connect(m_ui->copiesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onNumCopiesChanged(int)));
    connect(m_ui->colorModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onColorModeChanged(int)));
    connect(m_ui->widthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onWidthChanged(double)));
    connect(m_ui->heightSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onHeightChanged(double)));
    connect(m_ui->sizeUnitsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onSizeUnitsChanged(int)));
    connect(m_ui->xResolutionSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onXResolutionChanged(double)));
    connect(m_ui->yResolutionSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onYResolutionChanged(double)));
    connect(m_ui->keepAspectCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onKeepAspectStateChanged()));
    connect(m_ui->resolutionUnitsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onResolutionUnitsChanged(int)));
    connect(m_ui->loadDefaultsButton, SIGNAL(clicked()), this, SLOT(onLoadDefaultsClicked()));
    connect(m_ui->leftSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onLeftChanged(double)));
    connect(m_ui->rightSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onRightChanged(double)));
    connect(m_ui->topSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onTopChanged(double)));
    connect(m_ui->bottomSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onBottomChanged(double)));
    connect(m_ui->centerComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onCenterChanged(int)));
    connect(m_ui->ignorePageMarginsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onIgnorePageMarginsStateChanged()));
    connect(m_ui->dialogButtonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onPrintClicked()));
    connect(m_ui->dialogButtonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(close()));

    ensurePolished();
    adjustSize();
    setFixedSize(minimumSize());
}

PrintDialog::~PrintDialog()
{}

void PrintDialog::onCurrentPrinterChanged(int index)
{
    m_impl->printer.reset();
    updatePrinterInfo(QPrinterInfo());
    updatePageInfo();
    m_ui->dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    if(index < 0 || index >= m_impl->availablePrinters.size())
        return;

    const QPrinterInfo& info = m_impl->availablePrinters[index];
    m_impl->printer.reset(new QPrinter(info, QPrinter::HighResolution));
    QPageSetupDialog(m_impl->printer.data(), Q_NULLPTR).accept();
    QPrintDialog(m_impl->printer.data(), Q_NULLPTR).accept();

    m_impl->printer->setDocName(QFileInfo(m_impl->filePath).fileName());
    m_impl->printer->setCreator(qApp->applicationName() + QString::fromLatin1(" ") + qApp->applicationVersion());

    if(true)
    {
        const QSignalBlocker guard(m_ui->copiesSpinBox);
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
        m_ui->copiesSpinBox->setValue(m_impl->printer->copyCount());
        m_ui->copiesSpinBox->setEnabled(m_impl->printer->supportsMultipleCopies());
#else
        m_ui->copiesSpinBox->setValue(m_impl->printer->numCopies());
#endif
    }
    if(true)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
        const QList<QPrinter::ColorMode> supportedColorModes = info.supportedColorModes();
#else
        const QList<QPrinter::ColorMode> supportedColorModes = QList<QPrinter::ColorMode>() << QPrinter::Color << QPrinter::GrayScale;
#endif
        const QSignalBlocker guard(m_ui->colorModeComboBox);
        m_ui->colorModeComboBox->clear();
        if(supportedColorModes.contains(QPrinter::Color))
            m_ui->colorModeComboBox->addItem(qApp->translate("PrintDialog", "Color", "Color mode"), static_cast<int>(QPrinter::Color));
        if(supportedColorModes.contains(QPrinter::GrayScale))
            m_ui->colorModeComboBox->addItem(qApp->translate("PrintDialog", "Grayscale", "Color mode"), static_cast<int>(QPrinter::GrayScale));
        m_ui->colorModeComboBox->setCurrentIndex(m_ui->colorModeComboBox->findData(static_cast<int>(m_impl->printer->colorMode())));
    }

    updatePrinterInfo(info);
    updatePageInfo();
    m_ui->dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(m_impl->graphicsItem);
}

void PrintDialog::onPrintClicked()
{
    if(!m_impl->graphicsItem || !m_impl->printer)
        return;

    QPainter painter(m_impl->printer.data());
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const QRectF itemRect = m_impl->convert(m_impl->itemPrintRect, QPrinter::Point, QPrinter::DevicePixel);
    const QRectF boundingRect = m_impl->graphicsItem->boundingRect();
    const QRectF rotatedBoundingRect = QTransform()
            .translate(boundingRect.center().x(), boundingRect.center().y())
            .rotate(m_impl->rotateAngle)
            .translate(-boundingRect.center().x(), -boundingRect.center().y())
            .mapRect(boundingRect);
    painter.translate(itemRect.x(), itemRect.y());
    painter.scale(itemRect.width() / rotatedBoundingRect.width(), itemRect.height() / rotatedBoundingRect.height());
    painter.translate(-rotatedBoundingRect.x(), -rotatedBoundingRect.y());
    painter.translate(rotatedBoundingRect.center().x(), rotatedBoundingRect.center().y());
    painter.rotate(m_impl->rotateAngle);
    painter.translate(-rotatedBoundingRect.center().x(), -rotatedBoundingRect.center().y());
    if(m_impl->flipOrientations & Qt::Horizontal)
    {
        painter.scale(-1, 1);
        painter.translate(-boundingRect.width(), 0);
    }
    if(m_impl->flipOrientations & Qt::Vertical)
    {
        painter.scale(1, -1);
        painter.translate(0, -boundingRect.height());
    }
    QStyleOptionGraphicsItem options;
    options.exposedRect = boundingRect;
    m_impl->graphicsItem->paint(&painter, &options);
    painter.end();

    close();
}

void PrintDialog::onPageSetupClicked()
{
    if(!m_impl->graphicsItem || !m_impl->printer)
        return;

    QPageSetupDialog *dialog = new QPageSetupDialog(m_impl->printer.data(), this);
    if(dialog->exec() == QDialog::Accepted)
        updatePageInfo();
    dialog->hide();
    dialog->deleteLater();
}

void PrintDialog::onAutoRotateStateChanged()
{
    const bool autoRotate = m_ui->autoRotateCheckBox->checkState() == Qt::Checked;
    m_ui->portraitRadioButton->setDisabled(autoRotate);
    m_ui->landscapeRadioButton->setDisabled(autoRotate);
    updatePageInfo();
}

void PrintDialog::onPortraitToggled(bool checked)
{
    if(!checked || !m_impl->printer)
        return;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
    m_impl->printer->setPageOrientation(QPageLayout::Portrait);
#else
    m_impl->printer->setOrientation(QPrinter::Portrait);
#endif
    updatePageInfo();
}

void PrintDialog::onLandscapeToggled(bool checked)
{
    if(!checked || !m_impl->printer)
        return;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
    m_impl->printer->setPageOrientation(QPageLayout::Landscape);
#else
    m_impl->printer->setOrientation(QPrinter::Landscape);
#endif
    updatePageInfo();
}

void PrintDialog::onNumCopiesChanged(int value)
{
    if(!m_impl->printer)
        return;
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
    m_impl->printer->setCopyCount(value);
#else
    m_impl->printer->setNumCopies(value);
#endif
    const QSignalBlocker guard(m_ui->copiesSpinBox);
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
    m_ui->copiesSpinBox->setValue(m_impl->printer->copyCount());
#else
    m_ui->copiesSpinBox->setValue(m_impl->printer->numCopies());
#endif
}

void PrintDialog::onColorModeChanged(int index)
{
    if(!m_impl->printer)
        return;
    m_impl->printer->setColorMode(static_cast<QPrinter::ColorMode>(m_ui->colorModeComboBox->itemData(index).toInt()));
    const QSignalBlocker guard(m_ui->colorModeComboBox);
    m_ui->colorModeComboBox->setCurrentIndex(m_ui->colorModeComboBox->findData(static_cast<int>(m_impl->printer->colorMode())));
}

void PrintDialog::onWidthChanged(double value)
{
    const QPrinter::Unit sizeUnit = static_cast<QPrinter::Unit>(m_ui->sizeUnitsComboBox->itemData(m_ui->sizeUnitsComboBox->currentIndex()).toInt());
    const bool keepAspect = m_ui->keepAspectCheckBox->isChecked();
    m_impl->itemPrintRect.setWidth(m_impl->convert(value, sizeUnit, QPrinter::Point));
    if(keepAspect)
    {
        const QSizeF originalSize = m_impl->itemBounds().size();
        const qreal aspect = originalSize.width() / originalSize.height();
        m_impl->itemPrintRect.setHeight(m_impl->itemPrintRect.width() / aspect);
    }
    updateImageGeometry();
}

void PrintDialog::onHeightChanged(double value)
{
    const QPrinter::Unit sizeUnit = static_cast<QPrinter::Unit>(m_ui->sizeUnitsComboBox->itemData(m_ui->sizeUnitsComboBox->currentIndex()).toInt());
    const bool keepAspect = m_ui->keepAspectCheckBox->isChecked();
    m_impl->itemPrintRect.setHeight(m_impl->convert(value, sizeUnit, QPrinter::Point));
    if(keepAspect)
    {
        const QSizeF originalSize = m_impl->itemBounds().size();
        const qreal aspect = originalSize.width() / originalSize.height();
        m_impl->itemPrintRect.setWidth(m_impl->itemPrintRect.height() * aspect);
    }
    updateImageGeometry();
}

void PrintDialog::onSizeUnitsChanged(int index)
{
    Q_UNUSED(index);
    updateImageGeometry();
}

void PrintDialog::onXResolutionChanged(double value)
{
    const QPrinter::Unit resolutionUnit = static_cast<QPrinter::Unit>(m_ui->resolutionUnitsComboBox->itemData(m_ui->resolutionUnitsComboBox->currentIndex()).toInt());
    const QSizeF originalSize = m_impl->itemBounds().size();
    const bool keepAspect = m_ui->keepAspectCheckBox->isChecked();
    m_impl->itemPrintRect.setWidth(m_impl->convert(originalSize.width() / value, resolutionUnit, QPrinter::Point));
    if(keepAspect)
    {
        const qreal aspect = originalSize.width() / originalSize.height();
        m_impl->itemPrintRect.setHeight(m_impl->itemPrintRect.width() / aspect);
    }
    updateImageGeometry();
}

void PrintDialog::onYResolutionChanged(double value)
{
    const QPrinter::Unit resolutionUnit = static_cast<QPrinter::Unit>(m_ui->resolutionUnitsComboBox->itemData(m_ui->resolutionUnitsComboBox->currentIndex()).toInt());
    const QSizeF originalSize = m_impl->itemBounds().size();
    const bool keepAspect = m_ui->keepAspectCheckBox->isChecked();
    m_impl->itemPrintRect.setHeight(m_impl->convert(originalSize.height() / value, resolutionUnit, QPrinter::Point));
    if(keepAspect)
    {
        const qreal aspect = originalSize.width() / originalSize.height();
        m_impl->itemPrintRect.setWidth(m_impl->itemPrintRect.height() * aspect);
    }
    updateImageGeometry();
}

void PrintDialog::onKeepAspectStateChanged()
{
    updateImageGeometry();
}

void PrintDialog::onResolutionUnitsChanged(int index)
{
    Q_UNUSED(index);
    updateImageGeometry();
}

void PrintDialog::onLoadDefaultsClicked()
{
    m_impl->itemPrintRect = QRectF();
    updateImageGeometry();
}

void PrintDialog::onLeftChanged(double value)
{
    const QPrinter::Unit sizeUnit = static_cast<QPrinter::Unit>(m_ui->sizeUnitsComboBox->itemData(m_ui->sizeUnitsComboBox->currentIndex()).toInt());
    m_impl->itemPrintRect.moveLeft(m_impl->convert(value, sizeUnit, QPrinter::Point));
    int centeringOrientation = m_ui->centerComboBox->itemData(m_ui->centerComboBox->currentIndex()).toInt();
    if(centeringOrientation & Qt::Horizontal)
    {
        centeringOrientation &= ~Qt::Horizontal;
        const QSignalBlocker guard(m_ui->centerComboBox);
        m_ui->centerComboBox->setCurrentIndex(m_ui->centerComboBox->findData(centeringOrientation));
    }
    updateImageGeometry();
}

void PrintDialog::onRightChanged(double value)
{
    const QPrinter::Unit sizeUnit = static_cast<QPrinter::Unit>(m_ui->sizeUnitsComboBox->itemData(m_ui->sizeUnitsComboBox->currentIndex()).toInt());
    m_impl->itemPrintRect.moveRight(m_impl->convert(value, sizeUnit, QPrinter::Point));
    int centeringOrientation = m_ui->centerComboBox->itemData(m_ui->centerComboBox->currentIndex()).toInt();
    if(centeringOrientation & Qt::Horizontal)
    {
        centeringOrientation &= ~Qt::Horizontal;
        const QSignalBlocker guard(m_ui->centerComboBox);
        m_ui->centerComboBox->setCurrentIndex(m_ui->centerComboBox->findData(centeringOrientation));
    }
    updateImageGeometry();
}

void PrintDialog::onTopChanged(double value)
{
    const QPrinter::Unit sizeUnit = static_cast<QPrinter::Unit>(m_ui->sizeUnitsComboBox->itemData(m_ui->sizeUnitsComboBox->currentIndex()).toInt());
    m_impl->itemPrintRect.moveTop(m_impl->convert(value, sizeUnit, QPrinter::Point));
    int centeringOrientation = m_ui->centerComboBox->itemData(m_ui->centerComboBox->currentIndex()).toInt();
    if(centeringOrientation & Qt::Vertical)
    {
        centeringOrientation &= ~Qt::Vertical;
        const QSignalBlocker guard(m_ui->centerComboBox);
        m_ui->centerComboBox->setCurrentIndex(m_ui->centerComboBox->findData(centeringOrientation));
    }
    updateImageGeometry();
}

void PrintDialog::onBottomChanged(double value)
{
    const QPrinter::Unit sizeUnit = static_cast<QPrinter::Unit>(m_ui->sizeUnitsComboBox->itemData(m_ui->sizeUnitsComboBox->currentIndex()).toInt());
    m_impl->itemPrintRect.moveBottom(m_impl->convert(value, sizeUnit, QPrinter::Point));
    int centeringOrientation = m_ui->centerComboBox->itemData(m_ui->centerComboBox->currentIndex()).toInt();
    if(centeringOrientation & Qt::Vertical)
    {
        centeringOrientation &= ~Qt::Vertical;
        const QSignalBlocker guard(m_ui->centerComboBox);
        m_ui->centerComboBox->setCurrentIndex(m_ui->centerComboBox->findData(centeringOrientation));
    }
    updateImageGeometry();
}

void PrintDialog::onCenterChanged(int index)
{
    Q_UNUSED(index);
    updateImageGeometry();
}

void PrintDialog::onIgnorePageMarginsStateChanged()
{
    updateImageGeometry();
}

void PrintDialog::updatePrinterInfo(const QPrinterInfo& info)
{
    m_ui->printerNameLabel->setText(info.printerName());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    m_ui->printerDescriptionLabel->setText(info.description());
#else
    m_ui->printerDescriptionHeaderLabel->setEnabled(false);
    m_ui->printerDescriptionLabel->setEnabled(false);
#endif

    if(info.isDefault())
        m_ui->printerDefaultLabel->setText(qApp->translate("PrintDialog", "Yes", "Default"));
    else
        m_ui->printerDefaultLabel->setText(qApp->translate("PrintDialog", "No", "Default"));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
    if(info.isRemote())
        m_ui->printerRemoteLabel->setText(qApp->translate("PrintDialog", "Yes", "Remote"));
    else
        m_ui->printerRemoteLabel->setText(qApp->translate("PrintDialog", "No", "Remote"));
#else
    m_ui->printerRemoteHeaderLabel->setEnabled(false);
    m_ui->printerRemoteLabel->setEnabled(false);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    m_ui->printerLocationLabel->setText(info.location());
#else
    m_ui->printerLocationHeaderLabel->setEnabled(false);
    m_ui->printerLocationLabel->setEnabled(false);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    m_ui->printerMakeAndModelLabel->setText(info.makeAndModel());
#else
    m_ui->printerMakeAndModelHeaderLabel->setEnabled(false);
    m_ui->printerMakeAndModelLabel->setEnabled(false);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
    switch(info.state())
    {
    case QPrinter::Idle:
        m_ui->printerStateLabel->setText(qApp->translate("PrintDialog", "Idle", "State"));
        break;
    case QPrinter::Active:
        m_ui->printerStateLabel->setText(qApp->translate("PrintDialog", "Active", "State"));
        break;
    case QPrinter::Aborted:
        m_ui->printerStateLabel->setText(qApp->translate("PrintDialog", "Aborted", "State"));
        break;
    case QPrinter::Error:
        m_ui->printerStateLabel->setText(qApp->translate("PrintDialog", "Error", "State"));
        break;
    default:
        m_ui->printerStateLabel->setText(qApp->translate("PrintDialog", "Unknown (%1)", "State").arg(static_cast<int>(info.state())));
        break;
    }
#else
    m_ui->printerStateHeaderLabel->setEnabled(false);
    m_ui->printerStateLabel->setEnabled(false);
#endif
}

void PrintDialog::updatePageInfo()
{
    m_impl->itemPrintRect = QRectF();
    updatePageOrientation();
    updateImageGeometry();
}

void PrintDialog::updatePageOrientation()
{
    if(!m_impl->printer)
        return;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
    QPageLayout::Orientation orientation;
    const QPageLayout::Orientation portrait = QPageLayout::Portrait;
    const QPageLayout::Orientation landscape = QPageLayout::Landscape;
#else
    QPrinter::Orientation orientation;
    const QPrinter::Orientation portrait = QPrinter::Portrait;
    const QPrinter::Orientation landscape = QPrinter::Landscape;
#endif

    const bool autoRotate = m_ui->autoRotateCheckBox->checkState() == Qt::Checked;
    if(autoRotate)
    {
        orientation = (m_impl->itemOrientation() == Qt::Vertical) ? portrait : landscape;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
        m_impl->printer->setPageOrientation(orientation);
#else
        m_impl->printer->setOrientation(orientation);
#endif
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
    orientation = m_impl->printer->pageLayout().orientation();
#else
    orientation = m_impl->printer->orientation();
#endif

    const QSignalBlocker portraitRadioButtonBlocker(m_ui->portraitRadioButton);
    const QSignalBlocker landscapeRadioButtonBlocker(m_ui->landscapeRadioButton);
    m_ui->portraitRadioButton->setChecked(orientation == portrait);
    m_ui->landscapeRadioButton->setChecked(orientation == landscape);
}

void PrintDialog::updateImageGeometry()
{
    if(!m_impl->printer || !m_impl->graphicsItem)
    {
        m_ui->imageSettingsTabFrame->setEnabled(false);
        return;
    }

    m_ui->imageSettingsTabFrame->setEnabled(true);
    const QSignalBlocker widthSpinBoxGuard(m_ui->widthSpinBox);
    const QSignalBlocker heightSpinBoxGuard(m_ui->heightSpinBox);
    const QSignalBlocker xResolutionSpinBoxGuard(m_ui->xResolutionSpinBox);
    const QSignalBlocker yResolutionSpinBoxGuard(m_ui->yResolutionSpinBox);
    const QSignalBlocker leftSpinBoxGuard(m_ui->leftSpinBox);
    const QSignalBlocker rightSpinBoxGuard(m_ui->rightSpinBox);
    const QSignalBlocker topSpinBoxGuard(m_ui->topSpinBox);
    const QSignalBlocker bottomSpinBoxGuard(m_ui->bottomSpinBox);

    const bool ignoreMargins = m_ui->ignorePageMarginsCheckBox->isChecked();
    const bool keepAspect = m_ui->keepAspectCheckBox->isChecked();
    const int centering = m_ui->centerComboBox->itemData(m_ui->centerComboBox->currentIndex()).toInt();

    const QRectF paperRect = m_impl->printer->paperRect(QPrinter::Point);
    const QRectF pageRect = m_impl->printer->pageRect(QPrinter::Point);
    const QRectF availableRect = ignoreMargins ? paperRect : pageRect;
    const QSizeF preferredSize = m_impl->preferredItemSize(QPrinter::Point, ignoreMargins);

    if(m_impl->itemPrintRect.isEmpty())
    {
        m_impl->itemPrintRect = QRectF(QPointF(0, 0), preferredSize);
        const QPointF center = availableRect.center();
        const QPointF topLeft = availableRect.topLeft() + m_impl->itemPrintRect.center();
        QPointF targetCenter = topLeft;
        if(centering & Qt::Horizontal)
            targetCenter.setX(center.x());
        if(centering & Qt::Vertical)
            targetCenter.setY(center.y());
        m_impl->itemPrintRect.moveCenter(targetCenter);
    }

    if(m_impl->itemPrintRect.width() > availableRect.width() || m_impl->itemPrintRect.height() > availableRect.height())
    {
        QSizeF targetSize = m_impl->itemPrintRect.size();
        targetSize.scale(availableRect.size(), keepAspect ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio);
        targetSize = QSizeF(qMin(targetSize.width(), m_impl->itemPrintRect.width()), qMin(targetSize.height(), m_impl->itemPrintRect.height()));
        const QSizeF delta = m_impl->itemPrintRect.size() - targetSize;
        QPointF correction;
        if(centering & Qt::Horizontal)
            correction.setX(delta.width() / 2);
        if(centering & Qt::Vertical)
            correction.setY(delta.height() / 2);
        m_impl->itemPrintRect = QRectF(m_impl->itemPrintRect.topLeft() + correction, targetSize);
    }

    if(keepAspect && !qFuzzyCompare(preferredSize.width() / preferredSize.height(), m_impl->itemPrintRect.width() / m_impl->itemPrintRect.height()))
    {
        QSizeF targetSize = preferredSize;
        targetSize.scale(m_impl->itemPrintRect.size(), Qt::KeepAspectRatio);
        const QSizeF delta = m_impl->itemPrintRect.size() - targetSize;
        QPointF correction;
        if(centering & Qt::Horizontal)
            correction.setX(delta.width() / 2);
        if(centering & Qt::Vertical)
            correction.setY(delta.height() / 2);
        m_impl->itemPrintRect = QRectF(m_impl->itemPrintRect.topLeft() + correction, targetSize);
    }

    if(centering & Qt::Horizontal && !qFuzzyCompare(m_impl->itemPrintRect.center().x(), availableRect.center().x()))
        m_impl->itemPrintRect.moveCenter(QPointF(availableRect.center().x(), m_impl->itemPrintRect.center().y()));
    if(centering & Qt::Vertical && !qFuzzyCompare(m_impl->itemPrintRect.center().y(), availableRect.center().y()))
        m_impl->itemPrintRect.moveCenter(QPointF(m_impl->itemPrintRect.center().x(), availableRect.center().y()));

    if(m_impl->itemPrintRect.right() > availableRect.right())
        m_impl->itemPrintRect.moveRight(availableRect.right());
    if(m_impl->itemPrintRect.bottom() > availableRect.bottom())
        m_impl->itemPrintRect.moveBottom(availableRect.bottom());
    if(m_impl->itemPrintRect.top() < availableRect.top())
        m_impl->itemPrintRect.moveTop(availableRect.top());
    if(m_impl->itemPrintRect.left() < availableRect.left())
        m_impl->itemPrintRect.moveLeft(availableRect.left());

    const QPrinter::Unit sizeUnit = static_cast<QPrinter::Unit>(m_ui->sizeUnitsComboBox->itemData(m_ui->sizeUnitsComboBox->currentIndex()).toInt());
    m_ui->widthSpinBox->setMinimum(m_impl->convert(std::pow(10.0, static_cast<double>(-m_ui->widthSpinBox->decimals())), QPrinter::Point, sizeUnit));
    m_ui->widthSpinBox->setMaximum(m_impl->convert(availableRect.width(), QPrinter::Point, sizeUnit));
    m_ui->widthSpinBox->setValue(m_impl->convert(m_impl->itemPrintRect.width(), QPrinter::Point, sizeUnit));
    m_ui->heightSpinBox->setMinimum(m_impl->convert(std::pow(10.0, static_cast<double>(-m_ui->heightSpinBox->decimals())), QPrinter::Point, sizeUnit));
    m_ui->heightSpinBox->setMaximum(m_impl->convert(availableRect.height(), QPrinter::Point, sizeUnit));
    m_ui->heightSpinBox->setValue(m_impl->convert(m_impl->itemPrintRect.height(), QPrinter::Point, sizeUnit));
    m_ui->leftSpinBox->setMinimum(m_impl->convert(availableRect.left(), QPrinter::Point, sizeUnit));
    m_ui->leftSpinBox->setMaximum(m_impl->convert(availableRect.right() - m_impl->itemPrintRect.width(), QPrinter::Point, sizeUnit));
    m_ui->leftSpinBox->setValue(m_impl->convert(m_impl->itemPrintRect.left(), QPrinter::Point, sizeUnit));
    m_ui->rightSpinBox->setMinimum(m_impl->convert(availableRect.left() + m_impl->itemPrintRect.width(), QPrinter::Point, sizeUnit));
    m_ui->rightSpinBox->setMaximum(m_impl->convert(availableRect.right(), QPrinter::Point, sizeUnit));
    m_ui->rightSpinBox->setValue(m_impl->convert(m_impl->itemPrintRect.right(), QPrinter::Point, sizeUnit));
    m_ui->topSpinBox->setMinimum(m_impl->convert(availableRect.top(), QPrinter::Point, sizeUnit));
    m_ui->topSpinBox->setMaximum(m_impl->convert(availableRect.bottom() - m_impl->itemPrintRect.height(), QPrinter::Point, sizeUnit));
    m_ui->topSpinBox->setValue(m_impl->convert(m_impl->itemPrintRect.top(), QPrinter::Point, sizeUnit));
    m_ui->bottomSpinBox->setMinimum(m_impl->convert(availableRect.top() + m_impl->itemPrintRect.height(), QPrinter::Point, sizeUnit));
    m_ui->bottomSpinBox->setMaximum(m_impl->convert(availableRect.bottom(), QPrinter::Point, sizeUnit));
    m_ui->bottomSpinBox->setValue(m_impl->convert(m_impl->itemPrintRect.bottom(), QPrinter::Point, sizeUnit));

    const QPrinter::Unit resolutionUnit = static_cast<QPrinter::Unit>(m_ui->resolutionUnitsComboBox->itemData(m_ui->resolutionUnitsComboBox->currentIndex()).toInt());
    const QSizeF originalSize = m_impl->itemBounds().size();
    m_ui->xResolutionSpinBox->setMinimum(originalSize.width() / m_impl->convert(m_ui->widthSpinBox->maximum(), sizeUnit, resolutionUnit));
    m_ui->xResolutionSpinBox->setMaximum(originalSize.width() / m_impl->convert(m_ui->widthSpinBox->minimum(), sizeUnit, resolutionUnit));
    m_ui->xResolutionSpinBox->setValue(originalSize.width() / m_impl->convert(m_impl->itemPrintRect.width(), QPrinter::Point, resolutionUnit));
    m_ui->yResolutionSpinBox->setMinimum(originalSize.height() / m_impl->convert(m_ui->heightSpinBox->maximum(), sizeUnit, resolutionUnit));
    m_ui->yResolutionSpinBox->setMaximum(originalSize.height() / m_impl->convert(m_ui->heightSpinBox->minimum(), sizeUnit, resolutionUnit));
    m_ui->yResolutionSpinBox->setValue(originalSize.height() / m_impl->convert(m_impl->itemPrintRect.height(), QPrinter::Point, resolutionUnit));

    m_ui->previewWidget->setPaperRect(m_impl->printer->paperRect(QPrinter::Point));
    m_ui->previewWidget->setPageRect(m_impl->printer->pageRect(QPrinter::Point));
    m_ui->previewWidget->setItemRect(m_impl->itemPrintRect);
}