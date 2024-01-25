Test described in GP test specification, clause 5.3.1.4	Commissioned Groupcast Communication Mode with limited Radius

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-tool joins to network, th-gpd pairing with th-tool

5.3.1.4 Test procedure
commands sent:

1: Groupcast pairing with limited Radius:
 - A pairing with derived/pre-commissioned groupcast communication mode is established between
   the TH-GPD and TH-GPT (e.g. as a result of any of the commissioning procedures).
   The corresponding information is also included in DUT-GPP (e.g. as a result of any of the commissioning procedures),
   with the Entry Active and Entry Valid sub-fields of the Options field both having the value 0b1, and SecurityLevel 0b00, , and Groupcast radius field set to 0x02.
 - Make TH-GPD send a Data GPDF with appropriate MAC sequence number, SecurityLevel, and RxAfterTx 0b0.


2: Groupcast pairing with default Radius:
 - Clean Proxy Table.
 - A pairing with derived/pre-commissioned groupcast communication mode is established between
   the TH-GPD and TH-GPT (e.g. as a result of any of the commissioning procedures).
   The corresponding information is also included in DUT-GPP (e.g. as a result of any of the commissioning procedures),
   with the Entry Active and Entry Valid sub-fields of the Options field both having the value 0b1, and SecurityLevel 0b00, , and Groupcast radius field set to 0x00.
 - Make TH-GPD send a Data GPDF with appropriate MAC sequence number, SecurityLevel, and RxAfterTx 0b0.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
