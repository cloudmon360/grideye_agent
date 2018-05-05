# Grideye_agent Changelog

* Hardening after AFL and valgrind runs
* Added curl-timeout (-T) with default value 1min
* For http controller communication, added resiliency for curl couldn't connect, timeout and bad gateway.
	
## 1.3.0 (27 November 2017)

* Grideye_agent can now be compiled and run on Apple Darwin. Thanks Fredrik Pettai.
* Added description in README.md on how to wrap a nagios test.
