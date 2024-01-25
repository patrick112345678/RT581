Test described in GP test specification, clause 2.3.5 GPDF security level 0b11

TH Commissioning tool
TH GPP            - GPPB as ZC
DUT GPD           - GPD ON/OFF device

th_gpp starts as ZC, th-commissioning-tool joins to network and
send proxy commissioning mode to ZC, send gp paring configuration for DUT GPD

2.3.4 Test procedure
commands sent by DUT:

1:
- DUT-GPD sends Data GPDF.

2:
- DUT-GPD sends another Data GPDF.

Test note:
Note: the MAC sequence number does not have to be equal to the 1LSB of the Security Frame Counter.
This is not correct. In specification A.1.6.4.4:
For gpdSecurityLevel 0b10 and 0b11, the MAC sequence number field SHOULD carry the 1LSB of the gpdSecurityFrameCounter.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
