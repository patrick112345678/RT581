Test described in GP test specification, clause 5.3.1.2	Derived Groupcast Communication Mode

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-tool joins to network, th-gpd pairing with th-tool

5.3.1.2 Test procedure
commands sent:

1:
- DGroup pairing with derived alias for ApplicationID = 0b000:
  A pairing with derived groupcast communication mode and SecurityLevel 0b00 (incremental sequence number) is established
  between the TH-GPD and TH-GPT (e.g. as a result of any of the commissioning procedures). Derived alias is used. Security Frame counter N.
- The corresponding information is also included in DUT-GPP (e.g. as a result of any of the commissioning procedures). 
- Read out Proxy Table of DUT-GPP.
- Make TH-GPD send a Data GPDF with MAC sequence number M => N+1, SecurityLevel 0b00, RxAfterTx = 0b0, SrcID = Z, Endpoint absent,
  and GPD CommandID (e.g. 0x22).

2:
- DGroup pairing with assigned alias for ApplicationID = 0b000:
  Clean DUT-GPP Proxy Table.
  A pairing with derived groupcast communication mode and SecurityLevel 0b00 (incremental sequence number) is established
  between the TH-GPD and TH-GPT (e.g. as a result of any of the commissioning procedures). Assigned alias 0xXXXX is used. Security Frame counter N.
- The corresponding information is also included in DUT-GPP (e.g. as a result of any of the commissioning procedures). 
- Read out Proxy Table of DUT-GPP.
- Make TH-GPD send a Data GPDF with MAC sequence number M => N+1, SecurityLevel 0b00, RxAfterTx = 0b0, SrcID = Z, Endpoint absent,
  and GPD CommandID (e.g. 0x22).

3:
- DGroup pairing with derived alias for ApplicationID = 0b010:
  Clean DUT-GPP Proxy Table.
  A pairing with derived groupcast communication mode is established between the TH-GPD and TH-GPT (e.g. as a result of any of the commissioning procedures).
  Derived alias is used. Security Frame counter N.
- The corresponding information is also included in DUT-GPP (e.g. as a result of any of the commissioning procedures),
  with the Entry Active and Entry Valid sub-fields of the Options field both having the value 0b1, and SecurityLevel 0b00.
- Make TH-GPD send a Data GPDF with MAC sequence number M => N+1, MAC header source address field carrying GPD IEEE address Z,
  SecurityLevel 0b00, RxAfterTx = 0b1, SrcID absent, Endpoint = X, and GPD CommandID (e.g. 0x22).

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
