# ChainKV – Phase 4: Chain‑Replicated Key‑Value Storage Service

> Fault‑tolerant key‑value store in C with ZooKeeper‑backed chain replication.

## Table of Contents

1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [Directory Layout](#directory-layout)
4. [Build Prerequisites](#build-prerequisites)
5. [Building](#building)
6. [Running](#running)
7. [Client Commands](#client-commands)
8. [Failure Handling & Reconfiguration](#failure-handling--reconfiguration)
9. [Testing](#testing)
10. [Contributing](#contributing)
11. [References](#references)
12. [Authors](#authors)

---

## Project Overview

This repository contains **Phase 4** of the Distributed Systems course project.
The goal is to deliver a **fault‑tolerant, in‑memory key‑value store** that:

* Exposes a Java‑`Map`‑like interface (put, get, del, size, getKeys, getTable, stats).
* Replicates state **synchronously** along a server chain (the *head* handles writes, the *tail* serves reads).
* Detects membership changes and re‑configures automatically using **Apache ZooKeeper**.
* Serialises all client–server messages with **Google Protocol Buffers**.
* Handles thousands of concurrent clients through **multi‑threaded** request processing in C11 (pthreads).

The previous phases provided data structures, (de)serialisation, a single‑server client/server prototype, and concurrency.
**Phase 4 builds on that code base to add replication, fault tolerance, and dynamic reconfiguration.**

---

## Architecture

```text
          +---------- clients ----------+
          |                             |
          | writes (put/del)            | reads (get/size/…)
          v                             v
 +------- HEAD -------+  --->  +-- Mid ---+  --->  +------ TAIL ------+
 | Server 1           |        | Server 2 |        | Server 3         |
 +--------------------+        +----------+        +------------------+
         ^                        ^  ^                       |
         | ZooKeeper watches      |  | ZooKeeper watches     |
         +------------------------+  +-----------------------+
                   ZooKeeper coordination service
```

* **Chain Replication** (van Renesse & Schneider, 2004) guarantees *linearizable* writes and *consistent* reads.
* **ZooKeeper** acts as a *single source of truth* for live servers (`/chain/nodeXXXXXX`), electing the head/tail and notifying participants of changes via *watches*.
* Each server embeds a light‑weight **client stub** to forward writes to its immediate successor.
* A background thread keeps statistics (req/s, uptime, memory usage) exposed through the `stats` command.

### Module Breakdown

| Phase | Folder                                                             | Key Files                                                                                        | What It Does                                                    |
| ----: | ------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------ | --------------------------------------------------------------- |
|     1 | `source/` & `include/`                                             | `table.*`, `list.*`, `block.*`, `entry.*`, `serialization.*`                                     | Hash‑table & list data structures + custom binary serialisation |
|     2 | `source/`                                                          | `client_network.*`, `server_network.*`, `client_stub.*`, `server_skeleton.*`, `htmessages.proto` | TCP framing, Protocol‑Buffers glue, remote stubs/skeletons      |
|     3 | `source/`                                                          | `stats.*` + thread extensions in network layer                                                   | Worker thread‑pool, mutex‑protected shared state, live stats    |
|     4 | `include/zookeeper_client.h`, modifications across server & client | Chain Replication logic, ZooKeeper coordination, join/leave handling                             |                                                                 |

---

## Directory Layout

```
.
├── include/         # Public headers (exported API)
├── source/          # C89/C11 implementation files
├── ZooKeeper/       # Mini‑examples for learning the ZK C API
├── proto/           # htmessages.proto (generated during build)
├── lib/             # Third‑party libs (optional)
├── object/          # *.o compiled objects   (git‑ignored)
├── binary/          # Final executables      (git‑ignored)
└── docs/            # Design notes & figures
```

> **Tip:** The obligatory `Makefile` (Phase 3) still works; just ensure `-lzookeeper_mt -lprotobuf-c -lpthread` are linked.

---

## Build Prerequisites

* **GCC 10+** or **Clang 12+**
* **GNU Make 4.x**
* **libprotobuf‑c >= 1.4**

  ```bash
  sudo apt install protobuf-c-compiler libprotobuf-c-dev
  ```
* **Apache ZooKeeper C client** (`libzookeeper-mt`)

  ```bash
  sudo apt install libzookeeper-mt-dev
  ```
* **pthreads** (usually in libc)

---

## Building

```bash
# clone & enter
git clone https://github.com/your‑org/chainkv.git
cd chainkv

# compile everything
make            # produces binary/{server,client}_hashtable

# optional: just a component
make server
make client
make clean
```

Under the hood `make`:

1. Generates `proto/htmessages.pb-c.[ch]` from `proto/htmessages.proto`.
2. Compiles source files from **all phases** into `.o` objects.
3. Links `server_hashtable` and `client_hashtable` with `-pthread -lprotobuf-c -lzookeeper_mt`.

---

## Running

### 1. Start ZooKeeper

```bash
docker run --rm -p 2181:2181 zookeeper:3.9
```

Or use a local `zkServer.sh start`.

### 2. Launch at least two servers

```bash
./binary/server_hashtable 9000 7 127.0.0.1:2181   # head
./binary/server_hashtable 9001 7 127.0.0.1:2181   # tail (initially)
```

### 3. Run the interactive client

```bash
./binary/client_hashtable 127.0.0.1:2181
```

### 4. Example session

```
> put key1 "hello"
Result: 0 (OK)

> get key1
"hello"

> size
1

> stats
Uptime: 44 s, Requests: 5, Chain length: 2
```

### 5. Failure demo

1. Kill the head (`Ctrl‑C`).
2. ZooKeeper promotes the next node to head; clients reconnect automatically.
3. Start a new server on port 9002 – it joins as tail and syncs state via `gettable`.

---

## Client Commands

| Command    | Args           | Description                 |
| ---------- | -------------- | --------------------------- |
| `put`      | `<key> <data>` | Insert or overwrite a value |
| `get`      | `<key>`        | Fetch a value               |
| `del`      | `<key>`        | Remove a key                |
| `size`     | –              | Return number of entries    |
| `getkeys`  | –              | List all keys               |
| `gettable` | –              | Dump the entire table       |
| `stats`    | –              | Show server statistics      |
| `quit`     | –              | Close the client            |

---

## Failure Handling & Reconfiguration

* **Sequential Ephemeral ZNodes** (`/chain/nodeXXXXXX`) encode the chain order.
* Servers **watch** `/chain` children:

  * **On node removal** → recompute successor; head/tail may change.
  * **On node addition** → if you are tail, you become predecessor of the new node; send full table snapshot for bootstrapping.
* Writes are **two‑phase** inside each server: local apply → forward to successor → acknowledge to predecessor/client, ensuring atomicity.

---

## Testing

* **Unit tests**: `make check` runs list/table serialization tests (Phase 1).
* **Integration tests**:`scripts/test_chain.sh` boots 3‑node chain, executes 1000 mixed operations and verifies state convergence.
* **Stress test**: `wrk` + custom Lua script issues concurrent puts/gets; target throughput ≥ 5 k ops/s on a 4‑core laptop.

---

## Contributing

Pull requests are welcome! Please open an issue first to discuss major changes.

* Follow the existing code style (`clang-format -i` before committing).
* Add tests for any new feature or bug fix.
* Update this README if your change affects configuration or usage.

---

## References

1. G. DeCandia *et al.* **Dynamo: Amazon’s Highly Available Key‑value Store**, SOSP 2007.
2. R. van Renesse & F. B. Schneider. **Chain Replication for Supporting High Throughput and Availability**, OSDI 2004.
3. Apache ZooKeeper – *The BookKeeper of Distributed Systems*.
4. Google Protocol Buffers – Language‑neutral, platform‑neutral extensible mechanism for serialising structured data.
5. K. Kernighan & D. Ritchie. **The C Programming Language**, 2nd Ed., 1988.

---

## Authors

*Miguel Angel (fc65675) – Gonçalo Moreira (fc56571) – Liliana Valente (fc59846)*
Distributed Systems · Faculdade de Ciências · Universidade de Lisboa

---

*© 2025 – Released under the MIT License.*
