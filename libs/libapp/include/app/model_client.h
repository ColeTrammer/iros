#pragma once

namespace App {

class ModelClient {
public:
    virtual ~ModelClient() {}

    virtual void model_did_update() {}
};

}
