#include "client.hpp"
#include "network.hpp"

int main() {
    Client client{PORT};
    client.run();
}
