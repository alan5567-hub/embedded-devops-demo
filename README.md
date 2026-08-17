# embedded-devops-demo

A minimal C project that simulates a **T-Box (Telematics Box) diagnostic system** — a component found in connected vehicles that reads and reports telemetry data.

This project is designed as a hands-on starting point for learning **Git**, **CI/CD**, and **DevOps** practices with a realistic embedded-systems flavour.

---

## Project Structure

```
embedded-devops-demo/
├── artifactory/
│   └── etc/system.yaml     # JFrog Artifactory configuration
├── jenkins/
│   └── Dockerfile          # Custom Jenkins image (LTS + Docker CLI + plugins)
├── CMakeLists.txt          # Build configuration (CMake + CTest)
├── docker-compose.yml      # Multi-service stack (Jenkins, JFrog Artifactory, Cloudflare Tunnel)
├── Dockerfile.build        # C build environment (Alpine + GCC + CMake)
├── Jenkinsfile             # Declarative CI/CD pipeline (Checkout → Build → Test → Publish)
├── .dockerignore           # Keeps the Docker context lean
├── main.c                  # Entry point — drives the diagnostic cycle
├── vehicle.c               # Vehicle logic: init, simulation, status, printing
├── vehicle.h               # Public API declarations and data types
├── .gitignore              # Ignores build artefacts, IDE files, and data volumes
├── ARTIFACTORY.md          # Full JFrog Artifactory setup & publish guide
├── JENKINS.md              # Full Jenkins setup and operation guide
└── README.md               # This file
```

---

## Building with Docker (Recommended)

> **No local build tools required.** GCC, CMake, and Make run entirely inside the container. The host only needs [Docker](https://docs.docker.com/get-docker/) installed.

### Step 1 — Build the Docker image (once)

```powershell
docker build -f Dockerfile.build -t tbox-builder .
```

### Step 2 — Compile the project

```powershell
docker run --rm -v ${PWD}:/workspace tbox-builder
```

The compiled binary is placed in `build/tbox_diagnostic` on your host.

### Step 3 — Run the CTest smoke test

```powershell
docker run --rm -v ${PWD}:/workspace tbox-builder `
  sh -c "cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel && ctest --test-dir build --output-on-failure"
```

### Step 4 — Run the executable

The binary is a Linux ELF binary; run it inside the container:

```powershell
docker run --rm -v ${PWD}:/workspace --entrypoint "" tbox-builder `
  sh -c "cmake -S . -B build -DCMAKE_BUILD_TYPE=Release --fresh && cmake --build build --parallel && ./build/tbox_diagnostic"
```

> **Tip (Linux/macOS hosts):** Replace `` ` `` (PowerShell line continuation) with `\` in bash.

---

## Building Without Docker (Manual)

### Prerequisites

| Tool   | Minimum Version | Notes                                      |
|--------|-----------------|---------------------------------------------|
| CMake  | 3.15            | [cmake.org](https://cmake.org/download/)    |
| GCC / MSVC / Clang | Any modern version | Any C11-capable compiler works |

### Linux / macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/tbox_diagnostic
```

### Windows (PowerShell)

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
.\build\Release\tbox_diagnostic.exe
```

---

## Sample Output

```
[T-Box] Initialising vehicle telemetry system...

========================================
  T-Box Diagnostic Report
========================================
  Engine       : ON
  Speed        : 72.3 km/h
  Battery      : 68.5 %
  Temperature  : 33.2 deg C
  Status       : NORMAL
========================================

[T-Box] Diagnostic cycle complete.
```

---

## What It Simulates

| Field       | Description                                     |
|-------------|-------------------------------------------------|
| Engine      | Derived from speed — ON if speed > 0            |
| Speed       | Randomised around 80 km/h ± 20                  |
| Battery     | Randomised around 75 % ± 10                     |
| Temperature | Randomised around 35 °C ± 7.5                   |
| Status      | `NORMAL`, `WARNING`, or `CRITICAL` based on thresholds |

### Status Thresholds

| Status     | Battery          | Temperature      |
|------------|------------------|------------------|
| `NORMAL`   | > 20 %           | < 70 °C          |
| `WARNING`  | 10 % – 20 %      | 70 °C – 90 °C    |
| `CRITICAL` | ≤ 10 %           | ≥ 90 °C          |

---

## Testing

The project uses **CTest** with a single smoke test (`smoke_test`) that:
1. Runs `tbox_diagnostic`
2. Verifies exit code is `0`
3. Checks that the output contains `T-Box Diagnostic Report`

Run tests (inside Docker):
```powershell
docker run --rm -v ${PWD}:/workspace tbox-builder `
  sh -c "cmake -S . -B build && cmake --build build && ctest --test-dir build -V"
```

---

## DevOps Stack (Jenkins, Artifactory & Cloudflare)

The project includes a complete containerized DevOps stack defined in [`docker-compose.yml`](./docker-compose.yml):

- **Jenkins CI (`http://localhost:8090`)** — Custom LTS image with Docker CLI to build sibling containers.
- **JFrog Artifactory (`http://localhost:8082`)** — C/C++ Community Edition artifact repository for versioned binary releases.
- **Cloudflare Quick Tunnel (`cloudflared`)** — Ephemeral public ingress (`*.trycloudflare.com`) forwarding to Jenkins for GitHub webhook triggers without needing public IP configuration or port forwarding.

> Detailed guides:
> - **[JENKINS.md](./JENKINS.md)** — Jenkins configuration, Docker socket permissions, credentials, and job setup.
> - **[ARTIFACTORY.md](./ARTIFACTORY.md)** — Repository creation, access tokens, and verification.

### Quick Start

```powershell
# 1. Start the complete stack
docker compose up -d

# 2. Get the initial Jenkins admin password (first run only)
docker exec tbox-jenkins cat /var/jenkins_home/secrets/initialAdminPassword

# 3. Access Services
# Jenkins UI:     http://localhost:8090
# Artifactory UI: http://localhost:8082 (default: admin / password)

# 4. View Cloudflare Quick Tunnel public URL (for GitHub Webhooks)
docker compose logs cloudflared
```

### Stack Management

```powershell
docker compose start         # Resume stopped containers
docker compose stop          # Pause all containers (data preserved in named volumes)
docker compose logs -f       # Stream all service logs
docker compose down          # Remove containers and networks (data volumes preserved)
docker compose down -v       # ⚠️ Remove containers AND wipe all persistent data volumes
```

### CI/CD Pipeline Stages

The Declarative Pipeline defined in [`Jenkinsfile`](./Jenkinsfile) executes on every push triggered via `githubPush()` or manual run:

| Stage | What it does |
|---|---|
| **Checkout** | Clones/updates repository from GitHub into Jenkins workspace |
| **Build** | Builds `tbox-builder` image; compiles C project inside sibling container using `--volumes-from` |
| **Test** | Runs CTest smoke test suite inside `tbox-builder` |
| **Publish** | Uploads compiled binary (`tbox_diagnostic`) to JFrog Artifactory via curl sibling container (`main` branch only) |

GCC, CMake, and Make never touch the host — they run inside Alpine sibling containers managed by Jenkins via the shared Docker socket.

---

## Learning Goals

This project is intentionally concise so you can focus on:

- **Git basics** — `init`, `add`, `commit`, `branch`, `merge`, and webhooks
- **CMake & CTest** — understanding modern C build systems and automated smoke tests
- **Docker Sibling Pattern** — running builds in isolated containers via `/var/run/docker.sock` and `--volumes-from`
- **Jenkins Pipelines** — declarative pipelines, environment variables, credentials injection, and push triggers
- **Artifact Management** — versioned binary publishing to JFrog Artifactory
- **Ephemeral Ingress** — exposing local services for webhooks using Cloudflare Quick Tunnels
- **Modular C** — clean separation of hardware/telemetry simulation (`vehicle.c`) from application entry point (`main.c`)

---

## Licence

MIT — free to use, modify, and distribute.
