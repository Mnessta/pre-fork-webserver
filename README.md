# Pre-Forking Web Server in C (Linux)

## 📌 Description

This project implements a **pre-forking web server** in C, using **UNIX domain sockets** and dynamic process pool management. It is designed to efficiently handle HTTP requests with a pool of child processes and self-regulate based on traffic load.

---

## 🧩 Features

- Pre-forked process pool model.
- Each child process independently calls `accept()` on the shared listening socket.
- Parent monitors and regulates child processes using:
  - `MinSpareServers`
  - `MaxSpareServers`
  - `MaxRequestsPerChild`
- Uses **exponential backoff** for process creation when idle processes are below the minimum.
- Child process:
  - Handles a client connection.
  - Logs PID, client IP, and port.
  - Sleeps for 1 second.
  - Sends a dummy HTTP response.
- Parent process:
  - Logs process pool changes and statuses.
  - Communicates with children using **UNIX domain sockets**.
  - Cleans up zombie processes.
- Graceful shutdown using `Ctrl-C`:
  - Displays active children and request stats.
- Compatible with `ab` or `httperf` for load testing.

---

## ⚙️ Command-Line Arguments

```bash
./webserver <PORT> <MinSpareServers> <MaxSpareServers> <MaxRequestsPerChild>
