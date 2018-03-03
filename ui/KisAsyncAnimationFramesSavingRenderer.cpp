/*
 *  Copyright (c) 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "KisAsyncAnimationFramesSavingRenderer.h"

#include "kis_image.h"
#include "kis_paint_device.h"
#include "KisImportExportFilter.h"
#include "KisPart.h"
#include "KisDocument.h"
#include "kis_time_range.h"
#include "kis_paint_layer.h"


struct KisAsyncAnimationFramesSavingRenderer::Private
{
    Private(KisImageSP image, const KisTimeRange &_range, int _sequenceNumberingOffset, KisPropertiesConfigurationSP _exportConfiguration)
        : savingDoc(KisPart::instance()->createDocument()),
          range(_range),
          sequenceNumberingOffset(_sequenceNumberingOffset),
          exportConfiguration(_exportConfiguration)
    {

        savingDoc->setAutoSaveDelay(0);
        savingDoc->setFileBatchMode(true);

        KisImageSP savingImage = new KisImage(savingDoc->createUndoStore(),
                                              image->bounds().width(),
                                              image->bounds().height(),
                                              image->colorSpace(),
                                              QString());

        savingImage->setResolution(image->xRes(), image->yRes());
        savingDoc->setCurrentImage(savingImage);

        KisPaintLayer* paintLayer = new KisPaintLayer(savingImage, "paint device", 255);
        savingImage->addNode(paintLayer, savingImage->root(), KisLayerSP(0));

        savingDevice = paintLayer->paintDevice();
    }

    QScopedPointer<KisDocument> savingDoc;
    KisPaintDeviceSP savingDevice;

    KisTimeRange range;
    int sequenceNumberingOffset = 0;


    QString filenamePrefix;
    QString filenameSuffix;

    QByteArray outputMimeType;
    KisPropertiesConfigurationSP exportConfiguration;
};

KisAsyncAnimationFramesSavingRenderer::KisAsyncAnimationFramesSavingRenderer(KisImageSP image,
                                                                             const QString &fileNamePrefix,
                                                                             const QString &fileNameSuffix,
                                                                             const QByteArray &outputMimeType,
                                                                             const KisTimeRange &range,
                                                                             const int sequenceNumberingOffset,
                                                                             KisPropertiesConfigurationSP exportConfiguration)
    : m_d(new Private(image, range, sequenceNumberingOffset, exportConfiguration))
{
    m_d->filenamePrefix = fileNamePrefix;
    m_d->filenameSuffix = fileNameSuffix;
    m_d->outputMimeType = outputMimeType;

    connect(this, SIGNAL(sigCompleteRegenerationInternal(int)), SLOT(notifyFrameCompleted(int)));
    connect(this, SIGNAL(sigCancelRegenerationInternal(int)), SLOT(notifyFrameCancelled(int)));
}




KisAsyncAnimationFramesSavingRenderer::~KisAsyncAnimationFramesSavingRenderer()
{
}

void KisAsyncAnimationFramesSavingRenderer::frameCompletedCallback(int frame)
{
    KisImageSP image = requestedImage();
    if (!image) return;

    m_d->savingDevice->makeCloneFromRough(image->projection(), image->bounds());

    KisTimeRange range(frame, 1);

    KisImportExportFilter::ConversionStatus status = KisImportExportFilter::OK;

    for (int i = range.start(); i <= range.end(); i++) {
        QString frameNumber = QString("%1").arg(i - m_d->range.start() + m_d->sequenceNumberingOffset, 4, 10, QChar('0'));
        QString filename = m_d->filenamePrefix + frameNumber + m_d->filenameSuffix;

        if (!m_d->savingDoc->exportDocumentSync(QUrl::fromLocalFile(filename), m_d->outputMimeType, m_d->exportConfiguration)) {
            status = KisImportExportFilter::InternalError;
            break;
        }
    }

    if (status == KisImportExportFilter::OK) {
        emit sigCompleteRegenerationInternal(frame);
    } else {
        emit sigCancelRegenerationInternal(frame);
    }
}

void KisAsyncAnimationFramesSavingRenderer::frameCancelledCallback(int frame)
{
    notifyFrameCancelled(frame);
}

