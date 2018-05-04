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
        /* Launch tests for the agent after builds */
        dir('tests')
	sh 'virtualenv -p /usr/bin/python3 venv'
	sh 'source venv/bin/activate && pip install -r requirements.txt'
	sh 'source venv/bin/activate && pytest --junit-xml=test_results.xml agent_tests || true'
	junit keepLongStdio: true, allowEmptyResults: true, testResults: 'test_results.xml'
     }
}
