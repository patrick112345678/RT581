Test described in GP test specification, clause 4.3.4.1 Commissioned Groupcast Communication Mode

Test Harness GPP or TH-Tool - as ZR
Test Harness GPD - as ZGPD
DUT GPS          - GPS (GPT/GPT+/GPC/GPCB) as ZC

DUT-GPS and TH-GPP operate in the same Zigbee PAN. They may 
join a Zigbee network created by another device, or one of them may 
start as ZC.

The DUT-GPS shall be in wireless communication proximity of TH-GPP.
TH-GPP shall be in wireless communication proximity of the TH-GPD.
If DUT-GPS is GPT+ or GPC they must be out of range of TH-GPD.

Note: a TH-Tool capable of sending GP Notification and GPD Commissioning command is preferably used instead of
a full GPP/GPPB + GPD. This also solves the problem of preventing direct GPDF reception by GPS with GP stub
(GPT+/GPC/GPCB).
A packet sniffer shall be observing the communication over the air interface.  

Initial Conditions:
 - The DUT-GPS’s application endpoint to-be-paired with the TH-GPD is known and it is a member of group 0xMMMM. 
   Preferably, this application endpoint to-be-paired with the TH-GPD is not a member of any other group.
 - A pairing with pre-commissioned groupcast communication mode is established between the TH-GPD and DUT-GPS 
   (e.g. as a result of any of the commissioning procedures), with the security settings as required by the DUT-GPS.
 - The corresponding information is also included in TH-GPP (e.g. as a result of any of the commissioning procedures), 
   with the Entry Active and Entry Valid of the Options field both having value 0b1, GroupID = 0xMMMM.

4.3.4.1 Test procedure
TH-GPP send Pairing Configuration command to DUT-GPS with appropriate TH-GPD information and modes.
Commands sent by TH-GPD:

1: TH-GPP send a GP Notification with the settings and for a GPD command according to the DUT-GPS pairing, 
   to GroupID = 0xMMMM; with ProxyInfoPresent = 0b1; and the fields GPP short address and GPP-GPD link present. 
   This can also be achieved by making the TH-GPD the send Data GPDF, but then for DUTGPT+/GPC/GPCB
   care  must  be  taken, than the Data GPDF from the GPD cannot be received directly. 

Pass verdict:
1:  DUT-GPS does react on received GP Notification. 
    DUT-GPS does NOT send APS ACK. 
    DUT-GPS does NOT send GP Notification Response or any other GP command.

Fail verdict:
1:  DUT-GPS does not react on received GP Notification. 
    AND/OR DUT-GPS does send APS ACK. 
    AND/OR DUT-GPS does send GP Notification Response or any other GP command.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
