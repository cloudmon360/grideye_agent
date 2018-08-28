import json
import iperf3

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

        protocol = "tcp"

        rate = 0
        duration = 0
        host = ""
        port = 0

        for paramstr in instr:
                values = paramstr.decode("utf-8").split("=")
                if values[0] == "duration":
                        duration = int(values[1])
                if values[0] == "rate":
                        rate = int(values[1])
                if values[0] == "host":
                        host = values[1]
                if values[0] == "port":
                        port = int(values[1])
                if values[0] == "protocol":
                        protocol = values[1]

        if protocol != "tcp" and protocol != "udp":
                print("iperf: Invalid protocol: %s" % protocol)
                return ""

        if rate == 0 or duration == 0 or host == "" or port == 0:
                print("iperf: Rate, duration, host and port must be set")
                return ""

        client = iperf3.Client()
        client.duration = duration
        client.server_hostname = host
        client.port = port
        client.bandwidth = rate
        client.protocol = protocol
        result = client.run()

        if result.error:
                print("iperf: Error: %s" % result.error)
                return ""

        j = json.loads(str(result))

        maxrtt = j['end']['streams'][0]['sender']['max_rtt']
        minrtt = j['end']['streams'][0]['sender']['min_rtt']
        meanrtt = j['end']['streams'][0]['sender']['mean_rtt']
        lostbytes = result.sent_bytes - result.received_bytes
        lostpercent = "%.6f" % (((result.sent_bytes - result.received_bytes) * 100) / result.sent_bytes)

        retstr = "<bytessent>" + str(result.sent_bytes) + "</bytessent>" + \
                 "<bytesrecv>" + str(result.received_bytes) + "</bytesrecv>" + \
                 "<byteslost>" + str(lostbytes) + "</byteslost>" + \
                 "<retransmits>" + str(result.retransmits) + "</retransmits>" + \
                 "<maxrtt>" + str(maxrtt) + "</maxrtt>" + \
                 "<minrtt>" + str(minrtt) + "</minrtt>" + \
                 "<meanrtt>" + str(meanrtt) + "</meanrtt>" + \
                 "<lostpercent>" + str(lostpercent) + "</lostpercent>"

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
        iperf_test([b"host=speedtest.serverius.net", b"port=5002", b"duration=1", b"rate=1"])
