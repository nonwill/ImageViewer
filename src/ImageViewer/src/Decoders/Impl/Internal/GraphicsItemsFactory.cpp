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

#include "GraphicsItemsFactory.h"

#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QImage>
#include <QPixmap>

#include "Utils/Global.h"

#include "Animation/IAnimationProvider.h"
#include "Animation/AnimationWidget.h"
#include "Animation/AnimationObject.h"
#include "Scaling/AbstractProgressiveImageProvider.h"
#include "Scaling/IScaledImageProvider.h"
#include "GraphicsItems/ProgressiveResampledImageGraphicsItem.h"
#include "GraphicsItems/RasterizedImageGraphicsItem.h"
#include "GraphicsItems/ResampledImageGraphicsItem.h"

GraphicsItemsFactory &GraphicsItemsFactory::instance()
{
    static GraphicsItemsFactory factory;
    return factory;
}

QGraphicsItem *GraphicsItemsFactory::createImageItem(const QImage &image)
{
    if(image.isNull())
        return Q_NULLPTR;
    return new ResampledImageGraphicsItem(image);
}

QGraphicsItem *GraphicsItemsFactory::createPixmapItem(const QPixmap &pixmap)
{
    if(pixmap.isNull())
        return Q_NULLPTR;
    return new ResampledImageGraphicsItem(pixmap);
}

QGraphicsItem *GraphicsItemsFactory::createProgressiveImageItem(AbstractProgressiveImageProvider *progressiveImageProvider)
{
    if(!progressiveImageProvider || !progressiveImageProvider->isValid())
    {
        if(progressiveImageProvider)
            delete progressiveImageProvider;
        return Q_NULLPTR;
    }
    return new ProgressiveResampledImageGraphicsItem(progressiveImageProvider);
}

QGraphicsItem *GraphicsItemsFactory::createAnimatedItem(IAnimationProvider *animationProvider)
{
    if(!animationProvider || !animationProvider->isValid())
    {
        if(animationProvider)
            delete animationProvider;
        return Q_NULLPTR;
    }

    if(animationProvider->isSingleFrame())
    {
        const QImage image = animationProvider->currentImage();
        delete animationProvider;
        return createImageItem(image);
    }

    AnimationWidget *widget = new AnimationWidget();
    widget->setStyleSheet(QString::fromLatin1("background: transparent; border: none; margin: 0px 0px 0px 0px; padding: 0px 0px 0px 0px;"));
    widget->setAnimationProvider(animationProvider);
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget();
    proxy->setWidget(widget);
    return proxy;
}

QGraphicsItem *GraphicsItemsFactory::createScalableItem(IScaledImageProvider *scaledImageProvider)
{
    if(!scaledImageProvider || !scaledImageProvider->isValid())
    {
        if(scaledImageProvider)
            delete scaledImageProvider;
        return Q_NULLPTR;
    }
    return new RasterizedImageGraphicsItem(scaledImageProvider);
}

GraphicsItemsFactory::GraphicsItemsFactory()
{}

GraphicsItemsFactory::~GraphicsItemsFactory()
{}
