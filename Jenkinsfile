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
}
