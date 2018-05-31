# Grideye_agent
Cloud monitoring agent. 

## Table of contents

  * [Starting](#1-starting)
  * [Installation](#2-installation)
  * [Plugins](#3-plugins)
  * [Licenses](#4-licenses)
  * [Contact](#5-contact)

Grideye_agent is part of the Grideye cloud monitoring
software. Grideye consists of a controller, a dashboard, and
agents. This is the agent part.

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

As an alternative grideye_agent can be run as a docker
container. Simply pull the grideye_agent from DockerHub and pass the
needed URL, name and UUID as variables.

For example:

    docker run -it -e GRIDEYE_URL=<URL> -e GRIDEYE_UUID=<UUID> -e GRIDEYE_NAME=<name> cloudmon360/grideye_agent

## 2. Installation

A typical installation is as follows:

    > configure	       	        # Configure grideye_agent
    > make                      # Compile
    > sudo make install         # Install agent and plugins

The source builds one main program: grideye_agent. An example
startup-script is available in util/grideye_agent.

Grideye_agent requires [CLIgen](http://www.cligen.se) and
[CLIXON](http://www.clicon.org) for building. To build and install
CLIgen and CLIXON:

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

The following steps illustrate how to add the Nagios
(*check_http*)[https://www.monitoring-plugins.org/doc/man/check_http.html]
test plugin and how to incorporate it to produce input to grideye.
There are lots of Nagios plugins making it an easy way to extend
Grideye.

### 3.1 The API

A grideye plugin is a dynamically loaded plugin written in C. You
write a file with a couple of functions, compile it, place it in a
directory, and restart grideye_agent.

A grideye plugin has a pre-defined function *grideye_plugin_init()*
which returns a table containing API functions.

The API table is as follows:

Symbol | Type | Mandatory |Description
--- | --- | --- | ---
gp_version | Variable | Yes | Must be 2
gp_magic | Variable | Yes | Must be 0x3f687f03
gp_name | Variable | Yes | Name of plugin
gp_input_formar | Variable | No | Test input parameter format
gp_output_formar | Variable | No | Test output parameter format
gp_getopt_fn | Function | No | Generic function for getting plugin info
gp_setopt_fn | Function | No | Generic function for setting plugin info
gp_test_fn | Function | Yes | The actual test function with input and output parameters.
gp_exit_fn | Function | No | An exit function

gp_output | Variable | Yes | Format of output. Only "xml" supported

### 3.2 Identifying the input: parameters

check_http has lots of parameters. You can hardcode most, or leave as
defaults.

```
   check_http -H www.youtube.com -S
   HTTP OK: HTTP/1.1 200 OK - 495051 bytes in 1.751 second response time |
   time=1.751042s;;;0.000000 size=495051B;;;0
```

In this example, the *host* parameter is chosen as dynamically
configurable, which means it can dynamically change in a Grideye testcase.

This means 'gp_input=host' is defined in the plugin and gp_test_fn is called with 'host' as input parameter, example:

```
   gp_test_fn("www.youtube.com")
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

There are two ways to do this, either the new metrics are defined
directly in the file on the controller which describes then YANG
model. Or new metrics can be registered directly from the plugin.


These entries are added to the grideye YANG model on the controller as
follows:

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

The other way is to register the new metric in teh YANG model from the
plugin. This is done using the getopt function which is called on when
the plugin is loaded.

To register a new metric named hsize:

...
    int http_getopt(const char *optname,
	            char      **value)
    {
        if (strcmp(optname, "yangmetric"))
            return 0;
        if ((*value = strdup("{\"metrics\":{\"name\":\"hsize\",\"description\":\"HTTP response size\",\"type\":\"int32\"}}")) == NULL)
            return -1;
        return 0;
    }
...

And in the struct describing the plugin we set the getopt field to the
function above:

...
    static const struct grideye_plugin_api api = {
        2,
        GRIDEYE_PLUGIN_MAGIC,
        "sysinfo",
        "json",        /* input format */
        "xml",         /* output format */
        http_getopt,   /* getopt yangmetrics */
        NULL,
        http_test,     /* actual test */
        NULL
    };
...

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
      if (http_fork(_PROGRAM, "-H", host, "-S", buf, buflen) < 0)
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

### 3.7 Grideye plugin

The grideye plugin is compiled into a loadable module:
'grideye_http.so.1', and installed under /usr/local/lib/grideye and the grideye_agent is restarted:

```
   sudo systemctl restart grideye_agent.service
```

The new metrics appears in the grideye controller, for plotting, alarming and analysis.

### 3.8 Invoking the new test from the controller

The next step is to invoke the new plugin from the central grideye
controller. This is done by creating a new test (or extend an existing
test) with the new plugin.

The following example shows a new test that is added to an agent
template to invoke the new test:

```
    <test>
       <name>newtest</name>
       <interval>10000</interval>
       <plugin>
          <name>http</name>
          <param>www.youtube.com</param>
       </plugin>
    </test>
```

### 3.9 Plugins written in Python

Plugins can now be written in Python as well as in C. The plugin must
be named with grideye_ as a prefix to the name. For example
"grideye_pytest.py".

A very simple Python plugin can look like this:
    
	GRIDEYE_PLUGIN_VERSION=2
	GRIDEYE_PLUGIN_MAGIC=0x3f687f03

	# The function which will be called to do the actual test
	def pytest(instr):
		print("pytest called")

		# Result data
		return ["<xml-tag>value</xml-tag"]

	# This function will be called when the plugin is loaded.
	def grideye_plugin_init(version):
		print("Python plugin loaded! %d" % version)

		# This list of plugin parameters must be returned when
		# the plugin is loaded.
		grideye_plugin = [2,		# Plugin version
			GRIDEYE_PLUGIN_MAGIC,	# Plugin magic
			"pytest",		# Plugin name
			"str",			# Input format
			"xml",			# Output format
			None,			# Getopt function
			None,			# Setopt function
			"pytest",		# Test function
			None			# Exit function
		]

		if version != GRIDEYE_PLUGIN_VERSION:
			return None

		# Return the list of parameters
		return grideye_plugin

There must be a function named grideye_plugin_init which takes one
argument. The function should then return a list of items described
above.

When the plugins are called periodically the test function will be
called. This function is responsible for the actual test and should
return a list with one string describing the test result metric.

The test function will get the parameter configured on the controller
as its only argument.

## 4. Licenses

Grideye_agent is covered by GPLv3, _except_ the plugins that are
covered by copyright and licenses in their source code header (if any).

## 5. Contact

I can be found at olof@hagsand.se.
