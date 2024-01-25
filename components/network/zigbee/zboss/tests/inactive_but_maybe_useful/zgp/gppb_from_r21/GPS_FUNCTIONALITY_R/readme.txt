Test described in GP test specification, clause 4.1.3 Functionality support

Test Harness tool - as ZR
DUT GPS           - GPS as ZC

dut_gps starts as ZC, th-tool joins to network and read functionality attribute

4.1.3 Test procedure
commands sent by TH-tool:

1:
- TH-ZR reads out the value of the gpsFunctionality attribute.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
