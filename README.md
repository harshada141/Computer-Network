# Computer Network: TCP & Chat Architecture

A collection of low-level networking systems built from scratch in C++ to understand the plumbing of the internet. By bypassing standard OS socket shortcuts and dealing directly with memory, synchronization, and raw packets, this project explores the fundamental protocols that power modern networks.

## Features

### 1. Multi-threaded Chat Server (`/src/chat_server`)
A thread-safe, concurrent chat server built with POSIX Sockets.
- **Concurrency:** Implements a thread-per-connection model using C++11 `std::thread`.
- **Race Condition Prevention:** Utilizes `std::mutex` to ensure thread-safe read/writes to shared state (message history, active client lists).
- **Features:** Supports private messaging, global broadcasting, and dynamic group creation.

### 2. Raw TCP 3-Way Handshake (`/src/raw_tcp`)
A raw implementation of the TCP protocol bypassing standard OS transport layer wrappers.
- **Raw Sockets:** Utilizes `SOCK_RAW` to manually construct network packets.
- **Header Crafting:** Manually builds IP Headers and TCP Headers byte-by-byte.
- **Checksum Calculation:** Implements the TCP pseudo-header to manually calculate and verify checksums.
- **Handshake Execution:** Successfully executes the SYN -> SYN-ACK -> ACK TCP initialization sequence.


## Tech Stack
- **Language:** C++
- **Networking:** POSIX Sockets, TCP/IP, Raw Sockets
- **Concurrency:** Multi-threading, Mutex Locks

## Architecture (Chat Server)
```mermaid
flowchart TD
    A[Start Server] --> B[Load user credentials]
    B --> C[Create & Bind TCP socket]
    C --> D[Listen for connections]
    D --> E[Accept client]
    E --> F[Spawn isolated thread]
    F --> G[Authenticate & Mutex Lock]
    G --> H[Process /msg, /broadcast, /group]
