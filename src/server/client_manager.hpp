#ifndef HIDO_SERVER_CLIENTMANAGER_HPP
#define HIDO_SERVER_CLIENTMANAGER_HPP

#include <netinet/in.h>

#include <cstring>
#include <set>
#include <unordered_map>

#include "network.hpp"
#include "state/player.hpp"

using ClientID = int;
struct ClientAddr {
    explicit ClientAddr(sockaddr_in addr);
    ClientAddr() = default;

    bool operator<(const ClientAddr &other) const {
        return id < other.id;
    }

    bool operator==(const ClientAddr &other) const {
        return addr.sin_addr.s_addr == other.addr.sin_addr.s_addr &&
               addr.sin_port == other.addr.sin_port;
    }

    bool operator==(sockaddr_in client) const {
        return client.sin_addr.s_addr == addr.sin_addr.s_addr &&
               client.sin_port == addr.sin_port;
    }
    sockaddr_in addr;
    PlayerState player;
    InputPacket last_input;
    // optional, just used for storing id's by server
    ClientID id;
};

class ClientManager {
  public:
    ClientManager();
    ClientAddr &add(sockaddr_in client);
    void remove(const ClientAddr &c);
    size_t count() const;

    std::unordered_map<ClientID, ClientAddr> &get_clients() {
        return clients;
    }

    ClientAddr &find_by_id(ClientID id);

  private:
    std::unordered_map<ClientID, ClientAddr> clients;
    ClientID idx = 0;
};

#endif // HIDO_SERVER_CLIENTMANAGER_HPP
