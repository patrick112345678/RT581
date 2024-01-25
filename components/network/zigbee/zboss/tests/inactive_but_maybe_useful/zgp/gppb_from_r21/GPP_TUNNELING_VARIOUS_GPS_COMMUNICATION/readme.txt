Test described in GP test specification, clause 5.3.2 Several GPS with various communication modes

Test Harness tool 1 - as ZR
Test Harness tool 2 - as ZR
Test Harness tool 3 - as ZR
DUT GPP             - GPPB as ZC
Test Harness GPD    - as ZGPD

dut_gpp starts as ZC, th-tool1,th-tool2 and th-tool3 joins to network, th-gpd pairing with th-tools

5.3.2 Test procedure
commands sent:

1. Test order of multiple GP Notification messages NOT incl. full unicast, RxAfterTx = 0b0: 
- A pairing is established between TH-GPD and TH-GPT1, TH-GPT2, and TH-GPT3. For the Sink Table entry for GPD ID = X,
  the Communication Mode sub-field of the Options field has value 0b11 for TH-GPT1 (lightweight unicast),
  value 0b01 for TH-GPT2 (DGroup) and assigned alias 0xOOOO,
  and value 0b10 for TH-GPT3 and group address 0xMMMM (CGroup) and assigned alias 0xNNNN.
- The corresponding information is stored in the Proxy Table attribute of DUT-GPP.
- Read out Proxy Table entry of DUT-GPP.
- Make TH-GPD send a Data GPDF with RxAfterTx = 0b0.

2. Test order of multiple GP Notification messages NOT incl. full unicast, RxAfterTx = 0b1: 
- Make TH-GPD send a Data GPDF with RxAfterTx = 0b1.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
