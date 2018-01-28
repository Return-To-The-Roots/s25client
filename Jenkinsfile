#!/bin/groovy

///////////////////////////////////////////////////////////////////////////////

def transformIntoStep(arch, wspwd) {
    return {
        timeout(120) {
            node('master') {
                ansiColor('xterm') {
                    ws(wspwd + "/ws/" + arch) {
                        echo "Build ${arch} in " + pwd()

                        if( isUnix() ) {
                            sh 'chmod -R u+w .git || true' // fixes unstash overwrite bug ... #JENKINS-33126
                        }

                        unstash 'source'

                        sh """set -x
                              git clean -f
                              git submodule foreach git clean -fxd
                              echo "Git status for main and sub repos:"
                              git status
                              git submodule foreach git status
                              BARCH=--arch=c.${arch}
                              if [ "\$(uname -s | tr "[:upper:]" "[:lower:]").\$(uname -m)" = "${arch}" ] ; then
                                  BARCH=
                              fi
                              PARAMS=
                              VOLUMES="-v /srv/apache2/siedler25.org/nightly:/www \
                                  -v /srv/backup/www/s25client:/archive \
                                  "
                              COMMANDS=

                              if [[ "${env.BRANCH_NAME}" == PR-* ]] ; then
                                  VOLUMES=""
                              elif [ "${env.BRANCH_NAME}" == "master" ] ; then
                                  PARAMS=create_nightly
                              elif [ "${env.BRANCH_NAME}" == "stable" ] ; then
                                  PARAMS=create_stable
                                  COMMANDS='&& rm -f build_version_defines.h.force && make updateversion && sed -i -e "s/WINDOW_VERSION \\\"[0-9]*\\\"/WINDOW_VERSION \\\"\$(cat ../.stable-version)\\\"/g" build_version_defines.h && touch build_version_defines.h.force && cat build_version_defines.h'
                              fi
                              BUILD_CMD="cd build && ./cmake.sh --prefix=. \$BARCH -DRTTR_ENABLE_WERROR=ON -DRTTR_USE_STATIC_BOOST=ON \$COMMANDS && make \$PARAMS"
                              echo "Executing: \$BUILD_CMD"
                              docker run --rm -u jenkins -v \$(pwd):/workdir \
                                                         -v ~/.ssh:/home/jenkins/.ssh \
                                                         -v ~/.ccache:/workdir/.ccache \
                                                         \$VOLUMES \
                                                         --name "${env.BUILD_TAG}-${arch}" \
                                                         git.ra-doersch.de:5005/rttr/docker-precise:master -c \
                                                         "\$BUILD_CMD"
                              EXIT=\$?
                              echo "Exiting with error code \$EXIT"
                              exit \$EXIT
                        """

                        archiveArtifacts artifacts: 's25rttr*.tar.bz2,s25rttr*.zip', fingerprint: true, onlyIfSuccessful: true
                    }
                }
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////

catchError() {

    properties([
        buildDiscarder(logRotator(artifactDaysToKeepStr: '', artifactNumToKeepStr: '14', daysToKeepStr: '14', numToKeepStr: '180')),
        disableConcurrentBuilds()
    ])

    milestone label: 'Start'

    def wspwd = "";

    stage("Checkout") {
        node('master') {
            ansiColor('xterm') {
                checkout scm

                sh """set -x
                      git reset --hard
                      git submodule foreach git clean -fxd
                      git submodule foreach git reset --hard
                      git submodule update --init --force --checkout
                   """

                stash includes: '**, .git/', excludes: 'ws/**', name: 'source', useDefaultExcludes: false

                wspwd = pwd()
            }
        }
    }

    milestone label: 'Checkout complete'

    stage("Building") {
        String[] archs = ["windows.i386", "windows.x86_64", "linux.i386", "linux.x86_64", "apple.universal" ]
        def parallel_map = [:]

        for(int i = 0; i < archs.size(); i++) {
            def arch = archs[i]
            echo "Adding Job ${arch}"
            parallel_map["${arch}"] = transformIntoStep(arch, wspwd)
        }

        // mirror to launchpad step
        parallel_map["mirror"] = {
            node('master') {
                ansiColor('xterm') {
                    sh """set -x
                          mkdir -p ws
                          pushd ws
                          if [ ! -d s25client.git ] ; then
                              git clone --mirror https://github.com/Return-To-The-Roots/s25client.git
                              (cd s25client.git && git remote set-url --push origin git+ssh://git.launchpad.net/s25rttr)
                          fi
                          cd s25client.git
                          git fetch -p origin
                          git push --mirror
                    """
                    // todo: mirror submodules?
                }
            }
        }

        /*
        parallel_map["upload-ppa"] = {
            node('master') {
                ansiColor('xterm') {
                    ws(wspwd+"/ws/upload-ppa") {
                        echo "Upload to PPA in "+pwd()
                        sh 'chmod -R u+w .git || true' // fixes unstash overwrite bug ... #JENKINS-33126
                        unstash 'source'
                        sh """set -x
                              if [ "${env.BRANCH_NAME}" == "master" ] || [ "${env.BRANCH_NAME}" == "latest" ] ; then
                                    cd release
                                    ./build_deb.sh ${env.BUILD_NUMBER} || exit 1
                              fi
                        """
                    }
                }
            }
        }
        */

        parallel_map.failFast = true
        parallel parallel_map
    }


    milestone label: 'Build complete'

    stage("Publishing") {
        node('master') {
            ansiColor('xterm') {
                sh """set -x
                      if [ "${env.BRANCH_NAME}" == "stable" ] ; then
                          git tag -a "\$(cat .stable-version)-$BUILD_NUMBER" -m "Created release \$(cat .stable-version) from Jenkins build $BUILD_NUMBER"
                          git push git@github.com:Return-To-The-Roots/s25client.git --tags
                      fi

                      alias ssh="ssh -o ForwardX11=no"
                      cd release
                      ./upload_urls.sh nightly
                      ./upload_urls.sh stable
                """

                archiveArtifacts artifacts: 'release/changelog-*.txt,release/rapidshare-*.txt', fingerprint: true, onlyIfSuccessful: true
            }
        }
    }

    milestone label: 'Publishing complete'

} // catchError()

node {
    step([$class: 'Mailer',
          notifyEveryUnstableBuild: true,
          recipients: "sf-team@siedler25.org",
          sendToIndividuals: true
    ])
}
