# Performance & Architecture Audit (AnimeGameServer)

## 1. Key bottlenecks found

### 1.1 DB worker used one connection acquisition per single task
- Previous flow handled one task per queue pop and acquired/released one DB connection per task.
- This increases lock contention in connection pool and increases context-switch overhead under high concurrency.
- Fix in this patch: shard worker now pops a batch (`PopBatch`) and reuses one connection for up to 32 tasks.

### 1.2 MySQL connection objects had no network timeout control
- Without connect/read/write timeout limits, slow network or transient DB issues can block worker progress too long.
- Fix in this patch: add connect/read/write timeout options in `MySQLConnection` constructor and guard against failed `mysql_init`.

### 1.3 Connection pool init was not idempotent/thread-safe enough for repeated init paths
- `Init` lacked explicit `initialized_` gate.
- Fix in this patch: add `initialized_` and lock-protected single-init behavior.

### 1.4 Inventory save had a SQL edge-case bug
- When all item counts are non-positive, SQL became `INSERT ... VALUES ON DUPLICATE ...` (invalid SQL).
- Fix in this patch: skip execution if no valid tuples are produced.

## 2. Framework/module redundancy audit

### 2.1 Build dependency redundancy
- `absl` was required in CMake but never used in source.
- Fix in this patch: remove `find_package(absl REQUIRED CONFIG)`.

### 2.2 Protobuf generation incompleteness
- `heartbeat.proto` existed but wasn't included in CMake protobuf generation list.
- Fix in this patch: include heartbeat proto in `PROTO_FILES`.

### 2.3 Suspected currently-unused runtime modules
- `RedisClient` appears defined but not referenced by business flow.
- `AccountRepository` appears implemented but not wired into login path.

> Recommendation: if these are roadmap features, keep with clear TODO labels; if not needed in current milestone, remove to reduce cognitive load and compile surface.

## 3. Industry-grade optimization roadmap (next step)

1. SQL layer
   - move from string concatenation to prepared statements + parameter binding;
   - use batch upsert windows with bounded size (e.g., 100~500 rows) and fallback retry;
   - add `INSERT ... ON DUPLICATE` hot-key metrics and slow SQL sampling.

2. Storage architecture
   - split read/write paths: Redis cache for hot player profile, MySQL async flush (write-behind);
   - add idempotent version field (`row_version`) to avoid stale write overwrite;
   - separate gacha history to append-only partitioned table.

3. Concurrency model
   - keep actor single-thread ownership per player;
   - add per-shard adaptive batching (dynamic batch size by backlog depth);
   - enforce backpressure policy (drop/coalesce non-critical dirty flags when queue is overloaded).

4. Observability
   - dashboard metrics: queue backlog percentiles, worker batch size histogram, DB wait/acquire latency, commit latency;
   - add alert thresholds: sustained backlog, failed transaction rate, p99 DB latency.

## 4. Quality guardrails

- keep transaction boundaries in `PlayerSaver::Save` (already present);
- add integration tests for transaction rollback and partial dirty-flag save paths;
- add stress test scenario: 10k concurrent sessions + mixed dirty flags + chaos DB latency injection.

