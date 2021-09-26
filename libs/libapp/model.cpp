#include <app/model.h>

namespace App {
void Model::did_update() {
    emit<ModelUpdateEvent>();
}
}
