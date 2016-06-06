#!/bin/groovy
String[] archs = ["windows.i386", "windows.x86_64", "linux.i386", "linux.x86_64", "apple.universal" ]

def compile_map = [:]

node('docker') {
    stage "Checkout"
    checkout scm
    sh "env"
}

compile_map = [:]

for (int i = 0 ; i < archs.size(); ++i) {
    def x = archs.get(i)
    compile_map["${x}"] = { 
        node('docker') {
            deleteDir()
            unstash 'source'
            dir('build') {
                sh """BARCH=--arch=c.${x}
                      if [ "\$(uname -s | tr "[:upper:]" "[:lower:]").\$(uname -m)" = "${x}" ] ; then
                          BARCH=
                      fi
                      ./cmake.sh --prefix=. \$BARCH -DRTTR_USE_STATIC_BOOST=ON
                   """
                archive 's25rttr*.tar.bz2,s25rttr*.zip'
            }
        } 
    }
}

parallel compile_map
