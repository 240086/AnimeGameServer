
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
│  │  ├─ ErrorCode.h
│  │  ├─ logger
│  │  │  └─ Logger.h
│  │  ├─ metrics
│  │  │  └─ ServerMetrics.h
│  │  ├─ random
│  │  │  └─ RandomEngine.h
│  │  └─ thread
│  │     ├─ GlobalThreadPool.h
│  │     └─ ThreadPool.h
│  ├─ database
│  │  ├─ mysql
│  │  │  ├─ MySQLConnection.h
│  │  │  ├─ MySQLConnectionPool.h
│  │  │  └─ MySQLResult.h
│  │  ├─ player
│  │  │  ├─ PlayerLoader.h
│  │  │  └─ PlayerSaver.h
│  │  ├─ queue
│  │  │  └─ SaveQueue.h
│  │  ├─ redis
│  │  │  ├─ PlayerCache.h
│  │  │  ├─ RedisClient.h
│  │  │  ├─ RedisKeyManager.h
│  │  │  └─ RedisPool.h
│  │  ├─ repository
│  │  │  ├─ AccountRepository.h
│  │  │  └─ PlayerRepository.h
│  │  ├─ task
│  │  │  ├─ DatabaseTask.h
│  │  │  └─ SavePlayerTask.h
│  │  └─ worker
│  │     ├─ DBWorker.h
│  │     └─ DBWorkerPool.h
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
│  │     ├─ PlayerDirtyFlag.h
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
│  │  │  ├─ ErrorSender.h
│  │  │  ├─ IMessage.h
│  │  │  ├─ MessageDecoder.h
│  │  │  ├─ MessageId.h
│  │  │  ├─ MessageMacro.h
│  │  │  ├─ MessageRegistry.h
│  │  │  ├─ messages
│  │  │  │  └─ LoginMessage.h
│  │  │  ├─ Packet.h
│  │  │  ├─ PacketParser.h
│  │  │  ├─ proto
│  │  │  │  ├─ common.proto
│  │  │  │  ├─ gacha.proto
│  │  │  │  ├─ heartbeat.proto
│  │  │  │  └─ login.proto
│  │  │  ├─ ProtocolRegistry.h
│  │  │  └─ ProtoMessage.h
│  │  ├─ session
│  │  │  ├─ Session.h
│  │  │  └─ SessionManager.h
│  │  └─ TcpServer.h
│  └─ services
│     ├─ BaseService.h
│     ├─ GachaService.h
│     ├─ HeartbeatService.h
│     ├─ IdempotencyService.h
│     ├─ LoginService.h
│     └─ ServiceManager.h
├─ README.md
├─ src
│  ├─ common
│  │  ├─ config
│  │  │  └─ Config.cpp
│  │  ├─ logger
│  │  │  └─ Logger.cpp
│  │  ├─ metrics
│  │  │  └─ ServerMetrics.cpp
│  │  ├─ random
│  │  │  └─ RandomEngine.cpp
│  │  └─ thread
│  │     ├─ GlobalThreadPool.cpp
│  │     └─ ThreadPool.cpp
│  ├─ database
│  │  ├─ mysql
│  │  │  ├─ MySQLConnection.cpp
│  │  │  └─ MySQLConnectionPool.cpp
│  │  ├─ player
│  │  │  ├─ PlayerLoader.cpp
│  │  │  └─ PlayerSaver.cpp
│  │  ├─ queue
│  │  │  └─ SaveQueue.cpp
│  │  ├─ redis
│  │  │  ├─ PlayerCache.cpp
│  │  │  ├─ RedisClient.cpp
│  │  │  └─ RedisPool.cpp
│  │  ├─ repository
│  │  │  ├─ AccountRepository.cpp
│  │  │  └─ PlayerRepository.cpp
│  │  ├─ task
│  │  │  └─ SavePlayerTask.cpp
│  │  └─ worker
│  │     ├─ DBWorker.cpp
│  │     └─ DBWorkerPool.cpp
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
│  │  │  ├─ PacketParser.cpp
│  │  │  └─ ProtocolRegistry.cpp
│  │  ├─ session
│  │  │  ├─ Session.cpp
│  │  │  └─ SessionManager.cpp
│  │  └─ TcpServer.cpp
│  └─ services
│     ├─ GachaService.cpp
│     ├─ HeartbeatService.cpp
│     ├─ IdempotencyService.cpp
│     ├─ LoginService.cpp
│     └─ ServiceManager.cpp
├─ Structure.md
├─ tests
└─ third_party

```