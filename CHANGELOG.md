# Grideye_agent Changelog

* Moved plugin-lib from /usr/local/lib/grideye to /usr/local/lib/grideye/agent
* Added INSTALLFLAGS and set mode to 0644 for plugins
* Added configure --enable-debug flag for gcc -g and remove -S from install
* Added debug of plugins called and their arguments, ie test(1000) when -D
* Added getopt to plugin API
* Added synamic yangmetrics to callhome
* Added error check for http_callhome and http_data
* Hardening after AFL and valgrind runs
* Added curl-timeout (-T) with default value 1min
* For http controller communication, added resiliency for curl couldn't connect, timeout and bad gateway.
	
## 1.3.0 (27 November 2017)

* Grideye_agent can now be compiled and run on Apple Darwin. Thanks Fredrik Pettai.
* Added description in README.md on how to wrap a nagios test.
