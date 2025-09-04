#include <csignal>

#include "network.hpp"
#include "server.hpp"

Server live_stream(PORT);
void handler(int s) {
    live_stream.shutdown();
}

int main() {
    // SIGNAL INTERRUPT HANDLER
    // https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event
    struct sigaction sig_int_handler;

    sig_int_handler.sa_handler = handler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;

    sigaction(SIGINT, &sig_int_handler, NULL);
    live_stream.serve();

    return 0;
}
