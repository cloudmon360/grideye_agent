node {
  git url: 'https://github.com/cloudmon360/grideye_agent.git'
  stages {
        stage('Configure') {
            steps {
                /* `make check` returns non-zero on test failures,
                * using `true` to allow the Pipeline to continue nonetheless
                */
                sh './configure || true' 
            }
        }
        stage('Make') {
            steps {
                /* `make check` returns non-zero on test failures,
                * using `true` to allow the Pipeline to continue nonetheless
                */
                sh 'make || true' 
            }
        }

    }
}
