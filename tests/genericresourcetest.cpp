#include <QtTest>

#include <QString>

#include "testimplementations.h"

#include "event_generated.h"
#include "entity_generated.h"
#include "metadata_generated.h"
#include "createentity_generated.h"
#include "commands.h"
#include "entitybuffer.h"
#include "pipeline.h"
#include "genericresource.h"
#include "definitions.h"

static void removeFromDisk(const QString &name)
{
    Akonadi2::Storage store(Akonadi2::storageLocation(), name, Akonadi2::Storage::ReadWrite);
    store.removeFromDisk();
}

/**
 * Test of the generic resource implementation.
 * 
 * This test relies on a working pipeline implementation, and writes to storage.
 */
class GenericResourceTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void init()
    {
        Akonadi2::GenericResource::removeFromDisk("org.kde.test.instance1");
        Akonadi2::Log::setDebugOutputLevel(Akonadi2::Log::Trace);
    }

    ///Ensure the resource can process messages
    void testProcessCommand()
    {
        flatbuffers::FlatBufferBuilder eventFbb;
        eventFbb.Clear();
        {
            auto summary = eventFbb.CreateString("summary");
            Akonadi2::ApplicationDomain::Buffer::EventBuilder eventBuilder(eventFbb);
            eventBuilder.add_summary(summary);
            auto eventLocation = eventBuilder.Finish();
            Akonadi2::ApplicationDomain::Buffer::FinishEventBuffer(eventFbb, eventLocation);
        }

        flatbuffers::FlatBufferBuilder localFbb;
        {
            auto uid = localFbb.CreateString("testuid");
            auto localBuilder = Akonadi2::ApplicationDomain::Buffer::EventBuilder(localFbb);
            localBuilder.add_uid(uid);
            auto location = localBuilder.Finish();
            Akonadi2::ApplicationDomain::Buffer::FinishEventBuffer(localFbb, location);
        }

        flatbuffers::FlatBufferBuilder entityFbb;
        Akonadi2::EntityBuffer::assembleEntityBuffer(entityFbb, 0, 0, eventFbb.GetBufferPointer(), eventFbb.GetSize(), localFbb.GetBufferPointer(), localFbb.GetSize());

        flatbuffers::FlatBufferBuilder fbb;
        auto type = fbb.CreateString(Akonadi2::ApplicationDomain::getTypeName<Akonadi2::ApplicationDomain::Event>().toStdString().data());
        auto delta = fbb.CreateVector<uint8_t>(entityFbb.GetBufferPointer(), entityFbb.GetSize());
        Akonadi2::Commands::CreateEntityBuilder builder(fbb);
        builder.add_domainType(type);
        builder.add_delta(delta);
        auto location = builder.Finish();
        Akonadi2::Commands::FinishCreateEntityBuffer(fbb, location);

        const QByteArray command(reinterpret_cast<const char *>(fbb.GetBufferPointer()), fbb.GetSize());
        {
            flatbuffers::Verifier verifyer(reinterpret_cast<const uint8_t *>(command.data()), command.size());
            QVERIFY(Akonadi2::Commands::VerifyCreateEntityBuffer(verifyer));
        }

        //Actual test
        auto pipeline = QSharedPointer<Akonadi2::Pipeline>::create("org.kde.test.instance1");
        QSignalSpy revisionSpy(pipeline.data(), SIGNAL(revisionUpdated(qint64)));
        QVERIFY(revisionSpy.isValid());
        TestResource resource("org.kde.test.instance1", pipeline);
        resource.processCommand(Akonadi2::Commands::CreateEntityCommand, command);
        resource.processCommand(Akonadi2::Commands::CreateEntityCommand, command);
        resource.processAllMessages().exec().waitForFinished();
        QCOMPARE(revisionSpy.last().at(0).toInt(), 2);
    }
};

QTEST_MAIN(GenericResourceTest)
#include "genericresourcetest.moc"
