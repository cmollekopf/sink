/*
 * Copyright (c) 2016 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "sink_export.h"

namespace Sink {
namespace Flush {

enum FlushType {
    /**
     * Guarantees that any commands issued before this flush are written back to the server once this flush completes.
     * Note that this does not guarantee the success of writeback, only that an attempt has been made.
     */
    FlushReplayQueue,
    /**
     * Guarantees that any synchronization request issued before  this flush has been executed and that all entities created by it have been processed once this flush completes.
     */
    FlushSynchronization,
    /**
     * Guarantees that any modification issued before this flush has been processed once this flush completes.
     */
    FlushUserQueue
};

}
}

