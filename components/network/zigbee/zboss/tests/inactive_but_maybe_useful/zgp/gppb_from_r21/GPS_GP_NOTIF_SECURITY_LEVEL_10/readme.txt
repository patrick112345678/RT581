Test described in GP test specification, 
    clause 4.3.9 GPD security in GP Notification
	sub-clause 4.3.9.2 SecurityLevel 0b10

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
 - A pairing is established between the TH-GPD and DUT-GPT
   (e.g. as a result of any of the commissioning procedures).
   The corresponding information is also included in TH-GPP
   (e.g. as a result of any of the commissioning procedures).
 - The initial frame counter value (M) for TH-GPD is known.
 - TH-GPD, TH-GPP and DUT-GPS use SecurityLevel 0b10 and 
   SecurityKeyType 0b010 (GPD group key).

4.3.9.2 Test procedure
commands sent by TH-GPP and TH-GPD:

1: TH-GPD sends Data GPDF, with the Extended NWK Frame Control field present,
   the SecurityLevel sub-field set to 0b10, and the payload authenticated with
   4B MIC, using frame counter value 0xWWXXYYZZ >= M+1.
   OR: TH-GPP/TH-Tool GP Notification with the SecurityLevel sub-field set to 0b10,
   the frame counter value 0xWWXXYYZZ >= M+1 and GPD command as supported by DUT-GPS.

2: Negative test (incorrect frame counter):
   TH-GPP sends GP Notification with GPD command supported by DUT-GPS,
   SecurityLevel sub-field set to 0b10, KeyType sub-field
   set to 0b010, and an incorrect Frame counter value 0xWWXXYY00.

3: Negative test (incorrect security level):
   TH-GPP sends GP Notification with GPD command supported by DUT-GPS,
   with the SecurityLevel sub-field set incorrect (0b00), the KeyType sub-field 
   set to 0b010, and an correct Frame counter value 0xWWXXYYZZ+1.

4: Negative test (incorrect key type): TH-GPP sends GP Notification with GPD command
   supported by DUT-GPS, with the SecurityLevel set to 0b10, incorrect KeyType 0b100,
   and an correct Frame counter value 0xWWXXYYZZ+2.

Pass verdict:
1: DUT-GPS executes the GPD command and does store the new frame counter value.

2: 3: 4:
   DUT-GPS does not execute the GPD command and does not store the new frame counter value.
   DUT-GPS may send GP Pairing command for GPD ID N, with the correct security parameters values.

Fail verdict:
1: DUT-GPS does not execute the GPD command AND/OR does not store the new frame counter value.

2: 3: 4:
   DUT-GPS does execute the GPD command 
   AND/OR DUT-GPS does store the new frame counter value.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
