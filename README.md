# Hido

Multiplayer UDP dungeon shooter game!

## Features

- _Authoritive UDP Game Server_
  - Server synchronizes and validates the game state for the clients to prevent cheating
- _Real-Time Multiplayer_
  - epoll networking to support multiple simulataneous players
- _Client Prediction & Reconciliation_
  - Smooth player movement in real-time with authoritative server reconciliation
- _Entity Interpolation/Lag Compensation_
  - Renders other entities in the past (~100ms) in case of packet loss/jitters and interpolates between render times for a smooth render
- _Graceful Connect/Disconnect_
- _Constant Timestep Game loop_
  - Allows consistent simulation amongst all clients

## Building

With CMake:

```
./build.sh
```

The client uses raylib. [Ensure libraries and drivers are installed.](https://github.com/raysan5/raylib/wiki)

### Running the server (default is port 8080):

```
./build/hido
```

### Running the client:

```
./build/client <address> <port>
```

## Resources

- [epoll](https://man7.org/linux/man-pages/man2/epoll_wait.2.html)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split/)
- [Multiplayer Game](https://codersblock.org/multiplayer-fps/part1/)
- [Client Side Prediction and Server Reconciliation](https://www.gabrielgambetta.com/client-side-prediction-server-reconciliation.html)
