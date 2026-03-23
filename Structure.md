
```
AnimeGameServer
├─ CMakeLists.txt
├─ config
│  ├─ gacha_pool.yaml
│  └─ server.yaml
├─ include
│  ├─ common
│  │  ├─ metrics
│  │  │  └─ ServerMetrics.h
│  │  └─ random
│  │     └─ RandomEngine.h
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
│  │  ├─ dispatcher
│  │  │  └─ MessageDispatcher.h
│  │  ├─ manager
│  │  │  └─ ConnectionManager.h
│  │  ├─ protocol
│  │  │  ├─ ErrorSender.h
│  │  │  ├─ MessageContext.h
│  │  │  ├─ MessageDecoder.h
│  │  │  ├─ MessageMacro.h
│  │  │  ├─ MessageRegistry.h
│  │  │  ├─ messages
│  │  │  │  └─ LoginMessage.h
│  │  │  ├─ proto
│  │  │  │  ├─ common.proto
│  │  │  │  ├─ gacha.proto
│  │  │  │  ├─ heartbeat.proto
│  │  │  │  └─ login.proto
│  │  │  ├─ ProtocolRegistry.h
│  │  │  └─ ResponseSender.h
│  │  └─ session
│  │     ├─ Session.h
│  │     └─ SessionManager.h
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
│  │  ├─ metrics
│  │  │  └─ ServerMetrics.cpp
│  │  └─ random
│  │     └─ RandomEngine.cpp
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
│  │  ├─ dispatcher
│  │  │  └─ MessageDispatcher.cpp
│  │  ├─ manager
│  │  │  └─ ConnectionManager.cpp
│  │  ├─ protocol
│  │  │  └─ ProtocolRegistry.cpp
│  │  └─ session
│  │     ├─ Session.cpp
│  │     └─ SessionManager.cpp
│  └─ services
│     ├─ GachaService.cpp
│     ├─ HeartbeatService.cpp
│     ├─ IdempotencyService.cpp
│     ├─ LoginService.cpp
│     └─ ServiceManager.cpp
├─ Structure.md
├─ tests
└─ third_party
   └─ AnimeCore
      ├─ CMakeLists.txt
      ├─ include
      │  ├─ common
      │  │  ├─ config
      │  │  │  └─ Config.h
      │  │  ├─ ErrorCode.h
      │  │  ├─ logger
      │  │  │  └─ Logger.h
      │  │  ├─ metrics
      │  │  │  ├─ Metrics.h
      │  │  │  └─ MetricsReporter.h
      │  │  └─ thread
      │  │     ├─ GlobalThreadPool.h
      │  │     └─ ThreadPool.h
      │  └─ network
      │     ├─ asio
      │     │  └─ AsioContextPool.h
      │     ├─ buffer
      │     │  └─ RecvBuffer.h
      │     ├─ Connection.h
      │     ├─ protocol
      │     │  ├─ ClientPacket.h
      │     │  ├─ ClientPacketParser.h
      │     │  ├─ IMessage.h
      │     │  ├─ InternalPacket.h
      │     │  ├─ InternalPacketParser.h
      │     │  ├─ MessageId.h
      │     │  ├─ PacketParser.h
      │     │  └─ ProtoMessage.h
      │     └─ TcpServer.h
      ├─ src
      │  ├─ common
      │  │  ├─ logger
      │  │  │  └─ Logger.cpp
      │  │  ├─ metrics
      │  │  │  ├─ Metrics.cpp
      │  │  │  └─ MetricsReporter.cpp
      │  │  └─ thread
      │  │     ├─ GlobalThreadPool.cpp
      │  │     └─ ThreadPool.cpp
      │  └─ network
      │     ├─ asio
      │     │  └─ AsioContextPool.cpp
      │     ├─ buffer
      │     │  └─ RecvBuffer.cpp
      │     ├─ Connection.cpp
      │     ├─ protocol
      │     │  ├─ ClientPacket.cpp
      │     │  ├─ ClientPacketParser.cpp
      │     │  ├─ InternalPacket.cpp
      │     │  └─ InternalPacketParser.cpp
      │     └─ TcpServer.cpp
      └─ Structure.md

```