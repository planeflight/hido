#include "client_manager.hpp"

#include <netinet/in.h>
#include <spdlog/spdlog.h>

#include <algorithm>

#include "state/player.hpp"

ClientAddr::ClientAddr(sockaddr_in addr) : addr(addr), player() {}

ClientManager::ClientManager() {}

ClientAddr *ClientManager::add(sockaddr_in client) {
    ClientAddr new_client(client);
    // set starting position
    new_client.player.rect = {20.0f, 20.0f, PLAYER_WIDTH, PLAYER_HEIGHT};
    auto itr = std::find_if(
        clients.begin(), clients.end(), [&new_client](const auto &kv) {
            return kv.second == new_client;
        });
    // add client if not already contained
    if (itr == clients.end()) {
        new_client.id = idx++;
        spdlog::info("Client {} connected.", new_client.id);
        auto [itr, _] = clients.emplace(new_client.id, std::move(new_client));
        return &itr->second;
    }
    return &itr->second;
}

ClientAddr *ClientManager::get(sockaddr_in client) {
    ClientAddr new_client(client);
    // set starting position
    new_client.player.rect = {20.0f, 20.0f, PLAYER_WIDTH, PLAYER_HEIGHT};
    auto itr = std::find_if(
        clients.begin(), clients.end(), [&new_client](const auto &kv) {
            return kv.second == new_client;
        });
    // if client doesn't exist uh oh
    if (itr == clients.end()) {
        return nullptr;
    }
    return &itr->second;
}

void ClientManager::remove(const ClientAddr &c) {
    auto itr = clients.find(c.id);
    if (itr != clients.end()) {
        clients.erase(itr);
        spdlog::info("Client {} disconnected.", c.id);
    }
}

size_t ClientManager::count() const {
    return clients.size();
}

ClientAddr &ClientManager::find_by_id(ClientID id) {
    return clients[id];
}
