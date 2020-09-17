#include <app/model.h>
#include <app/model_client.h>
#include <app/view.h>

namespace App {

void Model::register_client(ModelClient* client) {
    m_clients.add(client);
    client->model_did_update();
}

void Model::unregister_client(ModelClient* client) {
    m_clients.remove_element(client);
}

void Model::did_update() {
    for (auto* client : m_clients) {
        client->model_did_update();
    }
}

}
