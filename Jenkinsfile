// ============================================================
// Jenkinsfile — T-Box Diagnostic CI Pipeline
// ============================================================
//
// Stages
// ------
//   Checkout  — Clone / update source from GitHub via Jenkins SCM config
//   Build     — Build Dockerfile.build image, compile C project inside it
//   Test      — Run CTest smoke test inside the same build image
//   Publish   — Upload build/tbox_diagnostic to JFrog Artifactory (main branch only)
//
// How Docker access works
// -----------------------
// Jenkins runs inside the 'tbox-jenkins' container. Its workspace lives
// in the named volume 'tbox-jenkins-data' mounted at /var/jenkins_home.
//
// When a stage needs build tools, we run a sibling container (tbox-builder)
// with '--volumes-from tbox-jenkins'. This mounts every volume that Jenkins
// has access to — including the workspace — into the build container at the
// SAME paths. Combined with '-w "${WORKSPACE}"', the build tools see the
// source tree exactly where cmake expects it.
//
// Result: GCC, CMake, and Make are never installed on the host or in Jenkins;
//         they only exist inside the tbox-builder Alpine image.
// ============================================================

pipeline {
    agent any

    options {
        timestamps()                        // Prefix every log line with HH:mm:ss
        timeout(time: 15, unit: 'MINUTES')  // Kill runaway builds after 15 min
        disableConcurrentBuilds()           // Prevent parallel runs of the same job
        buildDiscarder(logRotator(numToKeepStr: '10'))  // Keep last 10 builds
    }

    // ---- Pipeline-wide variables --------------------------------
    // Non-sensitive config only. Credentials are NEVER stored here;
    // the access token is injected at runtime by withCredentials().
    environment {
        ARTI_URL  = 'http://tbox-artifactory:8082/artifactory'
        ARTI_REPO = 'tbox-generic-local'
    }

    // ---- Stages -----------------------------------------------
    stages {

        // ----------------------------------------------------------
        stage('Checkout') {
        // ----------------------------------------------------------
        // Jenkins clones (or updates) the configured SCM repository
        // into the workspace. On first run this is a full clone;
        // subsequent runs do an incremental fetch.
        // ----------------------------------------------------------
            steps {
                echo '=== Checkout ==='
                checkout scm
                echo "Workspace : ${WORKSPACE}"
                echo "Branch    : ${env.GIT_BRANCH ?: 'unknown'}"
                echo "Commit    : ${env.GIT_COMMIT ?: 'unknown'}"
                sh 'ls -la'
            }
        }

        // ----------------------------------------------------------
        stage('Build') {
        // ----------------------------------------------------------
        // 1. Build (or update) the tbox-builder Docker image from
        //    Dockerfile.build that lives in the repo.
        // 2. Run cmake configure + compile INSIDE that image, sharing
        //    the Jenkins workspace via --volumes-from.
        // ----------------------------------------------------------
            steps {
                echo '=== Build ==='

                // Step 1: Build the Docker build environment image.
                // Docker caches layers, so subsequent runs are fast.
                sh 'docker build -f Dockerfile.build -t tbox-builder .'

                // Step 2: Compile the C project inside tbox-builder.
                // --volumes-from tbox-jenkins  → shares the jenkins_home volume
                //                               (which contains the workspace)
                // -w "${WORKSPACE}"             → sets working dir to the repo root
                sh """
                    docker run --rm \
                        --volumes-from tbox-jenkins \
                        -w "${WORKSPACE}" \
                        tbox-builder \
                        sh -c "cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
                               cmake --build build --parallel \$(nproc)"
                """
            }
        }

        // ----------------------------------------------------------
        stage('Test') {
        // ----------------------------------------------------------
        // Run the CTest smoke test suite inside tbox-builder.
        // The test verifies:
        //   - The binary exits with code 0
        //   - The output contains 'T-Box Diagnostic Report'
        // ----------------------------------------------------------
            steps {
                echo '=== Test ==='
                sh """
                    docker run --rm \
                        --volumes-from tbox-jenkins \
                        -w "${WORKSPACE}" \
                        tbox-builder \
                        ctest --test-dir build --output-on-failure -V
                """
            }
        }

        // ----------------------------------------------------------
        stage('Publish') {
        // ----------------------------------------------------------
        // Upload the compiled Linux ELF binary to JFrog Artifactory.
        //
        // Conditions
        // ----------
        //   • Runs ONLY on the 'main' branch — skipped on all others
        //   • Only reached when Build AND Test both succeed
        //   • Build is marked FAILURE if the upload fails (-f flag)
        //
        // How it works
        // ------------
        //   A curlimages/curl sibling container runs the upload so
        //   curl is never installed on the host, Jenkins, or tbox-builder.
        //
        //   --volumes-from tbox-jenkins  → shares the jenkins_home volume,
        //                                  giving access to build/tbox_diagnostic
        //   --network tbox-devops-net    → lets the container resolve
        //                                  'tbox-artifactory' by container name
        //
        //   The access token is injected only at runtime via:
        //     Manage Jenkins → Credentials → Secret Text
        //     ID: artifactory-access-token
        //   It is NEVER committed to Git, Jenkinsfile, Dockerfile, or Compose.
        // ----------------------------------------------------------
            when {
                // Works for both standard Pipeline (GIT_BRANCH) and
                // Multibranch Pipeline (BRANCH_NAME). Handles the
                // 'origin/main' prefix the git plugin sometimes adds.
                expression {
                    def b = env.GIT_BRANCH ?: env.BRANCH_NAME ?: ''
                    return b == 'main' || b.endsWith('/main')
                }
            }
            steps {
                echo '=== Publish ==='
                withCredentials([string(
                    credentialsId: 'artifactory-access-token',
                    variable: 'JFROG_TOKEN'
                )]) {
                    sh """
                        docker run --rm \\
                            --volumes-from tbox-jenkins \\
                            --network tbox-devops-net \\
                            -w "${WORKSPACE}" \\
                            curlimages/curl:latest \\
                            -f -s -S \\
                            -H "Authorization: Bearer \${JFROG_TOKEN}" \\
                            -T build/tbox_diagnostic \\
                            "\${ARTI_URL}/\${ARTI_REPO}/tbox_diagnostic/${BUILD_NUMBER}/tbox_diagnostic"
                    """
                }
                echo "\u2705 Published \u2192 \${ARTI_URL}/\${ARTI_REPO}/tbox_diagnostic/${BUILD_NUMBER}/tbox_diagnostic"
            }
        }

    } // end stages

    // ---- Post-build actions ------------------------------------
    post {
        success {
            echo '✅  Pipeline SUCCEEDED — T-Box build and tests passed.'
        }
        failure {
            echo '❌  Pipeline FAILED — review the stage logs above.'
        }
        always {
            echo "Pipeline complete. Result: ${currentBuild.currentResult}"
        }
    }
}
