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
                    env.VERSION = "${commit_nr}"
                    env.VERSION_PREFIX = prefix
                }
            }
        }

        stage("Build") {
            parallel {
                stage("Dwarf") {
                    steps {
                        sh 'cmake --preset dwarf -DVERSION=$VERSION -DVERSION_PREFIX=$VERSION_PREFIX'
                        sh 'cmake --build --preset dwarf'
                    }
                }

                stage("Modular Bed") {
                    steps {
                        sh 'cmake --preset modularbed -DVERSION=$VERSION -DVERSION_PREFIX=$VERSION_PREFIX'
                        sh 'cmake --build --preset modularbed'
                    }
                }

                stage("XBuddy Extension") {
                    steps {
                        sh 'cmake --preset xbuddy_extension -DVERSION=$VERSION -DVERSION_PREFIX=$VERSION_PREFIX'
                        sh 'cmake --build --preset xbuddy_extension'
                    }
                }

                stage("INDX Head") {
                    steps {
                        sh 'cmake --preset indx_head -DVERSION=$VERSION -DVERSION_PREFIX=$VERSION_PREFIX'
                        sh 'cmake --build --preset indx_head'
                    }
                }
            }
        }
    }

    post {
        always {
            // archive build products
            archiveArtifacts artifacts: 'build/*/bootloader-*', fingerprint: true
        }
        cleanup {
            deleteDir()
        }
    }
}
