/*
 *   Copyright (C) 2015 Christian Mollekopf <chrigi_1@fastmail.fm>
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

#include <Async/Async>

#include <KMime/KMime/KMimeMessage>
#include <KIMAP/KIMAP/ListJob>
#include <KIMAP/KIMAP/Session>
#include <KIMAP/KIMAP/FetchJob>

namespace Imap {

namespace Flags
{
    /// The flag for a message being seen (i.e. opened by user).
    extern const char* Seen;
    /// The flag for a message being deleted by the user.
    extern const char* Deleted;
    /// The flag for a message being replied to by the user.
    extern const char* Answered;
    /// The flag for a message being marked as flagged.
    extern const char* Flagged;
}

struct Message {
    qint64 uid;
    qint64 size;
    QPair<QByteArray, QVariant> attributes;
    QList<QByteArray> flags;
    KMime::Message::Ptr msg;
};

struct Folder {
    QString normalizedPath() const
    {
        return pathParts.join('/');
    }

    QList<QString> pathParts;
};

class ImapServerProxy {
    KIMAP::Session *mSession;
    QChar mSeparatorCharacter;
public:
    ImapServerProxy(const QString &serverUrl, int port);

    //Standard IMAP calls
    KAsync::Job<void> login(const QString &username, const QString &password);
    KAsync::Job<void> select(const QString &mailbox);
    KAsync::Job<void> append(const QString &mailbox, const QByteArray &content, const QList<QByteArray> &flags = QList<QByteArray>(), const QDateTime &internalDate = QDateTime());
    KAsync::Job<void> store(const KIMAP::ImapSet &set, const QList<QByteArray> &flags);
    KAsync::Job<void> create(const QString &mailbox);
    KAsync::Job<void> remove(const QString &mailbox);
    KAsync::Job<void> expunge();

    typedef std::function<void(const QString &,
                        const QMap<qint64,qint64> &,
                        const QMap<qint64,qint64> &,
                        const QMap<qint64,KIMAP::MessageAttribute> &,
                        const QMap<qint64,KIMAP::MessageFlags> &,
                        const QMap<qint64,KIMAP::MessagePtr> &)> FetchCallback;

    KAsync::Job<void> fetch(const KIMAP::ImapSet &set, KIMAP::FetchJob::FetchScope scope, FetchCallback callback);
    KAsync::Job<void> list(KIMAP::ListJob::Option option, const std::function<void(const QList<KIMAP::MailBoxDescriptor> &mailboxes,const QList<QList<QByteArray> > &flags)> &callback);

    //Composed calls that do login etc.
    KAsync::Job<QList<qint64>> fetchHeaders(const QString &mailbox);
    KAsync::Job<void> remove(const QString &mailbox, const QByteArray &imapSet);

    KAsync::Future<void> fetchFolders(std::function<void(const QVector<Folder> &)> callback);
    KAsync::Future<void> fetchMessages(const Folder &folder, std::function<void(const QVector<Message> &)> callback);
};

}