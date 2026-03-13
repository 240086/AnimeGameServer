
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
│  │     ├─ PlayerCommandQueue.h
│  │     ├─ PlayerLogicLoop.h
│  │     └─ PlayerManager.h
│  ├─ network
│  │  ├─ buffer
│  │  │  └─ RecvBuffer.h
│  │  ├─ Connection.h
│  │  ├─ dispatcher
│  │  │  └─ MessageDispatcher.h
│  │  ├─ manager
│  │  │  └─ ConnectionManager.h
│  │  ├─ protocol
│  │  │  ├─ MessageId.h
│  │  │  ├─ Packet.h
│  │  │  └─ PacketParser.h
│  │  └─ TcpServer.h
│  └─ services
│     ├─ BaseService.h
│     ├─ GachaService.h
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
│  ├─ game
│  │  ├─ actor
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
│  │     ├─ PlayerCommandQueue.cpp
│  │     ├─ PlayerLogicLoop.cpp
│  │     └─ PlayerManager.cpp
│  ├─ main.cpp
│  ├─ network
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
│  │  └─ TcpServer.cpp
│  └─ services
│     ├─ GachaService.cpp
│     ├─ LoginService.cpp
│     └─ ServiceManager.cpp
├─ Structure.md
├─ tests
│  ├─ TestGacha.cpp
│  └─ TestMain.cpp
└─ third_party

```