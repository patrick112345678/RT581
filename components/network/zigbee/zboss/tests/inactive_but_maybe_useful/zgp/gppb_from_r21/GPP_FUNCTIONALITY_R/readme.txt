Test described in GP test specification, clause 5.1.3 Functionality support

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC

dut_gpp starts as ZC, th-tool joins to network and read functionality attribute

5.1.3 Test procedure
commands sent by TH-tool:

1:
- TH-ZR reads out the value of the gppFunctionality attribute.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
