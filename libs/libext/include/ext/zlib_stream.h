#pragma once

#include <ext/deflate.h>

namespace Ext {
class ZLibStreamDecoder final : public StreamDecoder {
public:
    ZLibStreamDecoder();
    virtual ~ZLibStreamDecoder() override;

private:
    Generator<StreamResult> decode();

    DeflateDecoder m_deflate_decoder;
};
}
