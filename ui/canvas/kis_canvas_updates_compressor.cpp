/*
 *  Copyright (c) 2015 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_canvas_updates_compressor.h"

bool KisCanvasUpdatesCompressor::putUpdateInfo(KisUpdateInfoSP info)
{
    const int levelOfDetail = info->levelOfDetail();
    const QRect newUpdateRect = info->dirtyImageRect();
    if (newUpdateRect.isEmpty()) return false;

    QMutexLocker l(&m_mutex);
    UpdateInfoList::iterator it = m_updatesList.begin();

    while (it != m_updatesList.end()) {
        if (levelOfDetail == (*it)->levelOfDetail() &&
            newUpdateRect.contains((*it)->dirtyImageRect())) {

            /**
             * We should always remove the overridden update and put 'info' to the end
             * of the queue. Otherwise, the updates will become reordered and the canvas
             * may have tiles artifacts with "outdated" data
             */
            it = m_updatesList.erase(it);
        } else {
            ++it;
        }
    }

    m_updatesList.append(info);

    return m_updatesList.size() <= 1;
}

KisUpdateInfoSP KisCanvasUpdatesCompressor::takeUpdateInfo()
{
    QMutexLocker l(&m_mutex);
    return !m_updatesList.isEmpty() ? m_updatesList.takeFirst() : 0;
}
