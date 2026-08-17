# JFrog Artifactory Setup — embedded-devops-demo

This document covers the **one-time manual setup** steps required to connect
JFrog Artifactory to the Jenkins CI pipeline.

---

## Architecture Quick Reference

```
Jenkins (tbox-jenkins)
  ├── Checkout stage  — clones repo
  ├── Build stage     — compiles C project inside tbox-builder container
  ├── Test stage      — runs CTest inside tbox-builder container
  └── Publish stage   — runs curlimages/curl with:
        --volumes-from tbox-jenkins   → reads build/tbox_diagnostic
        --network tbox-devops-net     → reaches tbox-artifactory:8082

Artifactory (tbox-artifactory)
  └── Generic Local Repository: tbox-generic-local
        └── tbox_diagnostic/<BUILD_NUMBER>/tbox_diagnostic
```

---

## Step 1 — Start the Stack

```powershell
cd c:\DevopsWS\embedded-devops-demo
docker compose up -d
```

> **Note:** On first run, Jenkins will be recreated (adding `tbox-devops-net`),
> but its named volume (`tbox-jenkins-data`) is preserved — all your jobs,
> credentials, and build history remain intact.
>
> Artifactory takes **60–90 seconds** on its very first boot while it
> initialises the embedded Derby database. Subsequent boots are faster.

Watch Artifactory start:
```powershell
docker compose logs -f artifactory
# Wait for: "Artifactory successfully started"
```

---

## Step 2 — First Login to Artifactory

1. Open **http://localhost:8082** in your browser.
2. Login with the default credentials: `admin` / `password`
3. Follow the welcome wizard:
   - Set a **new admin password** (save it — you will need it)
   - Skip the proxy configuration
   - Skip the repository creation wizard for now (we do it in Step 3)

---

## Step 3 — Create the Generic Local Repository

1. Go to **Administration** → **Repositories** → **Add Repositories** → **Local Repository**
2. Select **Generic** as the Package Type
3. Fill in:
   - **Repository Key**: `tbox-generic-local`
   - **Description**: `T-Box Diagnostic build artifacts`
4. Click **Create Local Repository**

---

## Step 4 — Generate an Access Token

> **Why an Access Token?** JFrog deprecated API keys in Artifactory 7.x.
> Access Tokens are the current standard and support fine-grained scoping.

1. Go to **Administration** → **User Management** → **Access Tokens**
2. Click **Generate Token**
3. Configure:
   - **Token Scope**: User Token (for the `admin` user)
   - **Description**: `jenkins-publish`
   - **Expiry**: Set a reasonable value (e.g., 1 year) or leave as non-expiring for local learning
4. Click **Generate**
5. **Copy the token immediately** — it is shown only once

---

## Step 5 — Add the Token to Jenkins

1. Open **http://localhost:8090** (Jenkins UI)
2. Go to **Manage Jenkins** → **Credentials** → **System** → **Global credentials (unrestricted)**
3. Click **Add Credentials**
4. Fill in:
   - **Kind**: Secret text
   - **Secret**: *(paste the Artifactory Access Token)*
   - **ID**: `artifactory-access-token`
   - **Description**: `JFrog Artifactory token for tbox-generic-local`
5. Click **Create**

> **Security:** The token is stored encrypted in Jenkins' credential store
> (`tbox-jenkins-data` volume). It is never written to Git, Jenkinsfile,
> Dockerfile, or docker-compose.yml.

---

## Step 6 — Run the Pipeline

1. Open the `tbox-diagnostic` job in Jenkins
2. Click **Build Now**
3. Watch the Stage View — you should see 4 stages:
   - ✅ Checkout
   - ✅ Build
   - ✅ Test
   - ✅ Publish *(only on `main` branch — skipped on other branches)*

---

## Step 7 — Verify the Artifact in Artifactory

1. Open **http://localhost:8082**
2. Go to **Artifactory** → **Artifacts** → **tbox-generic-local**
3. Browse to: `tbox_diagnostic/<BUILD_NUMBER>/tbox_diagnostic`
4. You should see the uploaded ELF binary with its checksum

---

## Artifact Path Convention

```
tbox-generic-local/
  tbox_diagnostic/
    1/
      tbox_diagnostic     ← Build #1 binary
    2/
      tbox_diagnostic     ← Build #2 binary
    ...
```

Each successful main-branch build produces one versioned entry.
This provides a full audit trail of every published build.

---

## Useful Commands

| Task | Command |
|------|---------|
| Start everything | `docker compose up -d` |
| Stop everything | `docker compose stop` |
| View Artifactory logs | `docker compose logs -f artifactory` |
| Check Artifactory health | `curl http://localhost:8082/artifactory/api/system/ping` |
| Check from inside Jenkins | `docker exec tbox-jenkins curl -sf http://tbox-artifactory:8082/artifactory/api/system/ping` |
| Restart only Artifactory | `docker compose restart artifactory` |
| Wipe Artifactory data | `docker compose down && docker volume rm tbox-artifactory-data` |

---

## Troubleshooting

### Publish stage fails: "Connection refused" or "Could not resolve host"

The `curlimages/curl` sibling container cannot reach `tbox-artifactory`.

Check:
```powershell
# Is Artifactory healthy?
docker ps --filter "name=tbox-artifactory"

# Can Jenkins reach Artifactory on the named network?
docker exec tbox-jenkins curl -sf http://tbox-artifactory:8082/artifactory/api/system/ping
```

If the `docker exec` succeeds but the pipeline fails, ensure the pipeline
`docker run` command includes `--network tbox-devops-net`.

### Publish stage fails: HTTP 401 Unauthorized

The access token is wrong, expired, or the Jenkins credential ID doesn't match.

Check:
- Jenkins credential ID is exactly: `artifactory-access-token`
- The token has not expired in Artifactory
- The token has write permission to `tbox-generic-local`

Generate a new token in Artifactory if needed and update the Jenkins credential.

### Publish stage is SKIPPED

This is expected on non-`main` branches. The `when` condition checks:
```groovy
env.GIT_BRANCH == 'main'  OR  env.GIT_BRANCH ends with '/main'
```

If you are on `main` and it is still skipped, check what value Jenkins
reports for `GIT_BRANCH` in the Checkout stage output:
```
Branch    : origin/main   ← this is handled (ends with '/main')
Branch    : main          ← this is handled
Branch    : feature/xyz   ← correctly skipped
```

### Artifactory takes too long to start

First boot can take up to 2 minutes. The healthcheck has a 120s `start_period`.
If it still doesn't start:
```powershell
docker compose logs artifactory | Select-Object -Last 50
```
Look for Java errors. Restart if needed:
```powershell
docker compose restart artifactory
```
