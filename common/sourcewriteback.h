/*
 * Copyright (C) 2016 Christian Mollekopf <mollekopf@kolabsys.com>
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

#include "changereplay.h"
#include "storage.h"
#include "entitystore.h"
#include "remoteidmap.h"

namespace Sink {

/**
 * Replay changes to the source
 */
class SINK_EXPORT SourceWriteBack : public ChangeReplay
{
public:
    SourceWriteBack(const QByteArray &resourceType,const QByteArray &resourceInstanceIdentifier);

protected:
    ///Base implementation calls the replay$Type calls
    virtual KAsync::Job<void> replay(const QByteArray &type, const QByteArray &key, const QByteArray &value) Q_DECL_OVERRIDE;

protected:
    ///Implement to write back changes to the server
    virtual KAsync::Job<QByteArray> replay(const Sink::ApplicationDomain::Mail &, Sink::Operation, const QByteArray &oldRemoteId);
    virtual KAsync::Job<QByteArray> replay(const Sink::ApplicationDomain::Folder &, Sink::Operation, const QByteArray &oldRemoteId);

    //Read/Write access to sync storage
    RemoteIdMap &syncStore();

private:
    //Read only access to main storage
    EntityStore &store();

    Sink::Storage mSyncStorage;
    QSharedPointer<RemoteIdMap> mSyncStore;
    QSharedPointer<EntityStore> mEntityStore;
    Sink::Storage::Transaction mTransaction;
    Sink::Storage::Transaction mSyncTransaction;
    QByteArray mResourceType;
    QByteArray mResourceInstanceIdentifier;
};

}
