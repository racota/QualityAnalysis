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

#include "KisAsyncAnimationRendererBase.h"

#include <QTimer>
#include <QThread>

#include "kis_image.h"
#include "kis_image_animation_interface.h"
#include "kis_signal_auto_connection.h"

struct KisAsyncAnimationRendererBase::Private
{

    KisSignalAutoConnectionsStore imageRequestConnections;
    QTimer regenerationTimeout;

    KisImageSP requestedImage;
    int requestedFrame = -1;
    bool isCancelled = false;

    static const int WAITING_FOR_FRAME_TIMEOUT = 10000;
};

KisAsyncAnimationRendererBase::KisAsyncAnimationRendererBase(QObject *parent)
    : QObject(parent),
      m_d(new Private())
{
    connect(&m_d->regenerationTimeout, SIGNAL(timeout()), SLOT(slotFrameRegenerationCancelled()));
    m_d->regenerationTimeout.setSingleShot(true);
    m_d->regenerationTimeout.setInterval(Private::WAITING_FOR_FRAME_TIMEOUT);
}

KisAsyncAnimationRendererBase::~KisAsyncAnimationRendererBase()
{

}

void KisAsyncAnimationRendererBase::startFrameRegeneration(KisImageSP image, int frame)
{
    KIS_SAFE_ASSERT_RECOVER_NOOP(QThread::currentThread() == this->thread());

    m_d->requestedImage = image;
    m_d->requestedFrame = frame;
    m_d->isCancelled = false;

    KisImageAnimationInterface *animation = m_d->requestedImage->animationInterface();

    m_d->imageRequestConnections.clear();
    m_d->imageRequestConnections.addConnection(
                animation, SIGNAL(sigFrameReady(int)),
                this, SLOT(slotFrameRegenerationFinished(int)),
                Qt::DirectConnection);

    m_d->imageRequestConnections.addConnection(
                animation, SIGNAL(sigFrameCancelled()),
                this, SLOT(slotFrameRegenerationCancelled()),
                Qt::AutoConnection);

    m_d->regenerationTimeout.start();
    animation->requestFrameRegeneration(m_d->requestedFrame, image->bounds());
}

bool KisAsyncAnimationRendererBase::isActive() const
{
    return m_d->requestedImage;
}

void KisAsyncAnimationRendererBase::cancelCurrentFrameRendering()
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->requestedImage);
    frameCancelledCallback(m_d->requestedFrame);
}

void KisAsyncAnimationRendererBase::slotFrameRegenerationCancelled()
{
    // the timeout can arrive in async way
    if (!m_d->requestedImage) return;
    frameCancelledCallback(m_d->requestedFrame);
}

void KisAsyncAnimationRendererBase::slotFrameRegenerationFinished(int frame)
{
    // We might have already cancelled the regeneration. We don't check
    // isCancelled flag here because this code runs asynchronously.
    if (!m_d->requestedImage) return;

    // WARNING: executed in the context of image worker thread!

    // probably a bit too strict...
    KIS_SAFE_ASSERT_RECOVER_NOOP(QThread::currentThread() != this->thread());

    frameCompletedCallback(frame);
}

void KisAsyncAnimationRendererBase::notifyFrameCompleted(int frame)
{
    KIS_SAFE_ASSERT_RECOVER_NOOP(QThread::currentThread() == this->thread());

    // the image events can come with a delay, even after
    // the processing was cancelled
    if (m_d->isCancelled) return;

    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->requestedImage);
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->requestedFrame == frame);

    clearFrameRegenerationState(false);
    emit sigFrameCompleted(frame);
}

void KisAsyncAnimationRendererBase::notifyFrameCancelled(int frame)
{
    KIS_SAFE_ASSERT_RECOVER_NOOP(QThread::currentThread() == this->thread());

    // the image events can come with a delay, even after
    // the processing was cancelled
    if (m_d->isCancelled) return;

    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->requestedImage);
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->requestedFrame == frame);

    clearFrameRegenerationState(true);
    emit sigFrameCancelled(frame);
}

void KisAsyncAnimationRendererBase::clearFrameRegenerationState(bool isCancelled)
{
    // TODO: for some reason we mark the process as cancelled in any case, and it
    //       seem to be a correct behavior
    Q_UNUSED(isCancelled);

    m_d->imageRequestConnections.clear();
    m_d->requestedImage = 0;
    m_d->requestedFrame = -1;
    m_d->regenerationTimeout.stop();
    m_d->isCancelled = true;
}

KisImageSP KisAsyncAnimationRendererBase::requestedImage() const
{
    return m_d->requestedImage;
}


