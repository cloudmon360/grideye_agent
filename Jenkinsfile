node {
     stage('Checkout') {
        git url: 'https://github.com/cloudmon360/grideye_agent.git'
        checkout scm
     }

     stage('Configure') {
        /* `make check` returns non-zero on test failures,
        * using `true` to allow the Pipeline to continue nonetheless
        */
        sh 'make clean || true'
        sh './configure || true'
     }

     stage('Make') {
        /* `make check` returns non-zero on test failures,
        * using `true` to allow the Pipeline to continue nonetheless
        */
        sh 'make || true'
     }

     stage('Testing') {
	sh 'pytest-3 --junit-xml=test_results.xml tests/*.py || true'
	junit keepLongStdio: true, allowEmptyResults: true, testResults: 'test_results.xml'
     }

     stage("Docker") {
          app = docker.build("cloudmon360/grideye_agent")
          docker.withRegistry('https://registry.hub.docker.com', 'docker-hub-credentials') {
               app.push("${env.BUILD_NUMBER}")
               app.push("latest")
          }
     }
}
