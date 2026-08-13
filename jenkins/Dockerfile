# ============================================================
# Jenkins Build Environment for embedded-devops-demo
# ============================================================
# Base  : jenkins/jenkins:lts-jdk17  (official Jenkins LTS)
# Adds  : Docker CLI  — so Jenkins can invoke Docker builds
# Plugins: Git, Pipeline, Docker Pipeline, Stage View, Timestamps
# ============================================================

FROM jenkins/jenkins:lts-jdk17

# --- Install Docker CLI (root required) -------------------------
USER root

RUN apt-get update -qq && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        gnupg && \
    # Add Docker's official GPG key
    install -m 0755 -d /etc/apt/keyrings && \
    curl -fsSL https://download.docker.com/linux/debian/gpg \
        | gpg --dearmor -o /etc/apt/keyrings/docker.gpg && \
    chmod a+r /etc/apt/keyrings/docker.gpg && \
    # Add Docker apt repository
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] \
        https://download.docker.com/linux/debian \
        $(. /etc/os-release && echo "$VERSION_CODENAME") stable" \
        | tee /etc/apt/sources.list.d/docker.list > /dev/null && \
    apt-get update -qq && \
    # Install Docker CLI only (not the daemon — host daemon is reused via socket)
    apt-get install -y --no-install-recommends docker-ce-cli && \
    rm -rf /var/lib/apt/lists/*

# Create the docker group (GID 999 is the common default on Linux hosts)
# and add the jenkins user to it so it can communicate with the Docker socket.
RUN groupadd -f -g 999 docker && \
    usermod -aG docker jenkins

# --- Switch back to the unprivileged jenkins user ---------------
USER jenkins

# Pre-install essential Jenkins plugins so the first-run wizard is fast.
#   git                — SCM checkout from GitHub
#   workflow-aggregator — core Pipeline (Declarative + Scripted)
#   docker-workflow    — docker.build() / docker.image().inside()
#   pipeline-stage-view — stage visualisation in classic UI
#   timestamper        — prefix every log line with a timestamp
#   credentials-binding — inject secrets into pipeline environment
RUN jenkins-plugin-cli --plugins git workflow-aggregator docker-workflow pipeline-stage-view timestamper credentials-binding
