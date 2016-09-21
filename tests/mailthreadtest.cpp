/*
 *   Copyright (C) 2016 Christian Mollekopf <chrigi_1@fastmail.fm>
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
#include "mailthreadtest.h"

#include <QtTest>

#include <QString>
#include <KMime/Message>

#include "store.h"
#include "resourcecontrol.h"
#include "log.h"
#include "test.h"

using namespace Sink;
using namespace Sink::ApplicationDomain;

SINK_DEBUG_AREA("mailthreadtest")

//TODO extract resource test
//
void MailThreadTest::initTestCase()
{
    Test::initTest();
    QVERIFY(isBackendAvailable());
    resetTestEnvironment();
    auto resource = createResource();
    QVERIFY(!resource.identifier().isEmpty());

    VERIFYEXEC(Store::create(resource));

    mResourceInstanceIdentifier = resource.identifier();
    mCapabilities = resource.getProperty("capabilities").value<QByteArrayList>();
}

void MailThreadTest::cleanup()
{
    //TODO the shutdown job fails if the resource is already shut down
    // VERIFYEXEC(ResourceControl::shutdown(mResourceInstanceIdentifier));
    ResourceControl::shutdown(mResourceInstanceIdentifier).exec().waitForFinished();
    removeResourceFromDisk(mResourceInstanceIdentifier);
}

void MailThreadTest::init()
{
    VERIFYEXEC(ResourceControl::start(mResourceInstanceIdentifier));
}


void MailThreadTest::testListThreadLeader()
{
    Sink::Query query;
    query.resources << mResourceInstanceIdentifier;
    query.request<Mail::Subject>().request<Mail::MimeMessage>().request<Mail::Folder>().request<Mail::Date>();
    query.threadLeaderOnly = true;
    query.sort<Mail::Date>();

    // Ensure all local data is processed
    VERIFYEXEC(Store::synchronize(query));
    ResourceControl::flushMessageQueue(query.resources).exec().waitForFinished();

    auto job = Store::fetchAll<Mail>(query).syncThen<void, QList<Mail::Ptr>>([](const QList<Mail::Ptr> &mails) {
        QCOMPARE(mails.size(), 1);
        QVERIFY(mails.first()->getSubject().startsWith(QString("ThreadLeader")));
    });
    VERIFYEXEC(job);
}

/*
 * Thread:
 * 1.
 *  2.
 *   3.
 *
 * 3. first, should result in a new thread.
 * 1. second, should be merged by subject
 * 2. last, should complete the thread.
 */
void MailThreadTest::testIndexInMixedOrder()
{
    auto folder = Folder::create(mResourceInstanceIdentifier);
    folder.setName("folder");
    VERIFYEXEC(Store::create(folder));

    auto message1 = KMime::Message::Ptr::create();
    message1->subject(true)->fromUnicodeString("1", "utf8");
    message1->messageID(true)->generate("foobar.com");
    message1->date(true)->setDateTime(QDateTime::currentDateTimeUtc());
    message1->assemble();

    auto message2 = KMime::Message::Ptr::create();
    message2->subject(true)->fromUnicodeString("Re: 1", "utf8");
    message2->messageID(true)->generate("foobar.com");
    message2->inReplyTo(true)->appendIdentifier(message1->messageID(true)->identifier());
    message2->date(true)->setDateTime(QDateTime::currentDateTimeUtc().addSecs(1));
    message2->assemble();

    auto message3 = KMime::Message::Ptr::create();
    message3->subject(true)->fromUnicodeString("Re: Re: 1", "utf8");
    message3->messageID(true)->generate("foobar.com");
    message3->inReplyTo(true)->appendIdentifier(message2->messageID(true)->identifier());
    message3->date(true)->setDateTime(QDateTime::currentDateTimeUtc().addSecs(2));
    message3->assemble();

    {
        auto mail = Mail::create(mResourceInstanceIdentifier);
        mail.setMimeMessage(message3->encodedContent());
        mail.setFolder(folder);
        VERIFYEXEC(Store::create(mail));
    }
    VERIFYEXEC(ResourceControl::flushMessageQueue(QByteArrayList() << mResourceInstanceIdentifier));

    Sink::Query query;
    query.resources << mResourceInstanceIdentifier;
    query.request<Mail::Subject>().request<Mail::MimeMessage>().request<Mail::Folder>().request<Mail::Date>();
    query.threadLeaderOnly = true;
    query.sort<Mail::Date>();
    query.filter<Mail::Folder>(folder);

    {
        auto job = Store::fetchAll<Mail>(query)
            .syncThen<void, QList<Mail::Ptr>>([=](const QList<Mail::Ptr> &mails) {
                QCOMPARE(mails.size(), 1);
                auto mail = *mails.first();
                QCOMPARE(mail.getSubject(), QString::fromLatin1("Re: Re: 1"));
            });
        VERIFYEXEC(job);
    }

    {
        auto mail = Mail::create(mResourceInstanceIdentifier);
        mail.setMimeMessage(message1->encodedContent());
        mail.setFolder(folder);
        VERIFYEXEC(Store::create(mail));
    }

    VERIFYEXEC(ResourceControl::flushMessageQueue(QByteArrayList() << mResourceInstanceIdentifier));
    {
        auto job = Store::fetchAll<Mail>(query)
            .syncThen<void, QList<Mail::Ptr>>([=](const QList<Mail::Ptr> &mails) {
                QCOMPARE(mails.size(), 1);
                auto mail = *mails.first();
                QCOMPARE(mail.getSubject(), QString::fromLatin1("Re: Re: 1"));
            });
        VERIFYEXEC(job);
        //TODO ensure we also find message 1 as part of thread.
    }

    /* VERIFYEXEC(Store::remove(mail)); */
    /* VERIFYEXEC(ResourceControl::flushMessageQueue(QByteArrayList() << mResourceInstanceIdentifier)); */
    /* { */
    /*     auto job = Store::fetchAll<Mail>(Query::RequestedProperties(QByteArrayList() << Mail::Folder::name << Mail::Subject::name)) */
    /*         .syncThen<void, QList<Mail::Ptr>>([=](const QList<Mail::Ptr> &mails) { */
    /*             QCOMPARE(mails.size(), 0); */
    /*         }); */
    /*     VERIFYEXEC(job); */
    /* } */
    /* VERIFYEXEC(ResourceControl::flushReplayQueue(QByteArrayList() << mResourceInstanceIdentifier)); */
}

#include "mailthreadtest.moc"