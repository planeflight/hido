# Hido

## Features

- UDP Multiplayer server
- 2D dungeon shooter game
- State Sychronization
- Latency/Interpolation Management

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
