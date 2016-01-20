/*
 * Copyright (C) 2014 Aaron Seigo <aseigo@kde.org>
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

#include "commands.h"

#include <QIODevice>

namespace Sink
{

namespace Commands
{

QByteArray name(int commandId)
{
    switch(commandId) {
        case UnknownCommand:
            return "Unknown";
        case CommandCompletionCommand:
            return "Completion";
        case HandshakeCommand:
            return "Handshake";
        case RevisionUpdateCommand:
            return "RevisionUpdate";
        case SynchronizeCommand:
            return "Synchronize";
        case FetchEntityCommand:
            return "FetchEntity";
        case DeleteEntityCommand:
            return "DeleteEntity";
        case ModifyEntityCommand:
            return "ModifyEntity";
        case CreateEntityCommand:
            return "CreateEntity";
        case SearchSourceCommand:
            return "SearchSource";
        case ShutdownCommand:
            return "Shutdown";
        case NotificationCommand:
            return "Notification";
        case PingCommand:
            return "Ping";
        case RevisionReplayedCommand:
            return "RevisionReplayed";
        case InspectionCommand:
            return "Inspection";
        case CustomCommand:
            return "Custom";
    };
    return QByteArray("Invalid commandId");
}

int headerSize()
{
    return sizeof(int) + (sizeof(uint) * 2);
}

void write(QIODevice *device, int messageId, int commandId)
{
    write(device, messageId, commandId, 0, 0);
}

void write(QIODevice *device, int messageId, int commandId, const char *buffer, uint size)
{
    if (size > 0 && !buffer) {
        size = 0;
    }

    device->write((const char*)&messageId, sizeof(int));
    device->write((const char*)&commandId, sizeof(int));
    device->write((const char*)&size, sizeof(uint));
    if (buffer) {
        device->write(buffer, size);
    }
}

void write(QIODevice *device, int messageId, int commandId, flatbuffers::FlatBufferBuilder &fbb)
{
    const int dataSize = fbb.GetSize();
    device->write((const char*)&messageId, sizeof(int));
    device->write((const char*)&commandId, sizeof(int));
    device->write((const char*)&dataSize, sizeof(int));
    device->write((const char*)fbb.GetBufferPointer(), dataSize);
}

} // namespace Commands

} // namespace Sink
