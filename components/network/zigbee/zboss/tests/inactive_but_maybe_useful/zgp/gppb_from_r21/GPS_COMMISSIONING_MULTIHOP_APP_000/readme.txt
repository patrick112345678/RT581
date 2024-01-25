Test described in GP test specification, 
    clause 4.4.2 Multi-hop commissioning
	sub-clause 4.4.2.1 Bidirectional multi-hop commissioning: 
	    shared key requested & delivered, ApplicationID = 0b000

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
 - TH-GPD uses ApplicationID = 0b000 and SrcID = N
    if GPS supports security, TH-GPD shall request the shared key by setting the 
    sub-fields Security Key request = 0b1; GPDkeyEncryption = 0b1 and 
    GPDkeyPresent = 0b1, encrypted GPD Key field is present and GPD Key MIC field is present.
 - DUT-GPS’s attributes have the value: gpSharedSecurityKeyType = 0b010 and the 
    gpSharedSecurityKey = 0xc0 0xc1 0xc2 0xc3 0xc4 0xc5 0xc6 0xc7 0xc8 0xc9 0xca 0xcb 0xcc 0xcd 0xce 0xcf.

4.4.2.1 Test procedure
commands sent by TH-tool and TH-gpd:

1A: DUT-GPS enters commissioning mode.

1B: Commissioning action is (repeatedly, if required) performed on TH-GPD
    (if manually: not more often than once per second), until DUT-GPS provides commissioning success feedback.
    The TH-GPD first sends Channel Request GPDF with:
      Frame Type sub-field of the NWK Frame Control field set to 0b01, 
      Auto-Commissioning: 
	0b0, if the GPD Channel Request transmission is immediately followed by reception window;
	0b1, if the GPD Channel Request transmission is immediately followed by another GPD Channel 
          Request transmission, on the same or another channel
      Frame Control Extension = 0b0; 
      and the Extended NWK Frame Control field, GPD SrcID field, Endpoint, Security frame counter and MIC field absent;
    The TH-GPD is configured such, that the GPD Channel Configuration has to be delivered on a
      channel other than the operational channel.
    TH-GPP forwards the corresponding GP Commissioning Notification, correctly formatted;
      in broadcast or unicast to DUT-GPS, depending on the Unicast communication sub-field setting
      in the GP Proxy Commissioning Mode command.

1C: After reception of GPD Channel Configuration, the TH-GPD sends Commissioning GPDF with: 
      Auto-Commissioning = 0b0, 
      RxAfterTx = 0b1 
      if DUT-GPS supports security, with GP Security Key request = 0b1,
      GPDkeyEncryption = 0b1 and GPDkeyPresent = 0b1; GPD Key field present 
      and encrypted and GPD Key MIC field present.
    TH-GPP forwards the corresponding GP Commissioning Notification, correctly formatted;
    in broadcast or unicast to DUT-GPS, depending on the Unicast communication sub-field
    setting in the GP Proxy Commissioning Mode command.

1D: After reception of Commissioning Reply GPDF, the TH-GPD sends Success GPDF with
      Auto-Commissioning = 0b0 and RxAfterTx = 0b0, protected using the security parameters
      exchanged in GPD Commissioning/Commissioning Reply.
    TH-GPP forwards the corresponding GP Commissioning Notification, correctly formatted, with:
      sub-fields of the Options field set to:
	- Security processing failed = 0b0,
	- SecurityLevel – copied from the SecurityLevel sub-field of the Extended NWK Frame Control field;
	- SecurityKeyType = 0b010;
      GPD CommandID in the clear and MIC field absent,
      the GPD security frame counter field carrying the value from the Security frame counter field of the Success GPDF;
    in broadcast or unicast to DUT-GPS, depending on the Unicast communication sub-field
      setting in the GP Proxy Commissioning Mode command.
    If required, DUT-GPS is put back into operational mode.
    
    TH-GPP reads out Sink Table entry of the DUT-GPS.
    If DUT-GPS supports Translation Table functionality, send GP Translation Table Request to DUT-GPS.

Pass verdict:

1A: DUT-GPS sends GP Proxy Commissioning Mode command (in broadcast or unicast to TH-GPP) with: 
      APS header fields:
	ProfileID: 0xA1E0 ;
	ClusterID: 0x0021;
	source endpoint: GPEP (242);
	destination endpoint: not present or if present, GPEP (242);
      ZCL header fields:
	Command Identifier: 0x02
	The sub-fields of the Frame Control field set to:
	  Frame type = 0b01 (cluster-specific command),
	  Manufacturer specific = 0b0.
	  Direction 0b1 (server to client),
	  Disable default response 0b1;
      Command payload:
	Action=Enter and Channel present sub-field set to 0b0;
	Unicast communication sub-field set according to DUT-GPS capabilities.

1B: On reception of (each) GP Commissioning Notification with RxAfterTx sub-field set to 0b1 
      and GPD Channel Request command, the DUT-GPS sends GP Response command (in broadcast or unicast to TH-GPP) with:
	APS header fields:
	  ProfileID: 0xA1E0;
	  ClusterID: 0x0021;
	  source endpoint: GPEP (242);
	destination endpoint: not present or if present, GPEP (242);
	ZCL header fields:
	  Command Identifier: 0x06
	  The sub-fields of the Frame Control field set to:
	    Frame type = 0b01 (cluster-specific command),
	    Manufacturer specific = 0b0.
	    Direction 0b1 (server to client),
	    Disable default response 0b1;
	Command payload:
	  ApplicationID sub-field of the Options field set to 0b000, 
	  GPD ID field set to 0x00000000;
	  Endpoint field absent;
	  carrying GPD Channel Configuration command with: 
	    operational channel
	    Basic = 0b0 if DUT-GPS is a Basic Sink, 
	  listing the TH-GPP in the TempMasterShortAddress field;
	  the TransmitChannel the TH-GPP shall transmit on, 
	    being one of the channels from the GPD Channel Request payload. 
    The payload of the GPD command  sent by the DUT-GPS in GP Response does NOT exceed:
	For a GPD with ApplicationID = 0b000: 64 octets;
	For a GPD with ApplicationID = 0b010: 59 octets.

1C: On reception of (each) GP Commissioning Notification with RxAfterTx sub-field set to 0b1
      and GPD Commissioning command, the DUT-GPS sends GP Response (in broadcast or unicast to TH-GPP) with:
	APS header fields:
	  ProfileID: 0xA1E0;
	  ClusterID: 0x0021;
	  source endpoint: GPEP (242);
	destination endpoint: not present or if present, GPEP (242);
	ZCL header fields:
	  Command Identifier: 0x06
	  The sub-fields of the Frame Control field set to:
	    Frame type = 0b01 (cluster-specific command),
	    Manufacturer specific = 0b0.
	    Direction 0b1 (server to client),
	    Disable default response 0b1;
	Command payload:
	  the ApplicationID sub-field of the Options field set to 0b000;
	  the GPD ID field of 4B length set to GPD SrcID of the TH-GPD
	    (as in the triggering GP Commissioning Notification);
	  carrying the GPD Commissioning Reply command,
    If DUT-GPS supports security and the Commissioning GPDF had 
	GP Security Key request = 0b1,
	GPDkeyEncryption= 0b1 and
	GPDkeyPresent = 0b1;
	the GPD Commissioning Reply command carries:
	  the shared key 0xc0 0xc1 0xc2 0xc3 0xc4 0xc5 0xc6 0xc7 0xc8 0xc9 0xca 0xcb 0xcc 0xcd 0xce 0xcf; encrypted;
	  GPD Key MIC field is present and Frame Counter field is present;
	  KeyType sub-field is set to 0b010; GPD key encryption = 0b1.
	listing the TH-GPP in the TempMasterShortAddress field and TransmitChannel set to the operational channel.
    The payload of the GPD command  sent by the DUT-GPS in GP Response does NOT exceed:
	For a GPD with ApplicationID = 0b000: 64 octets;
	For a GPD with ApplicationID = 0b010: 59 octets.

1D: On reception of GP Commissioning Notification with GPD Success command and RxAfterTx = 0b0, 
      DUT-GPS provides success indication, and sends out GP Pairing with:
	APS header fields:
	  ProfileID: 0xA1E0;
	  ClusterID: 0x0021;
	  source endpoint: GPEP (242);
	  destination endpoint, if present: GPEP (242);
	ZCL header fields:
	  Command Identifier: 0x01;
	  The sub-fields of the Frame Control field set to: 
	    Frame type = 0b01 (cluster-specific command), 
	    Manufacturer specific = 0b0, 
	    Direction = 0b1 (server to client), 
	    Disable default response = 0b1;
	ZCL payloadwith:
	  Sub-fields of the Options field set to: 
	    ApplicationID = 0b000, AddSink=0b1, RemoveGPD = 0b0;
	  Communication Mode: set according to DUT-GPS capabilities
	  GPD Fixed and GPD MAC sequence number capabilities – copied from the GPD Commissioning command payload;
	  SecurityLevel as exchanged during commissioning;
	  SecurityKeyType as exchanged during commissioning (if DUT-GPS supports security: 0b010);
	  Assigned Alias present and Forwarding Radius present: set according to DUT-GPS capabilities;
	  GPD SrcID = N; 
	  Endpoint field absent;
	Sink IEEE address and Sink NWK address fields:
	  present if Communication Mode sub-field of the Options field was set to 0b00 or 0b11; absent otherwise;
	Sink GroupID field:
	  present and set to derived alias, if Communication Mode sub-field of the Options field was set to 0b01;
	  present and set any value, if Communication Mode sub-field of the Options field was set to 0b10; absent otherwise;
	DeviceID field - copied from the GPD Commissioning command payload;
	GPD security Frame Counter field – as in GPD Success command;
	GPD key field:
	  present and carrying GPD Group Key in the clear, if SecurityLevel >= 0b10; absent otherwise;
	Assigned alias field:
	  present and set to any value if Assigned Alias present sub-field of the 
	    Options field was set to 0b1; absent otherwise;
	Forwarding Radius field: present and set to any value <= 0x1E
	  if Forwarding Radius present sub-field of the Options field was set to 0b1; absent otherwise.

    If DUT-GPS sets CommunicationMode sub-field of the Options field of the GP Pairing command 
      to value other than 0b11 (lightweight unicast): 
        DUT-GPS sends out Device_annce for the alias 
        (assigned, if it was included in the GP Pairing command, and else the derived), 
        with NWK source address equal to the alias, NWK sequence number = 0x00 and APS Counter = 0x00;
        if DUT-GPS set CommunicationMode sub-field of the Options field of the GP Pairing command to 0b11 
        (lightweight unicast), Device_annce SHALL not be sent.
      It may also send GP Proxy Commissioning Mode with Action=Exit.

    On reception of ZCL Read Attributes command, DUT-GPS responds with Read Attributes
      Response with a Sink Table having one entry for the TH-GPD GPD ID N and all relevant parameters
      exchanged in the commissioning process are stored there.

    If DUT-GPS supports Translation Table functionality, on receipt of GP Translation Table Request command,
      DUT-GPS responds with GP Translation Table Response command, containing default entries for GPD commands
      corresponding to TH-GPD DeviceID, being generic entries (with ApplicationID = 0b000 and SrcID = 0xffffffff)
      and/or entries for GPD ID = Z and the corresponding ApplicationID; with Endpoint field set to value other
      than 0xfd or 0x00 (e.g. 0xff or specific application endpoint).

Fail verdict:

1A: DUT-GPS does not sends GP Proxy Commissioning Mode command with 
      Action=Enter AND/OR Channel present sub-field is not set to 0b0.
      
1B: On reception of (each) GP Commissioning Notification with GPD Channel Request command
      and RxAfterTx = 0b1, the DUT-GPS does not send GP Response carrying 
      GPD Channel Configuration command, formatted exactly as specified in pass verdict 1B: above.
      
1C: On reception of (each) GP Commissioning Notification with GPD Commissioning command
      and RxAfterTx=0b1, the DUT-GPS does not send GP Response carrying Commissioning Reply command,
      formatted exactly as specified in pass verdict 1C: above.

1D: On reception of GP Commissioning Notification with GPD Success command, DUT-GPS does not
      provide success indication and/or does not send GP Pairing with AddSink=0b1,
      formatted exactly as specified in pass verdict 1D: above.
    AND/OR:
      in case of Communication Mode != 0b11: DUT-GPS does not send Device_annce for the alias with IEEE = 0xff..ff;
      in case of Communciation Mode = 0b11: DUT-GPS sends Device_annce for the alias.
    AND/OR DUT-GPS does not respond with Read Attributes Response having a Sink Table entry
      for the TH-GPD GPD ID N AND/OR all relevant parameters exchanged in the commissioning process are not stored there.
    AND/OR DUT-GPS supporting Translation Table functionality does not have Translation Table entries,
      as described in 4.4.2.1.4/item 1D: above.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
