# Socket Performance Analysis


---

## Quick overview ✅

This repository contains C implementations for (TCP client/server variants) and a simple perf-based performance analysis.

Main source files (examples):

* `single_thread_server.c`, `single_thread_client.c`
* `multi_thread_server.c`, `multi_thread_client.c`
* `server_select.c`, `client_select.c`
* `Makefile` and `CMakeLists.txt` to build the project. ([GitHub][1])

---

## Prerequisites 🛠

* Linux (tested on modern distributions)
* `gcc` (≥ 9 recommended)
* `make` (if using Makefile)
* `cmake` (optional)
* `perf` (for performance analysis)
* `taskset` (to pin processes to specific CPU cores)
* `pthread` (multithreaded server compiled with `-lpthread`)

---

## Build — recommended (Makefile) ⚙️

From the repo root:

```bash
# build everything (Makefile present in repo)
make
```

If `make` isn't available, compile manually as below.

### Manual compile examples

```bash
# single-threaded
gcc -o single_thread_server single_thread_server.c
gcc -o single_thread_client single_thread_client.c

# multi-threaded (link pthread)
gcc -o multi_thread_server multi_thread_server.c -lpthread
gcc -o multi_thread_client multi_thread_client.c -lpthread

# select-based
gcc -o server_select server_select.c
gcc -o client_select client_select.c
```

---

## Run — basic examples ▶️

> Run server and client on separate VMs/containers or separate terminals. Use `taskset` to pin processes to specific CPUs for performance measurements.

### 1) Start server (example: port `9000`)

```bash
# pin server to CPU 0
taskset -c 0 ./single_thread_server 9000
# or
taskset -c 0 ./multi_thread_server 9000
# or select-based
taskset -c 0 ./server_select 9000
```

### 2) Run client (example: start `n` concurrent clients)

```bash
# common pattern: client <server_ip> <port> <n>
# (check each source's usage header/comments — exact args may vary)
./single_thread_client 9000 <num_requests>
# spawn multiple concurrent client processes from one host:
for i in $(seq 1 10); do ./multi_thread_client 192.168.1.10 9000 1 & done
```

```bash
./multi_threaded_client.c <number of threads>

```

> Note: argument formats may differ slightly between client implementations. If a program prints a usage line or expects different params, follow the comments at the top of the `.c` file.

---

## Using `taskset` (CPU pinning) 🧷

Pin a process to CPU 0:

```bash
taskset -c 0 ./multi_thread_server 9000
```

Pin client processes to different cores:

```bash
taskset -c 1 ./multi_thread_client 192.168.1.10 9000 1
```

---

## Basic `perf` usage for measurement 📊

### Installing perf:
```bash
sudo apt install linux-tools-common linux-tools-generic
```

Collect high-level stats:

```bash
# run server, then on same host:
sudo taskset -c 0 perf stat \
-e task-clock,context-switches,cpu-migrations,cpu-cycles,page-faults,instructions,cache-misses,branch-misses \
<executable> <args>
```

For the assignment you can compare:

* single-threaded server
* concurrent multithreaded server
* select-based server

Collect results across varying concurrent clients (`n`) and pinned CPU configurations; aggregate into `documentation.pdf`.

---

## Troubleshooting & tips 🪛

* If a program fails at bind/accept, ensure firewall is disabled or port is free and server listens on the correct interface (`0.0.0.0` vs `127.0.0.1`).
* If `make` fails, run the manual `gcc` commands above and check compilation errors (missing headers, wrong flags).
* Use `strace -f ./program args...` to debug syscalls.
* Confirm required permissions for `perf` (may require `sudo` or enabled perf events).
