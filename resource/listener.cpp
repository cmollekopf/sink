#include "listener.h"

#include "common/console.h"

#include <QLocalSocket>

Listener::Listener(const QString &resource, QObject *parent)
    : QObject(parent),
      m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection,
             this, &Listener::acceptConnection);
    Console::main()->log(QString("Trying to open %1").arg(resource));
    if (!m_server->listen(resource)) {
        // FIXME: multiple starts need to be handled here
        m_server->removeServer(resource);
        if (!m_server->listen(resource)) {
            Console::main()->log("Utter failure to start server");
            exit(-1);
        }
    }

    if (m_server->isListening()) {
        Console::main()->log(QString("Listening on %1").arg(m_server->serverName()));
    }
}

Listener::~Listener()
{
}

void Listener::closeAllConnections()
{
    //TODO: close all client connectionsin m_connections
}

void Listener::acceptConnection()
{
    Console::main()->log(QString("Accepting connection"));
    QLocalSocket *connection = m_server->nextPendingConnection();

    if (!connection) {
        return;
    }

    Console::main()->log("Got a connection");
    Client client(connection);
    m_connections << client;
    connect(connection, &QLocalSocket::disconnected,
            this, &Listener::clientDropped);

}

void Listener::clientDropped()
{
    QLocalSocket *connection = qobject_cast<QLocalSocket *>(sender());
    if (!connection) {
        return;
    }

    Console::main()->log("Dropping connection...");
    QMutableListIterator<Client> it(m_connections);
    while (it.hasNext()) {
        const Client &client = it.next();
        if (client.m_socket == connection) {
            Console::main()->log("    dropped...");
            it.remove();
            break;
        }
    }
}
