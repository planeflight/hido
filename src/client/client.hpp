#ifndef CLIENT_HPP
#define CLIENT_HPP

class Client {
  public:
    Client(int port);
    void run();

  private:
    int sock = 0;
};

#endif // CLIENT_HPP
