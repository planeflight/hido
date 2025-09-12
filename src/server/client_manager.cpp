#include "client_manager.hpp"

#include <netinet/in.h>

ClientManager::ClientManager() {}

const ClientAddr &ClientManager::add(sockaddr_in client) {
    ClientAddr new_client{.addr = client};
    auto itr = clients.find(new_client);
    // add client if not already contained
    if (itr == clients.end()) {
        new_client.id = idx++;
        clients.insert(new_client);
        active_clients++;
        return *clients.find(new_client);
    }
    return *itr;
}

void ClientManager::remove(const ClientAddr &c) {
    auto itr = clients.find(c);
    if (itr != clients.end()) {
        active_clients--;
        clients.erase(itr);
    }
}

ClientID ClientManager::get(const ClientAddr &c) {
    auto itr = clients.find(c);
    if (itr != clients.end()) {
        return itr->id;
    }
    return -1;
}

ClientID ClientManager::get(sockaddr_in client) {
    return get(ClientAddr{client});
}

size_t ClientManager::count() const {
    return active_clients;
}
