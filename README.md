# embedded-devops-demo

A minimal C project that simulates a **T-Box (Telematics Box) diagnostic system** — a component found in connected vehicles that reads and reports telemetry data.

This project is designed as a hands-on starting point for learning **Git** and **DevOps** practices with a realistic embedded-systems flavour.

---

## Project Structure

```
embedded-devops-demo/
├── jenkins/
│   └── Dockerfile          # Custom Jenkins image (LTS + Docker CLI + plugins)
├── CMakeLists.txt          # Build configuration (CMake + CTest)
├── docker-compose.yml      # Starts the Jenkins CI server
├── Dockerfile.build        # C build environment (Alpine + GCC + CMake)
├── Jenkinsfile             # CI pipeline (Checkout → Build → Test)
├── .dockerignore           # Keeps the Docker context lean
├── main.c                  # Entry point — drives the diagnostic cycle
├── vehicle.c               # Vehicle logic: init, simulation, status, printing
├── vehicle.h               # Public API declarations and data types
├── .gitignore              # Ignores build artefacts, IDE files, and jenkins_home/
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

## Jenkins CI (Local)

> Full setup guide → **[JENKINS.md](./JENKINS.md)**

### Quick start

```powershell
# 1. Build the Jenkins image and start the server
docker compose up -d --build

# 2. Get the initial admin password (first run only)
docker exec tbox-jenkins cat /var/jenkins_home/secrets/initialAdminPassword

# 3. Open the UI
start http://localhost:8080
```

### Start / Stop

```powershell
docker compose start   # resume a stopped Jenkins
docker compose stop    # pause (data preserved in named volume)
docker compose down    # remove container (data preserved)
docker compose down -v # ⚠️ remove container AND data volume
```

### Pipeline stages

| Stage | What it does |
|-------|-------------|
| **Checkout** | Clones the GitHub repo into the Jenkins workspace |
| **Build** | Builds `tbox-builder` image; compiles C project inside it |
| **Test** | Runs CTest smoke test inside `tbox-builder` |

GCC, CMake, and Make never touch the host — they run only inside the Alpine `tbox-builder` container, which is itself launched by Jenkins via the shared Docker socket.

---

## Learning Goals

This project is intentionally small so you can focus on:

- **Git basics** — `init`, `add`, `commit`, `branch`, `merge`
- **CMake** — understanding a real build system
- **CTest** — basic test infrastructure
- **Docker** — reproducible, host-agnostic build environments
- **Jenkins** — CI server, pipelines, Docker socket access, `Jenkinsfile`
- **Modular C** — separating logic (`vehicle.c`) from the entry point (`main.c`)
- **DevOps readiness** — the structure is ready to add SonarQube, JFrog, and more

---

## Licence

MIT — free to use, modify, and distribute.
