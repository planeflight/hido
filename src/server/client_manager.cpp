#include "client_manager.hpp"

#include <netinet/in.h>
#include <spdlog/spdlog.h>

#include <algorithm>

ClientManager::ClientManager() {}

const ClientAddr &ClientManager::add(sockaddr_in client) {
    ClientAddr new_client{.addr = client};
    auto itr = clients.find(new_client);
    // add client if not already contained
    if (itr == clients.end()) {
        new_client.id = idx++;
        spdlog::info("Client {} connected.", new_client.id);
        clients.insert(new_client);
        return *clients.find(new_client);
    }
    return *itr;
}

void ClientManager::remove(const ClientAddr &c) {
    auto itr = clients.find(c);
    if (itr != clients.end()) {
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
    return clients.size();
}

const ClientAddr &ClientManager::find_by_id(ClientID id) {
    auto itr = std::find_if(
        clients.begin(), clients.end(), [&id](const ClientAddr &addr) -> bool {
            return id == addr.id;
        });
    if (itr == clients.end()) {
        spdlog::error("Failed to find client {}", id);
    }
    return *itr;
}
