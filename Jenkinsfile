/* Pipeline for Grideye Agent */

node {
    try {
     	stage('Checkout') {
	    git url: 'https://github.com/cloudmon360/grideye_agent.git'
	    checkout scm
	}
	
	stage('Configure') {
	    /* `make check` returns non-zero on test failures,
	     * using `true` to allow the Pipeline to continue nonetheless
	     */
	    sh 'make clean'
	    sh './configure'
	}
	
	stage('Make') {
	    sh 'make'
	}
	
	stage('Testing') {
	    /* Launch pytest scripts and create JUnit report */
	    sh 'pytest-3 --junit-xml=test_results.xml tests/*.py || true'
	    junit keepLongStdio: true, allowEmptyResults: true, testResults: 'test_results.xml'
	}
	
	stage("Docker") {
	    /* Build and push Docker image */
	    app = docker.build("cloudmon360/grideye_agent")
	    docker.withRegistry('https://registry.hub.docker.com', 'docker-hub-credentials') {
		app.push("${env.BUILD_NUMBER}")
		app.push("latest")
	    }
	}
    } catch (e) {
	currentBuild.result = "Failed"
	throw e
    } finally {
	notifyBuild(currentBuild.result)
    }
}

def notifyBuild(String buildStatus = 'Started') {
    buildStatus =  buildStatus ?: 'Successful'
    
    def colorName = 'RED'
    def colorCode = '#FF0000'
    def subject = "${buildStatus}: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]'"
    def summary = "${subject} (${env.BUILD_URL})"
    
    if (buildStatus == 'Started') {
	color = 'YELLOW'
	colorCode = '#FFFF00'
    } else if (buildStatus == 'Successful') {
	color = 'GREEN'
	colorCode = '#00FF00'
    } else {
	color = 'RED'
	colorCode = '#FF0000'
    }
    
    slackSend (color: colorCode, message: summary)            
}
