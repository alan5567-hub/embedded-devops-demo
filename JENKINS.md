# Jenkins CI Setup — embedded-devops-demo

This document explains how Jenkins is set up for the `embedded-devops-demo` project,
how it connects to Docker, and how to operate it day-to-day.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│  Windows Host (Docker Desktop)                      │
│                                                     │
│  ┌──────────────────────────────────────────────┐   │
│  │  tbox-jenkins container                      │   │
│  │  (jenkins/jenkins:lts-jdk17 + Docker CLI)    │   │
│  │                                              │   │
│  │  Port 8080 ──► http://localhost:8090         │   │
│  │  Volume: tbox-jenkins-data → /var/jenkins_home│  │
│  │                                              │   │
│  │  When a pipeline runs a `docker` command:    │   │
│  │  ┌────────────────────────────────────────┐  │   │
│  │  │  tbox-builder container (sibling)      │  │   │
│  │  │  (Alpine + GCC + CMake + Make)         │  │   │
│  │  │  --volumes-from tbox-jenkins           │  │   │
│  │  │  Sees workspace at same path           │  │   │
│  │  └────────────────────────────────────────┘  │   │
│  └──────────┬───────────────────────────────────┘   │
│             │ /var/run/docker.sock (bind-mount)      │
│  ┌──────────▼───────────┐                           │
│  │  Docker Engine       │  ◄── single daemon        │
│  └──────────────────────┘                           │
└─────────────────────────────────────────────────────┘
```

---

## How Jenkins Is Running

Jenkins runs as a **Docker container** named `tbox-jenkins`, defined in
[`docker-compose.yml`](./docker-compose.yml). It is based on the official
[`jenkins/jenkins:lts-jdk17`](https://hub.docker.com/r/jenkins/jenkins) image,
extended by [`jenkins/Dockerfile`](./jenkins/Dockerfile) to add:

- **Docker CLI** — so Jenkins can issue `docker build` / `docker run` commands
- **Pre-installed plugins** — Git, Pipeline, Docker Workflow, Stage View, Timestamps

Jenkins home data (jobs, build history, credentials, plugin configuration) is stored
in the named Docker volume `tbox-jenkins-data`, which survives container restarts.

---

## How Jenkins Accesses Docker

The Docker socket is **bind-mounted** from the host into the Jenkins container:

```yaml
volumes:
  - /var/run/docker.sock:/var/run/docker.sock
```

This means Jenkins does **not** run its own Docker daemon. Instead, it talks to
the **same Docker engine** that runs Jenkins itself. When Jenkins executes:

```sh
docker build -f Dockerfile.build -t tbox-builder .
```

…the command is processed by the host Docker engine via the socket. The result
(the built image) is visible in Docker Desktop just like any other image.

---

## How the Jenkinsfile Works

The pipeline has three stages:

### Stage 1 — Checkout

```groovy
checkout scm
```

Jenkins clones (or incrementally fetches) the GitHub repository configured in the job
into the workspace directory inside `/var/jenkins_home/workspace/<job-name>`.

### Stage 2 — Build

```groovy
// 1. Build the Alpine C build image (cached after first run)
sh 'docker build -f Dockerfile.build -t tbox-builder .'

// 2. Compile inside that image
sh """
    docker run --rm \
        --volumes-from tbox-jenkins \
        -w "${WORKSPACE}" \
        tbox-builder \
        sh -c "cmake -S . -B build ... && cmake --build build ..."
"""
```

**Key mechanism — `--volumes-from tbox-jenkins`:**

The Jenkins workspace lives in the named volume `tbox-jenkins-data`.
Because that volume is not on the host filesystem, a normal
`docker run -v /path/on/host:/workspace` would fail — Docker would look for
the path on the host disk, not inside the named volume.

The solution is `--volumes-from tbox-jenkins`: this mounts **all volumes**
that the `tbox-jenkins` container has access to into the new container at the
**exact same paths**. Combined with `-w "${WORKSPACE}"`, the `tbox-builder`
container sees the source tree at the same path Jenkins uses. CMake, GCC, and
Make run inside the Alpine container — never on the host or in Jenkins.

### Stage 3 — Test

Same `--volumes-from` pattern, but runs `ctest` against the `build/` directory
that Stage 2 created inside the workspace. The smoke test:

1. Runs `tbox_diagnostic`
2. Verifies exit code 0
3. Checks the output contains `T-Box Diagnostic Report`

### Post-build

```groovy
post {
    success { echo '✅  Pipeline SUCCEEDED...' }
    failure { echo '❌  Pipeline FAILED...'    }
}
```

Jenkins marks the build green/red and logs the final status regardless of outcome.

---

## Starting and Stopping Jenkins

### Start Jenkins

```powershell
cd c:\DevopsWS\embedded-devops-demo

# Build the custom Jenkins image and start the container (detached)
docker compose up -d --build
```

First start takes ~2 minutes while Jenkins initialises.
Watch progress with:
```powershell
docker compose logs -f jenkins
```
Wait until you see:
```
Jenkins is fully up and running
```

### Open the UI

```
http://localhost:8090
```

### Stop Jenkins (data preserved)

```powershell
docker compose stop
```

### Restart Jenkins

```powershell
docker compose start
```

### Destroy Jenkins (keeps volume / data)

```powershell
docker compose down
```

### Destroy Jenkins + wipe ALL data

```powershell
docker compose down -v   # ⚠️ deletes the tbox-jenkins-data volume
```

---

## First-Time Setup

### 1. Get the initial admin password

```powershell
docker exec tbox-jenkins cat /var/jenkins_home/secrets/initialAdminPassword
```

Copy the printed password.

### 2. Unlock Jenkins

Open `http://localhost:8090`, paste the password, click **Continue**.

### 3. Install suggested plugins

On the "Customize Jenkins" screen, click **Install suggested plugins**.
The essential pipeline plugins are already pre-baked in the image, so this
step mainly installs the remaining defaults (SSH, Email, etc.).

### 4. Create the admin user

Fill in a username, password, and email, then click **Save and Continue**.

### 5. Confirm the Jenkins URL

Leave the default `http://localhost:8090/` and click **Save and Finish**.

---

## Triggering the First Pipeline

### Step 1 — Create a new Pipeline job

1. Click **New Item** on the Jenkins dashboard.
2. Enter a name: `tbox-diagnostic`
3. Select **Pipeline** → click **OK**.

### Step 2 — Configure SCM

1. Under **Pipeline**, set **Definition** to **Pipeline script from SCM**.
2. Set **SCM** to **Git**.
3. Paste your GitHub repository URL into **Repository URL**.
4. If the repo is private, click **Add** → **Jenkins** to create a credential
   (use a GitHub Personal Access Token as the password).
5. Set **Branch Specifier** to `*/main` (or your default branch).
6. Leave **Script Path** as `Jenkinsfile` (default).
7. Click **Save**.

### Step 3 — Run the pipeline

Click **Build Now** on the job page.

Click the build number in **Build History** and open **Console Output** to
watch it run in real time.

On success you will see:
```
✅  Pipeline SUCCEEDED — T-Box build and tests passed.
Finished: SUCCESS
```

---

## Useful Commands

| Task | Command |
|------|---------|
| Start Jenkins | `docker compose up -d --build` |
| Stop Jenkins | `docker compose stop` |
| View logs | `docker compose logs -f jenkins` |
| Get admin password | `docker exec tbox-jenkins cat /var/jenkins_home/secrets/initialAdminPassword` |
| Open shell in Jenkins | `docker exec -it tbox-jenkins bash` |
| Check Docker socket access | `docker exec tbox-jenkins docker ps` |
| List all images | `docker images` |
| Remove tbox-builder image | `docker rmi tbox-builder` |

---

## Troubleshooting

### "permission denied" on Docker socket

Jenkins cannot access `/var/run/docker.sock`. Fix by granting socket access
temporarily (for Docker Desktop — the socket is usually permissive by default):

```powershell
docker exec -u root tbox-jenkins chmod 666 /var/run/docker.sock
```

Then retry the build. If you see this consistently, adjust the `docker` group
GID in `jenkins/Dockerfile` to match your host's GID:

```powershell
# Find your host docker group GID
docker run --rm -v /var/run/docker.sock:/var/run/docker.sock alpine stat /var/run/docker.sock
```

### Port 8080 already in use

Edit `docker-compose.yml` and change `"8080:8080"` to e.g. `"9090:8080"`,
then restart with `docker compose up -d`.

### cmake: command not found inside build stage

The `tbox-builder` image hasn't been built yet. The Build stage builds it
automatically, but if you see this error it means the image build failed.
Run manually to diagnose:
```powershell
docker build -f Dockerfile.build -t tbox-builder .
```
