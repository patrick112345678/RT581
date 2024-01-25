Test described in GP test specification, clause 5.1.1 Test GPEP DeviceID read-out

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC

dut_gpp starts as ZC, th-tool joins to network and start read simple descriptor

5.1.1 Test procedure
commands sent by TH-tool:

1:
- TH-ZR requests a Simple Descriptor from the DUT-GPP on GPEP endpoint (0xF2).

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
