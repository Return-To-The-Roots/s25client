// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

dockerRegistry = "registry.mytrap.de/"
dockerCredentials = "gitlab-Flow86"
dockerImages = [
    "windows.i686"    : "rttr/cross-compiler/mingw/mingw-w64-docker:master",
    "windows.x86_64"  : "rttr/cross-compiler/mingw/mingw-w64-docker:master",
    "linux.x86_64"    : "rttr/cross-compiler/linux/linux-amd64-docker:master",
    "apple.x86_64"    : "rttr/cross-compiler/apple/apple-docker:master"
]

def transformIntoStep(architecture, dockerImage, buildScript) {
    return {
        timeout(120) {
            def wspwd = pwd()
            def ccache_dir_host = "$HOME/.ccache-$architecture"
            dir("build-$architecture") {
                sh 'touch .git-keep'
                sh "mkdir -p '$ccache_dir_host'"
                withDockerRegistry( registry: [credentialsId: dockerCredentials, url: 'https://'+dockerRegistry ] ) {
                    withDockerContainer(
                        image: dockerRegistry+dockerImage,
                        args: " \
                            -v $ccache_dir_host:/.ccache \
                            -v $wspwd/source:/source:ro \
                            -v $wspwd/result:/result \
                        ") {
                        def script = buildScript.replace("%architecture%", architecture)
                        sh script
                    }
                }
            }
        }
    }
}

pipeline {
    agent {
        label "docker"
    }

    parameters {
        choice(name: 'DEPLOY_TO', choices: [ "none", "nightly", "stable"], description: 'deploy to this after build (none = disable deployment)')
        booleanParam(name: 'FORCE_DEPLOY', defaultValue: false, description: 'deploy even if nothing has been changed (false = only deploy on changes)')
    }

    options {
        buildDiscarder(logRotator(artifactDaysToKeepStr: '', artifactNumToKeepStr: '14', daysToKeepStr: '14', numToKeepStr: '180'))
        disableConcurrentBuilds()
        skipDefaultCheckout(true)
    }

    environment {
        DEBIAN_FRONTEND = 'noninteractive'
    }

    stages {
        stage('checkout') {
            steps {
                dir('source') {
                    checkout scm: [
                        $class: 'GitSCM',
                        branches: scm.branches,
                        doGenerateSubmoduleConfigurations: false,
                        extensions: scm.extensions + [
                            [$class: 'SubmoduleOption', disableSubmodules: false, parentCredentials: true, recursiveSubmodules: true, reference: '', trackingSubmodules: false],
                            [$class: 'AuthorInChangelog'],
                            [$class: 'CleanCheckout']
                        ],
                        submoduleCfg: [],
                        userRemoteConfigs: scm.userRemoteConfigs
                    ]

                    sh """
                        git status
                        git submodule foreach git status
                        git restore-mtime -c || echo "Unable to restore file modification time" >&2
                        git submodule foreach "git restore-mtime -c || echo "Unable to restore file modification time" >&2"
                    """
                }
                sh 'rm -rf result'
                dir('result') {
                    sh 'touch .git-keep'
                }
            }
        }

        stage('changelog') {
            steps {
                script {
                    def changelogScript = readTrusted("tools/ci/jenkins/changelog.sh")
                    sh changelogScript
                }
            }
        }

        stage('build') {
            steps {
                script {
                    def parallel_map = [:]
                    def buildScript = readTrusted("tools/ci/jenkins/build.sh")
                    buildScript = buildScript.replace("%deploy_to%", params.DEPLOY_TO)
                    buildScript = buildScript.replace("%force_deploy%", params.FORCE_DEPLOY.toString())

                    dockerImages.each { architecture, image ->
                        echo "Adding Job ${architecture} (${image})"
                        parallel_map["${architecture}"] = transformIntoStep(architecture, image, buildScript)
                    }

                    // todo: mirror launchpad

                    parallel_map.failFast = true
                    parallel parallel_map
                }
            }
        }

        stage('deploy') {
            when {
                expression { return params.DEPLOY_TO != "none" }
            }
            steps {
                script {
                    def prepareDeployScript = readTrusted("tools/ci/jenkins/prepare-deploy.sh")
                    prepareDeployScript = prepareDeployScript.replace("%deploy_to%", params.DEPLOY_TO)
                    prepareDeployScript = prepareDeployScript.replace("%force_deploy%", params.FORCE_DEPLOY.toString())
                    sh prepareDeployScript

                    def deployScript = readTrusted("tools/ci/jenkins/deploy.sh")
                    deployScript = deployScript.replace("%deploy_to%", params.DEPLOY_TO)
                    deployScript = deployScript.replace("%force_deploy%", params.FORCE_DEPLOY.toString())
                    sh deployScript
                }
            }
        }
    }
    post {
        success {
            dir('result') {
                archiveArtifacts artifacts: '*.tar.bz2,*.zip,*.txt', fingerprint: true
            }
        }
        failure {
            mail to: 'sf-team@siedler25.org',
                subject: "Failed Pipeline: ${currentBuild.fullDisplayName}",
                body: "Pipeline failed: ${env.BUILD_URL}"
        }
        cleanup {
            sh 'rm -rf result'
        }
    }
}
