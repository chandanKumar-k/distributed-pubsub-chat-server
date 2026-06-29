# 💬 Real-Time Distributed Chat Server Engine with Pub-Sub Broker Architecture

A high-performance, asynchronous network messaging broker built in native C++ using WinSock sockets and multi-threaded worker pools to handle stateful, in-memory topic channel routing.

## 🛠️ System Architecture & Pain Points Addressed
Traditional real-time chat architectures often fail under peak traffic conditions because they rely on slow disk-bound database writes and spawn heavy operating system threads sequentially for every incoming connection. This architecture resolves those core performance limitations through:

1. **Stateful In-Memory Processing (RAM)**: Operates entirely inside the computer's high-speed RAM cache to eliminate database write bottlenecks, handling messaging queues millions of times faster than standard architectures.
2. **Asynchronous Thread Workers (`std::thread`)**: Spawns detached, concurrent execution loops for every active client socket. This isolates background transaction payloads and leaves the primary network gateway free to accept new incoming users instantaneously.
3. **Publish-Subscribe (Pub-Sub) Topic Isolation**: Implements an intelligent traffic broker layout. Instead of searching a global user pool to route text packets, the engine dynamically partitions network streams into distinct topic channels (e.g., `#cse_placement_track`), optimizing fan-out messaging routing down to an O(1) memory lookup.

## ⚙️ Technical Skill Stack & Primitives
- **Network Interface Drivers**: Configured Windows Sockets API (`winsock2.h`) over custom local host TCP portals (`Port 8080`).
- **Memory Optimization**: Managed structural dynamic arrays (`std::vector`) and data reference pointers to tracks user connection states without leaking system RAM.
- **Protocol Command Router**: Implemented a text stream parsing logic layer to intercept real-time custom command string tokens (`/join`, `/name`, `/quit`).

## 🚀 Testing and Execution Guide
To test the multi-threaded message routing capabilities natively on a local operating system machine:

1. **Compile the C++ Broker Executable**:
   ```bash
   g++ chat_server.cpp -o Server -lws2_32
   ```
2. **Launch the Core Listening Process**:
   ```bash
   .\Server
   ```
3. **Simulate Concurrent Clients**:
   Connect via a terminal networking client or use a Python socket pipeline payload script inside separate console windows:
   ```python
   import socket
   s = socket.socket()
   s.connect(('localhost', 8080))
   print(s.recv(1024).decode())
   ```
