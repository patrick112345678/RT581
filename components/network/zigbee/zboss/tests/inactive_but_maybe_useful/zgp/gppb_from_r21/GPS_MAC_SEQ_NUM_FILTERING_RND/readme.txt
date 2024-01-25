Test described in GP test specification, 
    clause 4.3.8 MAC sequence number filtering 
	sub-clause 4.3.8.2 Random GPD MAC sequence numbers

Test Harness GPP or TH-Tool - as ZC
Test Harness GPD - as ZGPD with GPD ID = N
DUT GPS          - GPS (GPT/GPT+/GPC/GPCB) as ZR

DUT-GPS and TH-GPP operate in the same Zigbee PAN. TH-GPP starts as ZC and DUT_GPS joins to TH_GPP as ZR.

The DUT-GPS shall be in wireless communication proximity of TH-GPP.
TH-GPP shall be in wireless communication proximity of the TH-GPD.
If DUT-GPS is GPT+ or GPC they must be out of range of TH-GPD.

Note: a TH-Tool capable of sending GP Notification and GPD Commissioning command is preferably used instead of
a full GPP/GPPB + GPD. This also solves the problem of preventing direct GPDF reception by GPS with GP stub
(GPT+/GPC/GPCB).
A packet sniffer shall be observing the communication over the air interface.  

Initial Conditions:
 - A pairing is established between TH-GPD and DUT-GPS,
   in the communication mode as requested by the DUT-GPS.
   For the entry of the Sink Table with GPD ID value N,
   the MAC sequence number capability subfield of the Options field has value 0b0.
 - The corresponding information is available in TH-GPP’s Proxy Table attribute.

4.3.8.2 Test procedure
commands sent by TH-GPP and TH-GPD:

1: Make TH-GPP send a GP Notification command to DUT-GPS with
   GPD ID N and with GPD security frame counter field carrying 0xC3.

2: Make TH-GPP send a GP Notification command to DUT-GPS with
   GPD ID N and with GPD security frame counter field carrying 0xC3.

  *The time between two consecutive steps in the test should be at least gpDuplicateTimeout.

3: Make TH-GPP send a GP Notification command to DUT-GPS with
   GPD ID N and with GPD security frame counter field carrying 0xC2.

4: Make TH-GPP send a GP Notification command to DUT-GPS with
   GPD ID N and with GPD security frame counter field carrying 0xC5.

Pass verdict:
1: 2: 3: 4:
   DUT-GPS executes the command.
   If the CommunicationMode is full unicast, the DUT-GPS sends a GP Notification Response command to TH-GPP.

Fail verdict:
1: 2: 3: 4:
   For at least one of the four Data GPDFs, the DUT-GPS does not execute the command. 
   AfullND/OR If the CommunicationMode is full unicast, the DUT-GPS does not send a GP Notification Response command to TH-GPP.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
