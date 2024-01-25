Test described in GP test specification, clause 5.1.2 GPP Attribute writing and reading

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC

dut_gpp starts as ZC, th-tool joins to network and start read/write attrs

5.1.2.3.1 Test procedure
commands sent by TH-tool:

1:
- read gpSharedSecurityKeyType attribute
- attempts to write a different value M to the gpSharedSecurityKeyType attribute
- reads out the value of the gpSharedSecurityKeyType attribute

2:
- read gpSharedSecurityKey attribute
- attempts to wtite a different value M to the gpSharedSecurityKey attribute
- reads out the value of the gpSharedSecurityKey attribute

3:
- read gpLinkKey attribute
- attempts to wtite a different value M to the gpLinkKey attribute
- reads out the value of the gpLinkKey attribute

4:
- reads out ClusterRevision attribute

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
