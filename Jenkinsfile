// ============================================================
// Jenkinsfile — T-Box Diagnostic CI Pipeline
// ============================================================
//
// Stages
// ------
//   Checkout  — Clone / update source from GitHub via Jenkins SCM config
//   Build     — Build Dockerfile.build image, compile C project inside it
//   Test      — Run CTest smoke test inside the same build image
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
