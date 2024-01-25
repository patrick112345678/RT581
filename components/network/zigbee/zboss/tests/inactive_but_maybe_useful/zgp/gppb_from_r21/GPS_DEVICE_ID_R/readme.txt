Test described in GP test specification, clause 4.1.1 Test GPEP DeviceID read-out

Test Harness tool - as ZR
DUT GPS           - GPS as ZC

dut_gps starts as ZC, th-tool joins to network and start read simple descriptor

4.1.1 Test procedure
commands sent by TH-tool:

1:
- TH-ZR requests a Simple Descriptor from the DUT-GPS on GPEP endpoint (0xF2).

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
