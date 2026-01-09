#!/usr/bin/env bash
set -e

# Test script for Lite KV Store
# Requires redis-cli to be installed

PORT=${1:-6379}

echo "Running tests against port $PORT..."
echo "=================================="

# Filter out comment lines and pipe to redis-cli
sed '/^#/d' << 'EOF' | redis-cli -p $PORT
# Clear existing data
FLUSHALL

# Test: General Commands
PING
ECHO "Hello World"

# Test: String Operations
SET username alice
GET username
SET counter 100
GET counter
KEYS *
TYPE username
DEL counter
GET counter
SET session abc123
EXPIRE session 60
RENAME username user

# Test: List Operations
RPUSH tasks "task1" "task2" "task3"
LGET tasks
LLEN tasks
LPUSH tasks "urgent"
RPUSH tasks "later"
LPOP tasks
RPOP tasks
LINDEX tasks 0
LINDEX tasks -1
LSET tasks 1 "updated"
LGET tasks
RPUSH nums 1 2 1 3 1 4 1
LREM nums 2 1
LGET nums

# Test: Hash Operations
HSET user:1 name "Bob"
HSET user:1 email "bob@example.com"
HSET user:1 age "30"
HGET user:1 name
HGET user:1 email
HEXISTS user:1 name
HEXISTS user:1 phone
HDEL user:1 age
HEXISTS user:1 age
HLEN user:1
HKEYS user:1
HVALS user:1
HGETALL user:1
HMSET user:2 name "Eve" city "NYC" role "admin"
HGETALL user:2

# Cleanup
FLUSHALL
EOF

echo ""
echo "=================================="
echo "Tests completed!"
