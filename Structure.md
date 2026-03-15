
```
AnimeGameServer
├─ CMakeLists.txt
├─ config
│  ├─ gacha_pool.yaml
│  └─ server.yaml
├─ include
│  ├─ common
│  │  ├─ config
│  │  │  └─ Config.h
│  │  ├─ logger
│  │  │  └─ Logger.h
│  │  ├─ random
│  │  │  └─ RandomEngine.h
│  │  └─ thread
│  │     ├─ GlobalThreadPool.h
│  │     └─ ThreadPool.h
│  ├─ database
│  │  ├─ mysql
│  │  │  ├─ MySQLConnection.h
│  │  │  └─ MySQLConnectionPool.h
│  │  ├─ redis
│  │  │  └─ RedisClient.h
│  │  └─ repository
│  │     ├─ AccountRepository.h
│  │     └─ PlayerRepository.h
│  ├─ game
│  │  ├─ actor
│  │  │  ├─ Actor.h
│  │  │  ├─ ActorSystem.h
│  │  │  ├─ Mailbox.h
│  │  │  └─ PlayerActor.h
│  │  ├─ gacha
│  │  │  ├─ GachaItem.h
│  │  │  ├─ GachaPool.h
│  │  │  ├─ GachaPoolManager.h
│  │  │  ├─ GachaSystem.h
│  │  │  └─ PitySystem.h
│  │  └─ player
│  │     ├─ Currency.h
│  │     ├─ GachaHistory.h
│  │     ├─ Inventory.h
│  │     ├─ Player.h
│  │     └─ PlayerManager.h
│  ├─ network
│  │  ├─ asio
│  │  │  └─ AsioContextPool.h
│  │  ├─ buffer
│  │  │  └─ RecvBuffer.h
│  │  ├─ Connection.h
│  │  ├─ dispatcher
│  │  │  └─ MessageDispatcher.h
│  │  ├─ manager
│  │  │  └─ ConnectionManager.h
│  │  ├─ protocol
│  │  │  ├─ generated
│  │  │  │  ├─ gacha.pb.cc
│  │  │  │  ├─ gacha.pb.h
│  │  │  │  ├─ heartbeat.pb.cc
│  │  │  │  ├─ heartbeat.pb.h
│  │  │  │  ├─ login.pb.cc
│  │  │  │  └─ login.pb.h
│  │  │  ├─ MessageId.h
│  │  │  ├─ Packet.h
│  │  │  ├─ PacketParser.h
│  │  │  └─ proto
│  │  │     ├─ gacha.proto
│  │  │     ├─ heartbeat.proto
│  │  │     └─ login.proto
│  │  ├─ session
│  │  │  ├─ Session.h
│  │  │  └─ SessionManager.h
│  │  └─ TcpServer.h
│  └─ services
│     ├─ BaseService.h
│     ├─ GachaService.h
│     ├─ HeartbeatService.h
│     ├─ LoginService.h
│     └─ ServiceManager.h
├─ README.md
├─ src
│  ├─ common
│  │  ├─ config
│  │  │  └─ Config.cpp
│  │  ├─ logger
│  │  │  └─ Logger.cpp
│  │  ├─ random
│  │  │  └─ RandomEngine.cpp
│  │  └─ thread
│  │     ├─ GlobalThreadPool.cpp
│  │     └─ ThreadPool.cpp
│  ├─ database
│  │  ├─ mysql
│  │  │  ├─ MySQLConnection.cpp
│  │  │  └─ MySQLConnectionPool.cpp
│  │  ├─ redis
│  │  │  └─ RedisClient.cpp
│  │  └─ repository
│  │     ├─ AccountRepository.cpp
│  │     └─ PlayerRepository.cpp
│  ├─ game
│  │  ├─ actor
│  │  │  ├─ Actor.cpp
│  │  │  └─ ActorSystem.cpp
│  │  ├─ gacha
│  │  │  ├─ GachaPool.cpp
│  │  │  ├─ GachaPoolManager.cpp
│  │  │  ├─ GachaSystem.cpp
│  │  │  └─ PitySystem.cpp
│  │  └─ player
│  │     ├─ Currency.cpp
│  │     ├─ GachaHistory.cpp
│  │     ├─ Inventory.cpp
│  │     ├─ Player.cpp
│  │     └─ PlayerManager.cpp
│  ├─ main.cpp
│  ├─ network
│  │  ├─ asio
│  │  │  └─ AsioContextPool.cpp
│  │  ├─ buffer
│  │  │  └─ RecvBuffer.cpp
│  │  ├─ Connection.cpp
│  │  ├─ dispatcher
│  │  │  └─ MessageDispatcher.cpp
│  │  ├─ manager
│  │  │  └─ ConnectionManager.cpp
│  │  ├─ protocol
│  │  │  ├─ Packet.cpp
│  │  │  └─ PacketParser.cpp
│  │  ├─ session
│  │  │  ├─ Session.cpp
│  │  │  └─ SessionManager.cpp
│  │  └─ TcpServer.cpp
│  └─ services
│     ├─ GachaService.cpp
│     ├─ HeartbeatService.cpp
│     ├─ LoginService.cpp
│     └─ ServiceManager.cpp
├─ Structure.md
├─ tests
│  ├─ TestMain.cpp
│  └─ unit
│     └─ TestCurrency.cpp
└─ third_party

```