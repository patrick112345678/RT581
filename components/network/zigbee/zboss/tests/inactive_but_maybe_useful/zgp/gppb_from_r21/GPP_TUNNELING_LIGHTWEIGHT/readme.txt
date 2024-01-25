Test described in GP test specification, clause 5.3.1.6	Lightweight unicast communication

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-tool joins to network, th-gpd pairing with th-tool

5.3.1.6 Test procedure
commands sent:

1:
- Lightweight unicast forwarding for ApplicationID = 0b000, derived alias:
  A pairing with lightweight unicast communication mode and SecurityLevel = 0b00 (incremental MAC sequence number) is established
  between the TH-GPD with GPD SrcID = X (ApplicationID = 0b000) and TH-GPT (e.g. as a result of any of the commissioning procedures).
  AssignedAlias sub-field of the Options field is set to 0b0. Security Frame counter N.
- The corresponding information for GPD SrcID=X is also included in DUT-GPP (e.g. as a result of any of the commissioning procedures).
- Read out Proxy Table of DUT-GPP.
- Make TH-GPD send a Data GPDF with MAC sequence number M >= N+1, ApplicationID = 0b000, SecurityLevel 0b00, RxAfterTx 0b0, SrcID = Z,
  Endpoint absent, and GPD CommandID (e.g. 0x22).

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
