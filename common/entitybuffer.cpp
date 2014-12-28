#include "entitybuffer.h"

#include "entity_generated.h"
#include "metadata_generated.h"
#include <QDebug>

using namespace Akonadi2;

EntityBuffer::EntityBuffer(void *dataValue, int dataSize)
    : mEntity(nullptr)
{
    flatbuffers::Verifier verifyer(reinterpret_cast<const uint8_t *>(dataValue), dataSize);
    // Q_ASSERT(Akonadi2::VerifyEntity(verifyer));
    if (!Akonadi2::VerifyEntityBuffer(verifyer)) {
        qWarning() << "invalid buffer";
    } else {
        mEntity = Akonadi2::GetEntity(dataValue);
    }
}

const flatbuffers::Vector<uint8_t>* EntityBuffer::resourceBuffer()
{
    if (!mEntity) {
        qDebug() << "no buffer";
        return nullptr;
    }
    return mEntity->resource();
}

const flatbuffers::Vector<uint8_t>* EntityBuffer::metadataBuffer()
{
    if (!mEntity) {
        return nullptr;
    }
    return mEntity->metadata();
}

const flatbuffers::Vector<uint8_t>* EntityBuffer::localBuffer()
{
    if (!mEntity) {
        return nullptr;
    }
    return mEntity->local();
}

void EntityBuffer::extractResourceBuffer(void *dataValue, int dataSize, const std::function<void(const flatbuffers::Vector<uint8_t> *)> &handler)
{
    Akonadi2::EntityBuffer buffer(dataValue, dataSize);
    if (auto resourceData = buffer.resourceBuffer()) {
        handler(resourceData);
    }
}

