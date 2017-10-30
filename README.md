# Grideye_agent
Cloud monitoring agent. 

## Table of contents

  * [Starting](#1-starting)
  * [Installation](#2-installation)
  * [Plugins](#3-plugins)
  * [Licenses](#4-licenses)
  * [Contact](#5-contact)

Grideye_agent is part of grideye cloud monitoring software. Grideye consists of a controller, a dashboard, and agents. This is the agent part.  

Some documentation about grideye architecture can be found at
     http://grideye.nordu.net/arch

The grideye agent can be run in a process, in a VM or in a
container. It communicates with a grideye controller. 

## 1. Starting

When started, a grideye_agent typically makes a 'callhome' to an existing
grideye controller.

This is an example of how to start grideye_agent:

    ./grideye_agent -F -u <url> -I <id> -N <name>

where
- 'url' is the URL of the grideye server / controller
- 'id' is the UUID of the user as gievn in the controller.
- 'name' is the name of the agent as it will appear in grideye plots.

As an alternative grideye_agent can be run as a docker container.

## 2. Installation

A typical installation is as follows:

    > configure	       	        # Configure grideye_agent
    > make                      # Compile
    > sudo make install         # Install agent and plugins

The source builds one main program: grideye_agent. An
example startup-script is available in util/grideye_agent.

Grideye_agent requires [CLIgen](http://www.cligen.se) and [CLIXON](http://www.clicon.org) for building. To build and install CLIgen and CLIXON:

    git clone https://github.com/olofhagsand/cligen.git
    cd cligen; configure; make; sudo make install
    git clone https://github.com/clicon/clixon.git
       cd clixon; 
       configure --without-restconf --without-keyvalue; 
       make; 
       sudo make install; 
       sudo make install-include

## 3. Plugins

Grideye_agent contains an open plugin interface.  Several plugins
are included in this release. Other authors have (and can) contribute
to the plugins. 

The following steps illustrate how to add the Nagios (*check_http*)[https://www.monitoring-plugins.org/doc/man/check_http.html]
test plugin and how to incorporate it to produce input to
grideye. 
There are lots of Nagios plugins making it an easy way to extend Grideye.

### 3.1 The API

A grideye plugin is a dynamically loaded plugin written in C. You
write a file with a couple of functions, compile it, place it in a
directory, and restart grideye_agent.

A grideye plugin has a pre-defined function *grideye_plugin_init_v1()*
which returns a table containing API functions.

The API table is as follows:

Symbol | Type | Mandatory |Description
--- | --- | --- | ---
gp_version | Variable | Yes | Must be 1
gp_magic | Variable | Yes | Must be 0x3f687f03
gp_exit_fn | Function | No | An exit function
gp_test_fn | Function | Yes | The actual test function with input and output parameters.
gp_file_fn | Function | No | Predefined files and devices may be used in tests.
gp_input | Variable | Yes | Name of input parameter requested
gp_output | Variable | Yes | Format of output. Only "xml" supported

### 3.2 Identifying the input: parameters

check_http has lots of parameters. You can hardcode most, or leave as
defaults.

```
   check_http -H www.youtube.com -S
   HTTP OK: HTTP/1.1 200 OK - 495051 bytes in 1.751 second response time |
   time=1.751042s;;;0.000000 size=495051B;;;0
```

In this example, the *host* parameter as chosen as dynamically
configurable, which means it can dynamically change in a Grideye testcase.

This means 'gp_input=host' is defined in the plugin and gp_test_fn is called with 'host' as input parameter, example:

```
   gp_test_fn("www.youtube.com", ...)
```

### 3.3 Identifying the output: metrics

check_http has several outputs, such as the status (200 OK), size and
latency. The test function is written so that it outputs an XML string
such as:

```
   <htime>1751</htime><hsize>495051</hsize><hstatus>200 OK</hstatus>
```

### 3.4 Defining metrics in YANG

The three metrics: htime, hsize, hstatus need to be defined in the
Grideye YANG model, so that the Grideye controller can interpret them.
These entries are added to the grideye YANG model as follows:

```
   module grideye-result {
      ...
      grouping metrics{
         ...
         leaf hstatus{
            description "HTTP status code";
            type string;
	 }
         leaf htime{
            description "HTTP request time";
            type int32;
            units ms;
         }
         leaf hsize{
            description "HTTP response size";
            type int32;
            units bytes;
         }
      }
   }
```

Note that hstatus is string, and therefore cannot be plotted, but can
be used as event (if changed). Note also that htime has been defined
in milliseconds to avoid floating point numbers. Floats can be used but are more complex to handle.

If the YANG model is changed, the grideye *controller* is restarted.

Note that there already exists a large number of metrics and you can
most likely use already existing metrics.

### 3.5 Writing the test function

The test function is straightforward: Spawn the Nagios plugin and 
parse the data. In C, this is a little painful, but with help of a
help function, this is (a simplified way) to write it. The full code
can be found in (plugins/grideye_http.c):

```
   int
   http_test(int        host,
             char     **outstr)
   {
      if (http_fork(_PROGRAM, "-H", "www.youtube.com", "-S", buf, buflen) < 0)
         goto done;
      sscanf(buf, "%*s %*s %*s %s %s %*s %d %*s %*s %lf\n",
            code0, code1, &size, &time);
      if ((slen = snprintf(*outstr, slen,
                         "<hstatus>\"%s %s\"</hstatus>"
                         "<htime>%d</htime>"
                         "<hsize>%d</hsize>",
                         code0, code1,
                         (int)(time*1000),
                         size)) <= 0)
          goto done;
      return 0; 
   }
```
   
### 3.6 Local test run

Once the plugin code has been written, you can run the test as
stand-alone. This is useful for verifying, validating and debugging:

```
   gcc -o grideye_http grideye_http.c
   ./grideye_http 
   <hstatus>"200 OK"</hstatus><htime>1430</htime><hsize>491172</hsize>
```

### 3.7 Full grideye run

The grideye plugin is compiled into a loadable module:
'grideye_http.so.1', and installed under /usr/local/lib/grideye and the grideye_agent is restarted:

```
   sudo systemctl restart grideye_agent.service
```

The new metrics appears in the grideye controller, for plotting, alarming and analysis.


## 4. Licenses

Grideye_agent is covered by GPLv3, _except_ the plugins that are
covered by copyright and licenses in their source code header (if any).

## 5. Contact

I can be found at olof@hagsand.se.