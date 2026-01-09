# Lite KV Store

A lightweight, Redis-compatible key-value store implemented in C++17. This project demonstrates core concepts of database internals, network programming, and concurrent systems.

## Features

- **Multiple Data Types**: Strings, Lists, and Hashes
- **RESP Protocol**: Compatible with standard Redis clients (`redis-cli`)
- **Multi-threaded**: Handles concurrent client connections
- **Persistence**: Automatic background snapshots every 5 minutes
- **Key Expiration**: TTL support for automatic key cleanup
- **Graceful Shutdown**: Data persistence on SIGINT (Ctrl+C)

## Supported Commands

### General
| Command | Description |
|---------|-------------|
| `PING` | Test server connectivity |
| `ECHO <msg>` | Echo back the message |
| `FLUSHALL` | Clear all data |

### String Operations
| Command | Description |
|---------|-------------|
| `SET <key> <value>` | Store a string value |
| `GET <key>` | Retrieve a string value |
| `DEL <key>` | Delete a key |
| `KEYS` | List all keys |
| `TYPE <key>` | Get the type of a key |
| `EXPIRE <key> <sec>` | Set TTL on a key |
| `RENAME <old> <new>` | Rename a key |

### List Operations
| Command | Description |
|---------|-------------|
| `LPUSH <key> <val...>` | Push to front of list |
| `RPUSH <key> <val...>` | Push to back of list |
| `LPOP <key>` | Pop from front |
| `RPOP <key>` | Pop from back |
| `LLEN <key>` | Get list length |
| `LINDEX <key> <idx>` | Get element at index |
| `LSET <key> <idx> <val>` | Set element at index |
| `LREM <key> <count> <val>` | Remove elements |
| `LGET <key>` | Get all elements |

### Hash Operations
| Command | Description |
|---------|-------------|
| `HSET <key> <field> <val>` | Set hash field |
| `HGET <key> <field>` | Get hash field |
| `HDEL <key> <field>` | Delete hash field |
| `HEXISTS <key> <field>` | Check field existence |
| `HGETALL <key>` | Get all field-value pairs |
| `HKEYS <key>` | Get all field names |
| `HVALS <key>` | Get all values |
| `HLEN <key>` | Get number of fields |
| `HMSET <key> <f1> <v1>...` | Set multiple fields |

## Building

### Prerequisites
- C++17 compiler (GCC 7+ or Clang 5+)
- POSIX-compliant system (Linux, macOS)
- Make

### Compile
```bash
git clone https://github.com/yourusername/lite-kvstore.git
cd lite-kvstore
make
```

### Build with Debug Symbols
```bash
make debug
```

### Clean
```bash
make clean
```

## Usage

### Start the Server
```bash
# Default port 6379
./lite-kvstore

# Custom port
./lite-kvstore 6380
```

### Connect with redis-cli
```bash
redis-cli -p 6379
```

### Example Session
```
127.0.0.1:6379> SET greeting "Hello, World!"
OK
127.0.0.1:6379> GET greeting
"Hello, World!"
127.0.0.1:6379> LPUSH queue job1 job2 job3
(integer) 3
127.0.0.1:6379> LGET queue
1) "job3"
2) "job2"
3) "job1"
127.0.0.1:6379> HSET user:100 name "Alice" email "alice@example.com"
(integer) 1
127.0.0.1:6379> HGETALL user:100
1) "name"
2) "Alice"
3) "email"
4) "alice@example.com"
127.0.0.1:6379> PING
PONG
```

### Run Tests
```bash
chmod +x tests/test_commands.sh
./tests/test_commands.sh
```

## Project Structure
```
lite-kvstore/
├── include/
│   ├── KVStore.h          # Data storage engine
│   ├── CommandProcessor.h # RESP parser & command router
│   └── KVServer.h         # TCP server
├── src/
│   ├── main.cpp           # Entry point
│   ├── KVStore.cpp        # Storage implementation
│   ├── CommandProcessor.cpp # Command handlers
│   └── KVServer.cpp       # Network layer
├── tests/
│   └── test_commands.sh   # Integration tests
├── Makefile
├── README.md
├── LICENSE
└── .gitignore
```

## Architecture

```
┌─────────────┐     ┌──────────────────┐     ┌──────────┐
│ Redis CLI   │────▶│  KVServer        │────▶│ KVStore  │
│ (Client)    │◀────│  (TCP/Threading) │◀────│ (Data)   │
└─────────────┘     └──────────────────┘     └──────────┘
                            │
                    ┌───────▼────────┐
                    │ CommandProcessor│
                    │ (RESP Parser)   │
                    └────────────────┘
```

### Components

1. **KVServer**: TCP socket server that accepts connections and spawns worker threads
2. **CommandProcessor**: Parses RESP protocol and routes to command handlers
3. **KVStore**: Thread-safe singleton storing strings, lists, and hashes

### Persistence Format
Data is saved to `snapshot.kvdb` in a simple text format:
```
S key value           # String
L listkey item1 item2 # List
H hashkey f1:v1 f2:v2 # Hash
```

## Limitations

- Single-node only (no clustering/replication)
- No transactions or pub/sub
- Values containing spaces have limited persistence support
- Not compatible with RDB/AOF format

## Contributing

Contributions welcome! Please open an issue or submit a pull request.

## License

MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- Inspired by [Redis](https://redis.io/)
- Built for learning database internals and systems programming
