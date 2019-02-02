dockerRegistry = "git.ra-doersch.de:5005/"
dockerCredentials = "2d20af83-9ebd-4a5a-b6ee-c77bec430970"
dockerImages = [
    "windows.i686"    : "rttr/cross-compiler/mingw/mingw-w64-docker:master",
    "windows.x86_64"  : "rttr/cross-compiler/mingw/mingw-w64-docker:master",
    "linux.x86_64"    : "rttr/cross-compiler/linux/linux-amd64-docker:master",
    "apple.universal" : "rttr/cross-compiler/apple/apple-docker:master"
]

def transformIntoStep(architecture, dockerImage, buildScript) {
    return {
        timeout(120) {
            def wspwd = pwd()
            dir("build-$architecture") {
                sh 'touch .git-keep'
                withDockerRegistry( registry: [credentialsId: dockerCredentials, url: 'https://'+dockerRegistry ] ) {
                    withDockerContainer(
                        image: dockerRegistry+dockerImage,
                        args: " \
                            -v $HOME/.ccache:/.ccache \
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
                            [$class: 'GitLFSPull'],
                            [$class: 'AuthorInChangelog'],
                            [$class: 'CleanCheckout']
                        ],
                        submoduleCfg: [],
                        userRemoteConfigs: scm.userRemoteConfigs
                    ]

                    sh """
                        git status
                        git submodule foreach git status
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
                expression { params.DEPLOY_TO != "none" }
            }
            steps {
                script {
                    def prepareDeployScript = readTrusted("tools/ci/jenkins/prepare-deploy.sh")
                    prepareDeployScript = prepareDeployScript.replace("%deploy_to%", params.DEPLOY_TO)
                    sh prepareDeployScript

                    def deployScript = readTrusted("tools/ci/jenkins/deploy.sh")
                    deployScript = deployScript.replace("%deploy_to%", params.DEPLOY_TO)
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
