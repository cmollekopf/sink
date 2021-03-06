/*
 *   Copyright (C) 2017 Christian Mollekopf <mollekopf@kolabsys.com>
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

//xapian.h needs to be included first to build
#include <xapian.h>

#include <QDebug>
#include <QObject> // tr()
#include <QFile>

#include "common/resource.h"
#include "common/storage.h"
#include "common/resourceconfig.h"
#include "common/log.h"
#include "common/storage.h"
#include "common/definitions.h"
#include "common/entitybuffer.h"
#include "common/metadata_generated.h"
#include "common/bufferutils.h"

#include "sinksh_utils.h"
#include "state.h"
#include "syntaxtree.h"

namespace SinkInspect
{

bool inspect(const QStringList &args, State &state)
{
    if (args.isEmpty()) {
        state.printError(QObject::tr("Options: [--resource $resource] ([--db $db] [--filter $id] [--showinternal] | [--validaterids $type] | [--fulltext [$id]])"));
    }
    auto options = SyntaxTree::parseOptions(args);
    auto resource = SinkshUtils::parseUid(options.options.value("resource").value(0).toUtf8());

    Sink::Storage::DataStore storage(Sink::storageLocation(), resource, Sink::Storage::DataStore::ReadOnly);
    auto transaction = storage.createTransaction(Sink::Storage::DataStore::ReadOnly);

    if (options.options.contains("validaterids")) {
        if (options.options.value("validaterids").isEmpty()) {
            state.printError(QObject::tr("Specify a type to validate."));
            return false;
        }
        auto type = options.options.value("validaterids").first().toUtf8();
        /*
         * Try to find all rid's for all uid's.
         * If we have entities without rid's that either means we have only created it locally or that we have a problem.
         */
        Sink::Storage::DataStore syncStore(Sink::storageLocation(), resource + ".synchronization", Sink::Storage::DataStore::ReadOnly);
        auto syncTransaction = syncStore.createTransaction(Sink::Storage::DataStore::ReadOnly);

        auto db = transaction.openDatabase(type + ".main",
                [&] (const Sink::Storage::DataStore::Error &e) {
                    Q_ASSERT(false);
                    state.printError(e.message);
                }, false);

        auto ridMap = syncTransaction.openDatabase("localid.mapping." + type,
                [&] (const Sink::Storage::DataStore::Error &e) {
                    Q_ASSERT(false);
                    state.printError(e.message);
                }, false);

        QHash<QByteArray, QByteArray> hash;

        ridMap.scan("", [&] (const QByteArray &key, const QByteArray &data) {
                    hash.insert(key, data);
                    return true;
                },
                [&](const Sink::Storage::DataStore::Error &e) {
                    state.printError(e.message);
                },
                false);

        QSet<QByteArray> uids;
        db.scan("", [&] (const QByteArray &key, const QByteArray &data) {
                    uids.insert(Sink::Storage::DataStore::uidFromKey(key));
                    return true;
                },
                [&](const Sink::Storage::DataStore::Error &e) {
                    state.printError(e.message);
                },
                false);

        int missing = 0;
        for (const auto &uid : uids) {
            if (!hash.remove(uid)) {
                missing++;
                qWarning() << "Failed to find RID for " << uid;
            }
        }
        if (missing) {
            qWarning() << "Found a total of " << missing << " missing rids";
        }

        //If we still have items in the hash it means we have rid mappings for entities
        //that no longer exist.
        if (!hash.isEmpty()) {
            qWarning() << "Have rids left: " << hash.size();
        } else if (!missing) {
            qWarning() << "Everything is in order.";
        }

        return false;
    }
    if (options.options.contains("fulltext")) {
        try {
            Xapian::Database db(QFile::encodeName(Sink::resourceStorageLocation(resource) + '/' + "fulltext").toStdString(), Xapian::DB_OPEN);
            if (options.options.value("fulltext").isEmpty()) {
                state.printLine(QString("Total document count: ") + QString::number(db.get_doccount()));
            } else {
                auto entityId = SinkshUtils::parseUid(options.options.value("fulltext").first().toUtf8());
                auto id = "Q" + entityId.toStdString();
                Xapian::PostingIterator p = db.postlist_begin(id);
                if (p == db.postlist_end(id)) {
                    state.printLine(QString("Failed to find the document with the id: ") + QString::fromStdString(id));
                } else {
                    state.printLine(QString("Found the document: "));
                    auto document = db.get_document(*p);

                    QStringList terms;
                    for (auto it = document.termlist_begin(); it != document.termlist_end(); it++) {
                        terms << QString::fromStdString(*it);
                    }
                    state.printLine(QString("Terms: ") + terms.join(", "), 1);
                }

            }
        } catch (const Xapian::Error &) {
            // Nothing to do, move along
        }

        return false;
    }

    auto dbs = options.options.value("db");
    auto idFilter = options.options.value("filter");
    bool showInternal = options.options.contains("showinternal");

    state.printLine(QString("Current revision: %1").arg(Sink::Storage::DataStore::maxRevision(transaction)));
    state.printLine(QString("Last clean revision: %1").arg(Sink::Storage::DataStore::cleanedUpRevision(transaction)));

    auto databases = transaction.getDatabaseNames();
    if (dbs.isEmpty()) {
        state.printLine("Available databases: ");
        for (const auto &db : databases) {
            state.printLine(db, 1);
        }
        return false;
    }
    auto dbName = dbs.value(0).toUtf8();
    auto isMainDb = dbName.contains(".main");
    if (!databases.contains(dbName)) {
        state.printError(QString("Database not available: ") + dbName);
    }

    state.printLine(QString("Opening: ") + dbName);
    auto db = transaction.openDatabase(dbName,
            [&] (const Sink::Storage::DataStore::Error &e) {
                Q_ASSERT(false);
                state.printError(e.message);
            }, false);

    if (showInternal) {
        //Print internal keys
        db.scan("__internal", [&] (const QByteArray &key, const QByteArray &data) {
                state.printLine("Internal: " + key + "\tValue: " + QString::fromUtf8(data));
                return true;
            },
            [&](const Sink::Storage::DataStore::Error &e) {
                state.printError(e.message);
            },
            true, false);
    } else {
        QByteArray filter;
        if (!idFilter.isEmpty()) {
            filter = idFilter.first().toUtf8();
        }

        //Print rest of db
        bool findSubstringKeys = !filter.isEmpty();
        int keySizeTotal = 0;
        int valueSizeTotal = 0;
        auto count = db.scan(filter, [&] (const QByteArray &key, const QByteArray &data) {
                    keySizeTotal += key.size();
                    valueSizeTotal += data.size();
                    if (isMainDb) {
                        Sink::EntityBuffer buffer(const_cast<const char *>(data.data()), data.size());
                        if (!buffer.isValid()) {
                            state.printError("Read invalid buffer from disk: " + key);
                        } else {
                            const auto metadata = flatbuffers::GetRoot<Sink::Metadata>(buffer.metadataBuffer());
                            state.printLine("Key: " + key
                                          + " Operation: " + QString::number(metadata->operation())
                                          + " Replay: " + (metadata->replayToSource() ? "true" : "false")
                                          + ((metadata->modifiedProperties() && metadata->modifiedProperties()->size() != 0) ? (" [" + Sink::BufferUtils::fromVector(*metadata->modifiedProperties()).join(", ")) + "]": "")
                                          + " Value size: " + QString::number(data.size())
                                          );
                        }
                    } else {
                        state.printLine("Key: " + key + "\tValue: " + QString::fromUtf8(data));
                    }
                    return true;
                },
                [&](const Sink::Storage::DataStore::Error &e) {
                    state.printError(e.message);
                },
                findSubstringKeys);

        state.printLine("Found " + QString::number(count) + " entries");
        state.printLine("Keys take up " + QString::number(keySizeTotal) + " bytes => " + QString::number(keySizeTotal/1024) + " kb");
        state.printLine("Values take up " + QString::number(valueSizeTotal) + " bytes => " + QString::number(valueSizeTotal/1024) + " kb");
    }
    return false;
}

Syntax::List syntax()
{
    Syntax state("inspect", QObject::tr("Inspect database for the resource requested"), &SinkInspect::inspect, Syntax::NotInteractive);
    state.completer = &SinkshUtils::resourceCompleter;

    return Syntax::List() << state;
}

REGISTER_SYNTAX(SinkInspect)

}
