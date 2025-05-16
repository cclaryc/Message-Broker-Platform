# Message Broker Platform (TCP/UDP)

This project implements a topic-based messaging broker using TCP and UDP sockets
##  Project Structure

```
.
├── server.c                  # Core broker logic (TCP/UDP)
├── subscriber.c              # TCP client with subscribe/unsubscribe commands
├── udp_client.py             # UDP publisher simulator
├── test.py / testtest.py     # Python testing utilities
├── mypayloads.json           # Custom message sets for testing
├── sample_payloads.json
├── sample_wildcard_payloads.json
├── three_topics_payloads.json
├── utils/                    # Utility functions
├── .vscode/                  # VSCode config (optional)
├── pcom_hw2_udp_client/      # Provided UDP client (original)
├── Makefile                  # Build automation for C components
├── server / subscriber       # Compiled binaries (gitignored ideally)
├── README.md                 # You're reading it!
└── readme.txt                # Original assignment description
```

---

##  Build Instructions

```bash
make
```

This builds `server` and `subscriber`. Run `make clean` to remove binaries.

---

##  How to Run

### 1. Start the server
```bash
./server <PORT>
```

### 2. Start a subscriber client
```bash
./subscriber <ID_CLIENT> <SERVER_IP> <PORT>
```

### 3. Publish via UDP (Python)
```bash
python3 udp_client.py <SERVER_IP> <PORT> sample_payloads.json
```

---

## Protocol Details

### UDP Message Format
- **topic**: max 50 bytes (null-terminated)
- **type**: 1 byte (`INT`, `SHORT_REAL`, `FLOAT`, `STRING`)
- **payload**: depends on type (see `README.txt`)

### TCP Protocol
- Length-prefixed messages (`uint32_t` header)
- Commands: `subscribe <topic>`, `unsubscribe <topic>`, `exit`

---

## Testing

Use the Python scripts and provided JSON files to simulate message flows and wildcard behavior:

```bash
python3 test.py             # Custom tests
python3 udp_client.py ...   # Raw publish simulation
```

---
