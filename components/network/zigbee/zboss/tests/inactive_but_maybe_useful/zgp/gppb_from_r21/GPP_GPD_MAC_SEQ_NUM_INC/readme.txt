Test described in GP test specification,
    clause 5.2 Basic GPDF frame format
	sub-clause 5.2.4 Incremental GPD MAC sequence numbers

Test Harness GPT or TH-Tool - as ZR
Test Harness GPD - as ZGPD with GPD ID = N
DUT GPP          - GPP (GPPB) as ZC

TH-GPT and DUT-GPP operate in the same Zigbee PAN. DUT-GPP starts as ZC and TH-GPT joins to DUT-GPP as ZR.

The TH-GPT shall be in wireless communication proximity of DUT-GPP.
DUT-GPP shall be in wireless communication proximity of the TH-GPD.
If TH-GPT must be out of range of TH-GPD.

A packet sniffer shall be observing the communication over the air interface.

Initial Conditions:
 - A pairing with derived groupcast communication mode is established
   between the TH-GPD and TH-GPT (e.g. as a result of any of the
   commissioning procedures); 
 - No security and no alias is used.
 - The corresponding information is also included in the DUT-GPP (e.g. as a
   result of any of the commissioning procedures) with the Entry Active and
   Entry Valid sub-fields of the Options field both having value 0b1.
 - the Sequence Number Capabilities sub-field of the Options field 
   of the DUT-GPP's Proxy Table entry has value 0b0.
 - the SecurityFrameCounter parameter of the DUT-GPP's Proxy Table entry has value < 0xC3.

5.2.4 Test procedure
commands sent by TH-GPD:

1: Make TH-GPD send a Data GPDF with Sequence Number 0xC3.

2: Make TH-GPD send a Data GPDF with Sequence Number 0xC3.

  *The time between two consecutive steps in the test should be at least gpDuplicateTimeout.

3: Make TH-GPD send a Data GPDF with Sequence Number 0xC2.

4: Make TH-GPD send a Data GDPF with Sequence Number 0xC5.

5: Make TH-GPD send a Data GDPF with Sequence Number 0x05.

Pass verdict:
1: 2: 3: 4: 5:
   DUT-GPP sends a GP Notification command in groupcast to TH-GPT,with the
   GPD Security Frame Counter field carrying 0x00 on the 3MSB and the MAC 
   sequence number value of the received GPDF on the 1LSB.

Fail verdict:
1: 2: 3: 4: 5:
   For at least one of the Data GPDFs, DUT-GPP does not send 
   the GP Notification command in groupcast to TH-GPT.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
