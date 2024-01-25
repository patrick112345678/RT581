Test described in GP test specification, 
    clause 4.4.2 Multi-hop commissioning
	sub-case 4.4.2.8 Multi-hop commissioning: general tests

Test Harness GPP or TH-Tool - as ZC
Test Harness GPD - as ZGPD
DUT GPS          - GPS (GPT/GPT+/GPC/GPCB) as ZR

DUT-GPS and TH-GPP operate in the same Zigbee PAN.
TH-GPP starts as ZC and DUT-GPS joins to TH-GPP as ZR.
Then TH-GPP send security attributes to DUT-GPS and read out Sink Table from DUT-GPS.
After that TH-GPD Comissioning to DUT-GPS and after successful commissioning,
TH-GPP read out Sink Table from DUT-GPS again for compare.

The DUT-GPS shall be in wireless communication proximity of TH-GPP.
TH-GPP shall be in wireless communication proximity of the TH-GPD.
If DUT-GPS is GPT+ or GPC they must be out of range of TH-GPD.

- DUT-GPS:
    PANId= Generated in a random manner (within the range 0x0001 to 0x3FFF)
    Logical Address = Generated in a random manner(within the range 0x0000 to 0xFFF7)
    IEEE address=manufacturer-specific 
    DUT-GPS supports multi-hop commissioning.
- TH-GPP:
    PAN id=same PANId as DUT-GPS
    Logical Address=Generated in a random manner(within the range 0x0000 to 0xFFF7)
    IEEE address=manufacturer-specific
- TH-GPD:
    GPD ID = N
    GPD security usage according to DUT capabilities; 
    TH-GPD application functionality matches that of the DUT-GPS’s GPEP.
    TH-GPD implements bidirectional commissioning procedure.

Note: the presence of the TH-GPD may be emulated by TH-GPP sending appropriate commands.

A packet sniffer shall be observing the communication over the air interface.

Initial Conditions:
 - No pairing exists between DUT-GPS and TH-GPD; there is not Proxy Table entry for TH-GPD in the TH-GPP.

4.4.2.8 Test procedure
commands sent by TH-tool and TH-gpd:

1: Negative test: different frame counter in both modes:

- TH-GPS enters commissioning mode.
- Commissioning is successfully performed between TH-GPP (on behalf of THGPD) and DUT-GPS;
    gpdSecurityLevel >= 0b10 is used. After DUT-GPS sends GP Pairing,

- TH-GPP reads out Sink Table entry of the DUT-GPS.

- TH-GPP is made to send a GP Notification carrying Data GPDF with wrong security frame counter number 
    (lower than or equal to that carried in commissioning mode by the last Commissioning GPDF payload).

- TH-GPP reads out Sink Table entry of the DUT-GPS.

  --- Security errors in commissioning ---

2A: Negative test (gpdSecurityLevel = 0b01):

- Clean Sink Table of the DUT-GPS.
- DUT-GPS enters commissioning mode.
- TH-GPP (or TH-Tool) sends GP Commissioning Notification with all settings as supported by DUT-GPS, 
    only with SecurityLevelCapabilities sub-field of the Extended Options field of the 
    GPD Commissioning command set to 0b01.

- Read out Sink Table entry of the DUT-GPS.
- If DUT-GPS supports Translation Table functionality, send GP Translation Table request to DUT-GPS. 

2B: Negative test: gpdSecurityLevel >=0b10, no key:

- Clean Sink Table of the DUT-GPS.
- DUT-GPS enters commissioning mode.
- TH-GPP sends the GP Commissioning Notification with RxAfterTx = 0b0 carrying GPD Commissioning command with:
    • SecurityLevelCapabilities sub-field of the Extended Options field != 0b00,
    • GPDkey present sub-field of the Extended Options field set to 0b0 and GPDkey field not present;
    • all other sub-fields and fields set as required by the DUT-GPS.

- Read out Sink Table entry of the DUT-GPS.
- If DUT-GPS supports Translation Table functionality, send GP Translation Table request to DUT-GPS.

2C: Negative test: GPD Key not encrypted:

- Clean Sink Table of the DUT-GPS.
- Set Protection with gpLinkKey sub-field of the gpsSecurityLevel attribute to 0b1.
- DUT-GPS enters commissioning mode.
- TH-GPP sends the GP Commissioning Notification with RxAfterTx = 0b0 carrying GPD Commissioning command with:
    • SecurityLevelCapabilities sub-field of the Extended Options field set to the minimum security level 
      required by DUT-GPD, as indicated in Minimal GPD Security Level sub-field of the gpsSecurityLevel attribute, 
    • GPDkey present sub-field of the Extended Options field set to 0b1, GPD Key encryption = 0b0 and
      GPDkey field present in the clear; GPD Key MIC field absent, GPD outgoing counter field present;
    • all other sub-fields and fields set as required by the DUT-GPS.

- Read out Sink Table entry of the DUT-GPS.
- If DUT-GPS supports Translation Table functionality, send GP Translation Table request to DUT-GPS.

2D: Negative test (only if sink supports both gpdSecurityLevel 0b10 and 0b11): too low security level:

- Clean Sink Table of the DUT-GPS.
- Set Minimal GPD Security Level sub-field of the gpsSecurityLevel attribute to 0b11.
- DUT-GPS enters commissioning mode.
- TH-GPP sends the GP Commissioning Notification with RxAfterTx = 0b0 carrying GPD Commissioning command with:
    • SecurityLevelCapabilities sub-field of the Extended Options field = 0b10, 
    • GPDkey present sub-field of the Extended Options field set to 0b1, GPD Key encryption = 0b1 and
      GPDkey field present and correctly encrypted; GPD Key MIC field present, GPD outgoing counter field present;
    • all other sub-fields and fields set as required by the DUT-GPS.
    
- Read out Sink Table entry of the DUT-GPS.
- If DUT-GPS supports Translation Table functionality, send GP Translation Table request to DUT-GPS.

  --- Malformed commissioning frame ---

3: Negative test: Commissioning GPDF with wrong ApplicationID:

- Clean Sink Table of DUT-GPS.
- DUT-GPS enters commissioning mode.
- TH-GPP sends GP Commissioning Notification carrying Commissioning GPDF,
  with all the options as supported by the TH-GPS, with RxAfterTx = 0b0 (or not present),
  but with ApplicationID sub-field of the Options field set to 0b011.

  --- GPD ID = 0x00..00 ---

4A: Negative test: SrcID = 0x00000000:

- Clean Sink Table of DUT-GPS.
- DUT-GPS enters commissioning mode.
- TH-GPP sends GP Commissioning Notification carrying Commissioning GPDF with:
    RxAfterTx = 0b0 and ApplicationID = 0b000, SrcID = 0x00000000.

- Read out Sink Table of the DUT-GPS.

4B: Negative test: GPD IEEE address = 0x0000000000000000: 

- Clean Sink Table of DUT-GPS.
- DUT-GPS enters commissioning mode.
- TH-GPP sends GP Commissioning Notification carrying:
    RxAfterTx = 0b0 and ApplicationID = 0b010, GPD IEEE = 0x0000000000000000 and Endpoint X.

- Read out Sink Table of the DUT-GPS.

  --- Handling of GPD Commissioning command in operation ---

5: Negative test: GP Notification carrying GPD Commissioning in operational mode

- A pairing exists between DUT-GPS and TH-GPD; 
  if supported by DUT-GPS, it should preferably use SecurityLevel >= 0b10.
- DUT-GPS is in operational mode.
- TH-GPP send GP Notification carrying GPD Commissioning command for TH-GPD,
  indicating use of appropriate GPD security.

  --- Extendibility of GPD Commissioning frame ---

6: Clean Sink Table of the DUT-GPS.

- DUT-GPS enters commissioning mode.
- TH-GPP sends the GP Commissioning Notification carrying the Commissioning GPDF with:
    • RxAfterTx sub-field set to 0b0, 
    • the Reserved sub-fields of the Options field set to 0b1
    • all other sub-fields and fields set as required by the DUT-GPS; 
    • at the end of the GPD Commissioning command payload (i.e. after ApplicationInformation, if included), 5 additional bytes of payload, 0xe0 0xe1 0xe2 0xe3 0xe4, are applied; the maximal GPDF length is NOT exceeded.
- If required, DUT-GPS is put back into operational mode.

- Read out Sink Table entry of the DUT-GPS.
- If DUT-GPS supports Translation Table functionality, send GP Translation Table request to DUT-GPS.

Pass verdict:

1: DUT-GPS does NOT execute the GPDF.
   DUT-GPS does NOT write the new (incorrect) frame counter value to its Sink Table.

2A: 2B: 2C: 2D: 3:
   DUT-GPS does NOT send GP Pairing nor Device_annce for TH-GPD, 
           does NOT create Sink Table nor Translation Table entry for GPD ID N.

4A: 4B:
   DUT-GPS does NOT create Sink Table entry.
        It does NOT send GP Pairing.

5: DUT-GPS does NOT open commissioning mode nor does send GP Proxy Commissioning Mode (Action=Enter) command.
   DUT-GPS still has Sink Table entry for TH-GPD.
   DUT-GPS MAY make a product-specific indication of the attempted commissioning.

6: DUT-GPS may send GP Proxy Commissioning Mode command (Action=Enter).

   On reception of Commissioning GPDF, DUT-GPS:
    • does NOT send a Commissioning Reply (since RxAfterTx of the received Commissioning GPDF is set to 0b0);
    • provides success indication,
    • sends out correctly formatted GP Pairing for TH-GPD:
    • with the exception of lightweight unicast communication mode,
      sends out Device_annce for the alias (assigned, if it was included in the GP Pairing command,
      and else the derived), with NWK source address equal to the alias, NWK sequence number = 0x00
      and APS Counter = 0x00.
   On reception of ZCL Read Attributes command, DUT-GPS responds with Read Attributes Response with
   Sink Table having one entry for the TH-GPD GPD ID N and all relevant parameters exchanged in the
   commissioning process are stored there.

   If DUT-GPS supports Translation Table functionality, on receipt of GP Translation Table  Request
   command, DUT-GPS responds with GP Translation Table Response command, containing default entries
   for GPD commands corresponding to TH-GPD DeviceID, being generic entries
   (with ApplicationID = 0b000 and SrcID = 0xffffffff) and/or entries for GPD ID = Z and the
   corresponding ApplicationID;
   with Endpoint field set to value other than 0xfd or 0x00 (e.g. 0xff or specific application endpoint).

   The value of the appended extra field 0xe0 0xe1 0xe2 0xe3 0xe4 shall not be misused as any GPD
   configuration value or its part.

Fail verdict:

1: DUT-GPS does execute the GPDF.
   AND/OR DUT-GPS does write the new (incorrect) frame counter value to its Sink Table.

2A: 2B: 2C: 2D: 3:
   DUT-GPS creates Sink Table entry for GPD ID = N with SecurityLevel = 0b01 
   AND/OR Translation Table entry(s) for GPD ID = N 
   AND/OR sends GP Pairing with status AddSink for GPD ID = N AND/OR Device_annce for GPD alias.

4A: 4B:
   DUT-GPS creates Sink Table entry AND/OR sends GP Pairing.

5: DUT-GPS does open commissioning mode 
   AND/OR does send GP Proxy Commissioning Mode (Action=Enter) command.
   AND/OR DUT-GPS removes Sink Table entry for TH-GPD.

6: On reception of Commissioning GPDF with RxAfterTx = 0b0, DUT-GPS:
    • does send Commissioning Reply GPDF (despite RxAfterTx of the received Commissioning GPDF being set to 0b0),
    • does not provide success indication, 
    • AND/OR does not send out GP Pairing for TH-GPD’s GPD ID with AddSink=0b1, RemoveGPD = 0b0, appropriate
      communication mode and security level for the DUT-GPS, and other parameters as derived from the Commissioning GPDF;
    • AND/OR does not send out Device_annce for the alias (assigned, if it was included in the GP Pairing command,
      and else the derived), with NWK source address equal to the alias, NWK sequence number = 0x00 and
      APS Counter = 0x00  in case of Communication Mode != 0b11 OR does send it in case of Communication Mode = 0b11.

   AND/OR On reception of ZCL Read Attributes command, DUT-GPS does not respond with Read Attributes
     Response with Sink Table having one entry for the TH-GPD GPD ID N and all relevant parameters
     established in the commissioning process.
   AND/OR DUT-GPS supporting Translation Table functionality does not have Translation Table entries,
     as described in pass verdict 1 above.
   AND/OR the value of the appended extra field 0xe0 0xe1 0xe2 0xe3 0xe4 appears as any GPD configuration
     value or its part.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
