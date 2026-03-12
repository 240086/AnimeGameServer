AnimeGameServer

├── .gitignore
├── CMakeLists.txt
├── devlog.txt
├── README.md
├── Structure.md
│
├── config
│   └── server.yaml
│
├── include
│
│   ├── common
│   │   ├── Config.h
│   │   ├── Logger.h
│   │   ├── RandomEngine.h
│   │   ├── GlobalThreadPool.h
│   │   └── ThreadPool.h
│
│   ├── game
│   │   └── gacha
│   │       ├── GachaItem.h
│   │       ├── GachaPool.h
│   │       └── GachaSystem.h
│
│   ├── network
│   │   ├── Connection.h
│   │   ├── TcpServer.h
│   │   ├── RecvBuffer.h
│   │   ├── MessageDispatcher.h
│   │   ├── ConnectionManager.h
│   │   ├── MessageId.h
│   │   ├── Packet.h
│   │   └── PacketParser.h
│
│   └── services
│       ├── BaseService.h
│       ├── GachaService.h
│       ├── LoginService.h
│       └── ServiceManager.h
│
├── src
│
│   ├── common
│   │   ├── Config.cpp
│   │   ├── Logger.cpp
│   │   ├── RandomEngine.cpp
│   │   ├── GlobalThreadPool.cpp
│   │   └── ThreadPool.cpp
│
│   ├── game
│   │   └── gacha
│   │       ├── GachaPool.cpp
│   │       └── GachaSystem.cpp
│
│   ├── network
│   │   ├── Connection.cpp
│   │   ├── TcpServer.cpp
│   │   ├── RecvBuffer.cpp
│   │   ├── MessageDispatcher.cpp
│   │   ├── ConnectionManager.cpp
│   │   ├── Packet.cpp
│   │   └── PacketParser.cpp
│
│   ├── services
│   │   ├── GachaService.cpp
│   │   ├── LoginService.cpp
│   │   └── ServiceManager.cpp
│
└── main.cpp