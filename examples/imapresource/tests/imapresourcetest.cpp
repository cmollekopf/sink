#include <QtTest>

#include <QString>
#include <KMime/Message>

// #include "imapresource/imapresource.h"
#include "store.h"
#include "resourcecontrol.h"
#include "commands.h"
#include "entitybuffer.h"
#include "resourceconfig.h"
#include "modelresult.h"
#include "pipeline.h"
#include "log.h"
#include "test.h"
#include "../imapresource.h"
#include "../imapserverproxy.h"

#define ASYNCCOMPARE(actual, expected) \
do {\
    if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))\
        return KAsync::error<void>(1, "Comparison failed.");\
} while (0)

#define ASYNCVERIFY(statement) \
do {\
    if (!QTest::qVerify((statement), #statement, "", __FILE__, __LINE__))\
        return KAsync::error<void>(1, "Verify failed.");\
} while (0)

#define VERIFYEXEC(statement) \
do {\
    auto result = statement.exec(); \
    result.waitForFinished(); \
    if (!QTest::qVerify(!result.errorCode(), #statement, "", __FILE__, __LINE__))\
        return;\
} while (0)

using namespace Sink;
using namespace Sink::ApplicationDomain;

/**
 * Test of complete system using the imap resource.
 *
 * This test requires the imap resource installed.
 */
class ImapResourceTest : public QObject
{
    Q_OBJECT

    QTemporaryDir tempDir;
    QString targetPath;
private slots:
    void initTestCase()
    {
        Sink::Test::initTest();
        Sink::Log::setDebugOutputLevel(Sink::Log::Trace);
        ::ImapResource::removeFromDisk("org.kde.imap.instance1");
        system("resetmailbox.sh");
        // auto resource = ApplicationDomain::ImapResource::create("account1");
        Sink::ApplicationDomain::SinkResource resource;
        resource.setProperty("identifier", "org.kde.imap.instance1");
        resource.setProperty("type", "org.kde.imap");
        resource.setProperty("server", "localhost");
        resource.setProperty("user", "doe");
        resource.setProperty("password", "doe");
        resource.setProperty("port", 993);
        Sink::Store::create(resource).exec().waitForFinished();
    }

    void cleanup()
    {
        Sink::ResourceControl::shutdown(QByteArray("org.kde.imap.instance1")).exec().waitForFinished();
        ::ImapResource::removeFromDisk("org.kde.imap.instance1");
    }

    void init()
    {
        qDebug();
        qDebug() << "-----------------------------------------";
        qDebug();
        Sink::ResourceControl::start(QByteArray("org.kde.imap.instance1")).exec().waitForFinished();
    }

    void testListFolders()
    {
        Sink::Query query;
        query.resources << "org.kde.imap.instance1";
        query.request<Folder::Name>();

        // Ensure all local data is processed
        VERIFYEXEC(Store::synchronize(query));
        ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

        auto job = Store::fetchAll<Folder>(query).then<void, QList<Folder::Ptr>>([](const QList<Folder::Ptr> &folders) {
            QCOMPARE(folders.size(), 2);
            QStringList names;
            for (const auto &folder : folders) {
                names << folder->getName();
            }
            QVERIFY(names.contains("INBOX"));
            QVERIFY(names.contains("test"));
        });
        VERIFYEXEC(job);
    }

    void testListFolderHierarchy()
    {
        Sink::Query query;
        query.resources << "org.kde.imap.instance1";
        query.request<Folder::Name>().request<Folder::Parent>();

        Imap::ImapServerProxy imap("localhost", 993);
        VERIFYEXEC(imap.login("doe", "doe"));
        VERIFYEXEC(imap.create("INBOX.test.sub"));

        // Ensure all local data is processed
        VERIFYEXEC(Store::synchronize(query));
        ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

        auto job = Store::fetchAll<Folder>(query).then<void, QList<Folder::Ptr>>([](const QList<Folder::Ptr> &folders) {
            QCOMPARE(folders.size(), 3);
            QHash<QString, Folder::Ptr> map;
            for (const auto &folder : folders) {
                map.insert(folder->getName(), folder);
            }
            QCOMPARE(map.value("sub")->getParent(), map.value("test")->identifier());
        });
        VERIFYEXEC(job);
    }

    void testListNewFolders()
    {
        Sink::Query query;
        query.resources << "org.kde.imap.instance1";
        query.request<Folder::Name>();

        Imap::ImapServerProxy imap("localhost", 993);
        VERIFYEXEC(imap.login("doe", "doe"));
        VERIFYEXEC(imap.create("INBOX.test.sub1"));

        // Ensure all local data is processed
        VERIFYEXEC(Store::synchronize(query));
        ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

        auto job = Store::fetchAll<Folder>(query).then<void, QList<Folder::Ptr>>([](const QList<Folder::Ptr> &folders) {
            QStringList names;
            for (const auto &folder : folders) {
                names << folder->getName();
            }
            QVERIFY(names.contains("sub1"));
        });
        VERIFYEXEC(job);
    }

    void testListRemovedFolders()
    {
        Sink::Query query;
        query.resources << "org.kde.imap.instance1";
        query.request<Folder::Name>();

        VERIFYEXEC(Store::synchronize(query));
        ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

        Imap::ImapServerProxy imap("localhost", 993);
        VERIFYEXEC(imap.login("doe", "doe"));
        VERIFYEXEC(imap.remove("INBOX.test.sub1"));

        // Ensure all local data is processed
        VERIFYEXEC(Store::synchronize(query));
        ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

        auto job = Store::fetchAll<Folder>(query).then<void, QList<Folder::Ptr>>([](const QList<Folder::Ptr> &folders) {
            QStringList names;
            for (const auto &folder : folders) {
                names << folder->getName();
            }
            QVERIFY(!names.contains("sub1"));
        });
        VERIFYEXEC(job);
    }

    void testListMails()
    {
        Sink::Query query;
        query.resources << "org.kde.imap.instance1";
        query.request<Mail::Subject>().request<Mail::MimeMessage>();

        // Ensure all local data is processed
        VERIFYEXEC(Store::synchronize(query));
        ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

        auto job = Store::fetchAll<Mail>(query).then<void, QList<Mail::Ptr>>([](const QList<Mail::Ptr> &mails) {
            QCOMPARE(mails.size(), 1);
            QVERIFY(mails.first()->getSubject().startsWith(QString("[Nepomuk] Jenkins build is still unstable")));
            const auto data = mails.first()->getMimeMessage();
            QVERIFY(!data.isEmpty());

            KMime::Message m;
            m.setContent(data);
            m.parse();
            QCOMPARE(mails.first()->getSubject(), m.subject(true)->asUnicodeString());
        });
        VERIFYEXEC(job);
    }

    void testFetchNewMessages()
    {
        Sink::Query query;
        query.resources << "org.kde.imap.instance1";
        query.request<Mail::Subject>().request<Mail::MimeMessage>();

        // Ensure all local data is processed
        VERIFYEXEC(Store::synchronize(query));
        ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

        Imap::ImapServerProxy imap("localhost", 993);
        VERIFYEXEC(imap.login("doe", "doe"));

        auto msg = KMime::Message::Ptr::create();
        msg->subject(true)->fromUnicodeString("Foobar", "utf8");
        msg->assemble();

        VERIFYEXEC(imap.append("INBOX.test", msg->encodedContent(true)));

        Store::synchronize(query).exec().waitForFinished();
        ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

        auto job = Store::fetchAll<Mail>(query).then<void, QList<Mail::Ptr>>([](const QList<Mail::Ptr> &mails) {
            QCOMPARE(mails.size(), 2);
        });
        VERIFYEXEC(job);
    }

    void testFetchRemovedMessages()
    {
        Sink::Query query;
        query.resources << "org.kde.imap.instance1";
        query.request<Mail::Subject>().request<Mail::MimeMessage>();

        // Ensure all local data is processed
        VERIFYEXEC(Store::synchronize(query));
        ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

        Imap::ImapServerProxy imap("localhost", 993);
        VERIFYEXEC(imap.login("doe", "doe"));

        VERIFYEXEC(imap.remove("INBOX.test", "2:*"));

        Store::synchronize(query).exec().waitForFinished();
        ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

        auto job = Store::fetchAll<Mail>(query).then<void, QList<Mail::Ptr>>([](const QList<Mail::Ptr> &mails) {
            QCOMPARE(mails.size(), 1);
        });
        VERIFYEXEC(job);
    }

    void testFailingSync()
    {
        auto resource = ApplicationDomain::ImapResource::create("account1");
        resource.setProperty("server", "foobar");
        resource.setProperty("port", 993);
        Sink::Store::create(resource).exec().waitForFinished();
        Sink::Query query;
        query.resources << resource.identifier();

        // Ensure sync fails if resource is misconfigured
        auto future = Store::synchronize(query).exec();
        future.waitForFinished();
        QVERIFY(future.errorCode());
    }
};

QTEST_MAIN(ImapResourceTest)
#include "imapresourcetest.moc"