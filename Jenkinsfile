@Library('xmos_jenkins_shared_library@v0.37.0')

def runningOn(machine) {
  println "Stage running on:"
  println machine
}

def buildPyWheel() {
    checkout scm
    withTools(params.TOOLS_VERSION) {
        createVenv("requirements.txt")
        withVenv {
            sh "pip install build"
            sh "python -m build --wheel"
            archiveArtifacts artifacts: "dist/*.whl", allowEmptyArchive: true, fingerprint: true
        }
    }
}

def testPyWheel() {
    checkout scm
    withTools(params.TOOLS_VERSION) {
        createVenv("requirements.txt")
        withVenv {
            sh "pip install --find-links=dist xscope_fileio --force-reinstall"
            sh "pip install cmake ninja"
            sh "cmake -G Ninja -B build -S tests/simple"
            sh "cmake --build build"
            sh "python tests/test_simple.py"
        }
    }
}

getApproval()

pipeline {
  agent none
  parameters {
    string(
      name: 'TOOLS_VERSION',
      defaultValue: '15.3.0',
      description: 'The tools version to build with (check /projects/tools/ReleasesTools/)'
    )
  } // parameters
  environment {
    REPO_NAME = 'xscope_fileio' //TODO remove this after Jenkins Shared Library Update
  } // environment
  options {
    skipDefaultCheckout()
    timestamps()
    buildDiscarder(xmosDiscardBuildSettings(onlyArtifacts=false))
  } // options
  stages {
    stage('xcore.ai') {
      agent {
        label 'xcore.ai' // xcore.ai nodes have 2 devices atatched, allowing parallel HW test
      }

      stages {

        stage('Checkout') {
          steps {
            runningOn(env.NODE_NAME)
            dir('xscope_fileio') {
                checkout scm
                sh "git clone git@github0.xmos.com:xmos-int/xtagctl.git"
            } // dir
          } // steps
        } // stage 'Checkout'

        stage('Install Dependencies') {
          steps {
            dir('xscope_fileio') {
              withTools(params.TOOLS_VERSION) {
                createVenv("requirements.txt")
                withVenv {
                  sh "pip install -e xtagctl/"
                  sh "pip install -r requirements.txt"
                 }
              }
            }
          }
        }
        stage('Static analysis') {
          steps {
            dir('xscope_fileio') {
              withVenv {
                warnError("Flake") {
                  sh "flake8 --exit-zero --output-file=flake8.xml xscope_fileio"
                  recordIssues enabledForFailure: true, tool: flake8(pattern: 'flake8.xml')
                }
              }
            }
          }
        }

        stage('Build examples') {
              steps {
                dir("xscope_fileio/examples") {
                  xcoreBuild()
                } // dir
              } // steps
            }  // Build examples
        
          stage('Build tests') {
              steps {
                dir("xscope_fileio/tests") {
                  xcoreBuild()
                } // dir
              } // steps
            }  // Build examples

        stage('Cleanup xtagctl'){
          steps {
            dir('xscope_fileio') {
              withVenv {
                withTools(params.TOOLS_VERSION) {
                  sh 'rm -f ~/.xtag/status.lock ~/.xtag/acquired'
                  sh 'xtagctl reset_all XCORE-AI-EXPLORER'
                }
              }
            }
          }
        }
        
        stage('Tests') {
          steps { 
            dir('xscope_fileio/tests') {
              withVenv {
                withTools(params.TOOLS_VERSION) {
                  // sh 'pytest' // info: configuration opts in pytest.ini
                } // withTools
              } // withVenv
            } // dir
          } // steps
        } // Tests

      } // stages
      post {
        //always {
          // junit '**/reports/*.xml'
        //}
        cleanup {
          xcoreCleanSandbox()
        }
      }
    } // stage: xcore.ai

    stage('Windows wheel build') {
      agent {label 'x86_64&&windows'}
      steps {buildPyWheel() ; testPyWheel()}
      post {cleanup {xcoreCleanSandbox()}}
    } // stage: Windows build

    stage('Mac_x64 wheel build') {
      agent {label 'x86_64&&macOS'}
      steps {buildPyWheel() ; testPyWheel()}
      post {cleanup {xcoreCleanSandbox()}}
    } // stage: Mac_x64 build

    stage('Mac_arm64 wheel build') {
      agent {label 'arm64&&macos'}
      steps {buildPyWheel() ; testPyWheel()}
      post {cleanup {xcoreCleanSandbox()}}
    } // stage: Mac_arm64 build

    stage('Linux_x64 build') {
      agent {label 'x86_64 && linux'}
      steps {buildPyWheel() ; testPyWheel()}
      post {cleanup {xcoreCleanSandbox()}}
    } // stage: Linux_x64 build

    stage('Docs') {
      agent {
        label 'documentation'
      }
      steps {
        runningOn(env.NODE_NAME)
        dir('xscope_fileio') {
          checkout scm
          createVenv("requirements.txt")
          withTools(params.TOOLS_VERSION) {
            buildDocs(archiveZipOnly: true)
          }
        }
      }
      post {
        cleanup {
          cleanWs()
        }
      }
    } // stage: Docs

  } // stages
} // pipeline
