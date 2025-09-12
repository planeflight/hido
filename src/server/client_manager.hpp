#ifndef SERVER_CLIENTMANAGER_HPP
#define SERVER_CLIENTMANAGER_HPP

#include <netinet/in.h>

#include <cstring>
#include <set>

using ClientID = int;
struct ClientAddr {
    sockaddr_in addr;
    // optional, just used for storing id's by server
    ClientID id;

    bool operator<(const ClientAddr &other) const {
        return memcmp(&addr, &other.addr, sizeof(sockaddr_in)) < 0;
    }

    bool operator==(const ClientAddr &other) const {
        return addr.sin_addr.s_addr == other.addr.sin_addr.s_addr &&
               addr.sin_port == other.addr.sin_port;
    }

    bool operator==(sockaddr_in client) const {
        return client.sin_addr.s_addr == addr.sin_addr.s_addr &&
               client.sin_port == addr.sin_port;
    }
};

class ClientManager {
  public:
    ClientManager();
    const ClientAddr &add(sockaddr_in client);
    void remove(const ClientAddr &c);
    ClientID get(const ClientAddr &c);
    ClientID get(sockaddr_in client);
    size_t count() const;

    const std::set<ClientAddr> &get_clients() const {
        return clients;
    }

  private:
    std::set<ClientAddr> clients;
    size_t active_clients = 0;
    ClientID idx = 0;
};

#endif // SERVER_CLIENTMANAGER_HPP
