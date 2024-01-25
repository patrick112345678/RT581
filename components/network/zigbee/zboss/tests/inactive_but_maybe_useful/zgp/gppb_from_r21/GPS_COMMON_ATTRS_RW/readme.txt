Test described in GP test specification, clause 4.1.2.3	Common attributes

Test Harness tool - as ZR
DUT GPS           - GPS as ZC

dut_gps starts as ZC, th-tool joins to network and start read/write attrs

4.1.2.3 Test procedure
commands sent by TH-tool:

1:
- TH-ZR reads out the value of the gpSharedSecurityKeyType attribute, using ZCL Read Attributes command.
- TH-ZR writes a different value M (e.g. byte 0x07) to the gpSharedSecurityKeyType attribute, using ZCL Write Attributes No Response command. 
- TH-ZR reads out the value of the gpSharedSecurityKeyType attribute.

2:
- TH-ZR reads out the value of the gpSharedSecurityKey attribute.
- TH-ZR writes a different value M (e.g. 0xB0 0xB1 0xB2 0xB3 0xB4 0xB5 0xB6 0xB7 0xB8 0xB9 0xBA 0xBB 0xBC 0xBD 0xBE 0xBF) to the gpSharedSecurityKey attribute.
- TH-ZR reads out the value of the gpSharedSecurityKey attribute. 

3:
- TH-ZR reads out the value of the gpLinkKey attribute.
- TH-ZR writes different value M (e.g. ZigbeeAlliance11) to the gpLinkKey attribute.
- TH-ZR reads out the value of the gpLinkKey attribute.

4:
- TH-ZR reads out the value of the ClusterRevision attribute (AttributeID = 0xfffd, a Cluster global attribute).

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
