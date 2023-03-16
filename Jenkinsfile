pipeline {
    agent {
        dockerfile true
    }

    stages {
        stage("Prepare") {
            steps {
                sh 'git submodule update --init'

                script {
                    def commit_nr = sh(script: 'git rev-list HEAD --count', returnStdout: true).trim()
                    def prefix
                    if (env.CHANGE_ID) {
                        // This is a PR build
                        prefix = "PR${env.CHANGE_ID}-"
                    } else if (env.BRANCH_NAME == "master") {
                        prefix = ""
                    } else {
                        // This is build of an ordinary branch (not a release branch)
                        prefix = env.BRANCH_NAME.replaceAll("_", "-")
                    }
                    env.BL_VERSION = "${commit_nr}"
                    env.BL_VERSION_PREFIX = prefix
                }
            }
        }

        stage("Build libopencm3") {
            steps {
                sh 'make -C libopencm3 lib/stm32/g0'
            }
        }

        stage("Build Dwarf") {
            steps {
                sh "make dwarf"
            }
        }

        stage("Build Modular Bed") {
            steps {
                sh "make modularbed"
            }
        }
    }

    post {
        always {
            // archive build products
            archiveArtifacts artifacts: 'bootloader-*', fingerprint: true
        }
        cleanup {
            deleteDir()
        }
    }
}
