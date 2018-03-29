/*
 *   Copyright (C) 2018 Christian Mollekopf <chrigi_1@fastmail.fm>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#pragma once

#include "synchronizer.h"

#include <KDAV2/DavCollection>
#include <KDAV2/DavItem>
#include <KDAV2/DavUrl>

class WebDavSynchronizer : public Sink::Synchronizer
{
public:
    WebDavSynchronizer(const Sink::ResourceContext &, KDAV2::Protocol, QByteArray collectionName,
        QByteArray itemName);

    QList<Synchronizer::SyncRequest> getSyncRequests(const Sink::QueryBase &query) Q_DECL_OVERRIDE;
    KAsync::Job<void> synchronizeWithSource(const Sink::QueryBase &query) Q_DECL_OVERRIDE;

protected:
    /**
     * Called with the list of discovered collections. It's purpose should be
     * adding the said collections to the store.
     */
    virtual void updateLocalCollections(KDAV2::DavCollection::List collections) = 0;

    /**
     * Called when discovering a new item, or when an item has been modified.
     * It's purpose should be adding the said item to the store.
     *
     * `collectionLocalRid` is the local resource id of the collection the item
     * is in.
     */
    virtual void updateLocalItem(KDAV2::DavItem item, const QByteArray &collectionLocalRid) = 0;

    /**
     * Get the local resource id from a collection.
     */
    virtual QByteArray collectionLocalResourceID(const KDAV2::DavCollection &collection) = 0;

    KAsync::Job<void> synchronizeCollection(const KDAV2::DavCollection &,
        QSharedPointer<int> progress, QSharedPointer<int> total, QSharedPointer<QSet<QByteArray>> itemsResourceIDs);
    KAsync::Job<void> synchronizeItem(const KDAV2::DavItem &, const QByteArray &collectionLocalRid,
        QSharedPointer<int> progress, QSharedPointer<int> total);

    static QByteArray resourceID(const KDAV2::DavCollection &);
    static QByteArray resourceID(const KDAV2::DavItem &);

    bool unchanged(const KDAV2::DavCollection &);
    bool unchanged(const KDAV2::DavItem &);

    KDAV2::DavUrl serverUrl() const;

private:
    KDAV2::Protocol protocol;
    const QByteArray collectionName;
    const QByteArray itemName;

    QUrl server;
    QString username;
};