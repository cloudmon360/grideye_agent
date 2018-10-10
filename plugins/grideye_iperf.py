import re
import json
import subprocess

GRIDEYE_PLUGIN_VERSION=2
GRIDEYE_PLUGIN_MAGIC=0x3f687f03

#
# Register new metrics
#
def getopt(optname):
        return '''{"metrics":[
        {"name":"bytessent","description":"iperf bytes sent","type":"uint32","units":"bytes"},
        {"name":"bytesrecv","description":"iperf bytes recv","type":"uint32","units":"bytes"},
        {"name":"byteslost","description":"iperf bytes lost","type":"uint32","units":"bytes"},
        {"name":"retransmits","description":"iperf retransmits","type":"uint32","units":"bytes"},
        {"name":"bytessent","description":"iperf bytes sent","type":"uint32","units":"bytes"},
        {"name":"maxrtt","description":"iperf max rtt","type":"uint32","units":"ms"},
        {"name":"minrtt","description":"iperf min rtt","type":"uint32","units":"ms"},
        {"name":"meanrtt","description":"iperf mean rtt","type":"uint32","units":"ms"},
        {"name":"lostpercent","description":"iperf lost percent","type":"decimal64-6","units":"percent"}
        ]}'''

#
# The test is done here
#
def iperf_test(instr):

        args = []
        protocol = "tcp"
        rate = 0
        duration = 0
        host = ""
        port = 0

        for paramstr in instr:
                values = paramstr.decode("utf-8").split("=")
                if values[0] == "duration":
                        duration = values[1]
                if values[0] == "rate":
                        rate = values[1] + "K"
                if values[0] == "host":
                        host = values[1]
                if values[0] == "port":
                        port = values[1]
                if values[0] == "protocol":
                        protocol = values[1]

        if protocol != "tcp" and protocol != "udp":
                print("iperf: Invalid protocol: %s" % protocol)
                return ""

        if rate == 0 or duration == 0 or host == "" or port == 0:
                print("iperf: Rate, duration, host and port must be set")
                return ""

        args = ['iperf3', '-c', host, '-p', port, '-b', rate, '-t', duration, '-J']

        if protocol == 'udp' or protocol == 'UDP':
                args.append('-u')

        try:
                result = subprocess.check_output(args)
        except subprocess.CalledProcessError as e:
                print("iperf failed with error: " + e)
                return ""

        result = re.sub(r"\\n", "", str(result))
        result = re.sub(r"\\t", "", str(result))
        result = re.sub(r"b'", "", str(result))

        j = json.loads(result.rstrip('\''))

        maxrtt = j['end']['streams'][0]['sender']['max_rtt']
        minrtt = j['end']['streams'][0]['sender']['min_rtt']
        meanrtt = j['end']['streams'][0]['sender']['mean_rtt']
        sent_bytes = j['end']['streams'][0]['sender']['bytes']
        received_bytes = j['end']['streams'][0]['receiver']['bytes']
        retransmits = j['end']['streams'][0]['sender']['retransmits']
        lostbytes = sent_bytes - received_bytes
        lostpercent = "%.6f" % (((sent_bytes - received_bytes) * 100) / sent_bytes)

        retstr = "<bytessent>%d</bytessent>" % sent_bytes
        retstr += "<bytesrecv>%d</bytesrecv>" % received_bytes
        retstr += "<byteslost>%d</byteslost>" % lostbytes
        retstr += "<retransmits>%d</retransmits>" % retransmits
        retstr += "<maxrtt>%d</maxrtt>" % maxrtt
        retstr += "<minrtt>%d</minrtt>" % minrtt
        retstr += "<meanrtt>%d</meanrtt>" % meanrtt
        retstr += "<lostpercent>%s</lostpercent>" % lostpercent

        return retstr

#
# Called when the plugin is initialised
#
def grideye_plugin_init(version):
        grideye_plugin = [2,
		          GRIDEYE_PLUGIN_MAGIC,
		          "iperf",
		          "json",
		          "xml",
		          "getopt",
		          None,
		          "iperf_test",
		          None
        ]

        if version != GRIDEYE_PLUGIN_VERSION:
                return None

        return grideye_plugin


#
# For testing
#
if __name__ == '__main__':
        iperf_test([b"host=jenkins.krihal.se", b"port=4619", b"duration=2", b"rate=10000"])
