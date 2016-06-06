#!/bin/groovy

properties([[$class: 'jenkins.model.BuildDiscarderProperty',
            strategy: [$class: 'LogRotator',
                        daysToKeepStr: '10',
                        numToKeepStr: '100',
                        artifactNumToKeepStr: '10']]])

String[] archs = ["windows.i386", "windows.x86_64", "linux.i386", "linux.x86_64", "apple.universal" ]

def compile_map = [:]

node('master') {
    stage "Checkout"
    checkout scm
    sh """set -x
          git submodule foreach "git reset --hard || true" || true
          git reset --hard || true
          git submodule update --init || true
       """

    stash includes: '**, .git/', name: 'source', useDefaultExcludes: false
    
    sh "env"
}

compile_map = [:]

for (int i = 0 ; i < archs.size(); ++i) {
    def x = archs.get(i)
    compile_map["${x}"] = { 
        node('master') {
            ws(pwd()+"/"+x) {
                echo "Build ${x} in "+pwd()+"/"+x
                unstash 'source'
                sh """set -x
                      BARCH=--arch=c.${x}
                      if [ "\$(uname -s | tr "[:upper:]" "[:lower:]").\$(uname -m)" = "${x}" ] ; then
                          BARCH=
                      fi
                      PARAMS=
                      # if [[ "${env.BRANCH_NAME}" == "PR-*" ]] ; then
                      if [ "${env.BRANCH_NAME}" == "master" ] ; then
                          PARAMS=create_nightly
                      #elif [ "${env.BRANCH_NAME}" == "latest" ] ; then
                      #    PARAMS=create_release
                      fi
                      docker run --rm -u jenkins -v \$(pwd):/workdir \
                                                 -v /srv/apache2/siedler25.org/nightly:/www \
                                                 -v /srv/backup/www/s25client:/archive \
                                                 --name "${env.BUILD_TAG}-${x}" \
                                                 ubuntu/crossbuild:precise -c \
                                                 "cd build && ./cmake.sh --prefix=. \$BARCH -DRTTR_USE_STATIC_BOOST=ON && make \$PARAMS"
                   """
                archive 's25rttr*.tar.bz2,s25rttr*.zip'
            }
        } 
    }
}

stage "Building"
parallel compile_map

stage "Publishing"
// todo
