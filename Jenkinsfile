pipeline {
    agent {
        dockerfile true
    }

    stages {
        stage("Prepare") {
            steps {
                sh 'git submodule update --init'
            }
        }

        stage("Build") {
            parallel {
                stage("Dwarf") {
                    steps {
                        sh 'cmake --preset dwarf'
                        sh 'cmake --build --preset dwarf'
                    }
                }

                stage("Modular Bed") {
                    steps {
                        sh 'cmake --preset modularbed'
                        sh 'cmake --build --preset modularbed'
                    }
                }

                stage("XBuddy Extension") {
                    steps {
                        sh 'cmake --preset xbuddy_extension'
                        sh 'cmake --build --preset xbuddy_extension'
                    }
                }

                stage("INDX Head") {
                    steps {
                        sh 'cmake --preset indx_head'
                        sh 'cmake --build --preset indx_head'
                    }
                }

                stage("Baseboard") {
                    steps {
                        sh 'cmake --preset baseboard'
                        sh 'cmake --build --preset baseboard'
                    }
                }

                stage("Smartled01") {
                    steps {
                        sh 'cmake --preset smartled01'
                        sh 'cmake --build --preset smartled01'
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
