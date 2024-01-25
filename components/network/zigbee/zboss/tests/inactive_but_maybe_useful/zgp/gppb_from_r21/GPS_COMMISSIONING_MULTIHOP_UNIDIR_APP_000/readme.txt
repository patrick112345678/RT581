Test described in GP test specification, 
    clause 4.4.2 Multi-hop commissioning
	sub-case 4.4.2.2 Unidirectional multi-hop commissioning

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
 - TH-GPD uses ApplicationID = 0b000 and SrcID = N if GPS supports security, TH-GPD shall
    provide OOB key and set the subfields Security Key request = 0b0, GPDkeyEncryption = 0b1
    and GPDkeyPresent = 0b1; encrypted GPD Key field is present and GPD Key MIC field is present.

4.4.2.2 Test procedure
commands sent by TH-tool and TH-gpd:

1A: GPD addressed by SrcID:

 - DUT-GPS enters commissioning mode.

 - TH-GPD is triggered to send on the operational channel of DUT-GPS the Commissioning GPDF
    with ApplicationID = 0b000, AutoCommissioning sub-field of the NWK Frame Control field set to 0b0,
    the Extended NWK Frame Control field present and its RxAfterTx subfield set to 0b0,
    GPD SrcID = N and all other fields set as required by the DUT-GPS.

 - TH-GPP forwards the corresponding GP Commissioning Notification, correctly formatted;
    in broadcast or unicast to DUT-GPS, depending on the Unicast communication sub-field
    setting in the GP Proxy Commissioning Mode command.

     - If required, DUT-GPS is put back into operational mode.

 - TH-GPP Reads out Sink Table entry of the DUT-GPS.

 - If DUT-GPS supports Translation Table functionality, send GP Translation Table request to DUT-GPS.

 - TH-GPD sends a correctly formatted Data GPDF.

1B: GPD addressed by IEEE:

 - TH-GPP send command to Clean Sink Table of DUT-GPS.

 - DUT-GPS enters commissioning mode.

 - TH-GPD is triggered to send on the operational channel of DUT-GPS the Commissioning GPDF
    with ApplicationID = 0b010, AutoCommissioning sub-field of the NWK Frame Control field set to 0b0, 
    the Extended NWK Frame Control field present and its RxAfterTx subfield set to 0b0,
    GPD IEEE address (in the MAC header source address field) = N, Endpoint field set to endpoint X from the range 0x01 –
    0xFE), and all other fields set as required by the DUT -GPS.

 - TH-GPP forwards the corresponding GP Commissioning Notification, correctly formatted;
    in broadcast or unicast to DUT-GPS, depending on the Unicast communication sub-field
    setting in the GP Proxy Commissioning Mode command.

 - If required, DUT-GPS is put back into operational mode.

 - TH-GPP Read out Sink Table entry of the DUT-GPS.

 - If DUT-GPS supports Translation Table functionality, send GP Translation Table request to DUT-GPS.

 - TH-GPD sends a correctly formatted Data GPDF, with Endpoint = X.

Pass verdict:

1A: DUT-GPS does send GP Proxy Commissioning Mode command with:
    • APS header fields:
      -	ProfileID: 0x0A1E0;
      -	ClusterID: 0x0021;
      -	source endpoint: GPEP (242);
      -	destination endpoint: not present or if present, GPEP (242);
    • ZCL header fields:
      -	Command Identifier: 0x02
      -	The sub-fields of the Frame Control field set to: 
          Frame type = 0b01 (cluster-specific command), Manufacturer specific = 0b0. 
          Direction 0b1 (server to client), Disable default response 0b1; 
    • Command payload:
      -	Action=Enter and Channel present sub-field set to 0b0;
      -	Unicast communication sub-field set according to DUT-GPS capabilities.


    On reception of GP Commissioning Notification carrying GPD Commissioning command with RxAfterTx = 0b0, 
    DUT-GPS: provides success indication, sends out GP Pairing for TH-GPD with:
    • APS header fields:
      - ProfileID: 0xA1E0;
      - ClusterID: 0x0021;
      - source endpoint: GPEP (242);
      - destination endpoint, if present: GPEP (242);
    • ZCL header fields:
      -	Command Identifier: 0x01;
      -	The sub-fields of the Frame Control field set to: Frame type = 0b01 (cluster-specific command), 
        Manufacturer specific = 0b0, Direction = 0b1 (server to client), Disable default response = 0b1;
    • ZCL payload with:
      -	Sub-fields of the Options field set to: 
         ApplicationID = 0b000, AddSink=0b1, RemoveGPD = 0b0;
         Communication Mode: set according to DUT-GPS capabilities
        - GPD Fixed and GPD MAC sequence number capabilities – copied from the GPD Commissioning command payload;
         SecurityLevel as exchanged during commissioning;
         SecurityKeyType as exchanged during commissioning (if DUT-GPS supports security: 0b100);
         Assigned Alias present and Forwarding Radius present: set according to DUT-GPS capabilities;
      - GPD SrcID = N;
      - Endpoint field absent;
      - Sink IEEE address and Sink NWK address fields:
         present if Communication Mode sub-field of the Options field was set to 0b00 or 0b11; 
        - absent otherwise;
      - Sink GroupID field:
         present and set to derived alias, if Communication Mode sub-field of the Options field was set to 0b01;
         present and set any value, if Communication Mode sub-field of the Options field was set to 0b10;
         absent otherwise;
      - DeviceID field - copied from the GPD Commissioning command payload;
      - GPD security Frame Counter field – as in GPD Commissioning command;
      - GPD key field:
         present and carrying TH-GPD’s OOB key in the clear, if SecurityLevel >= 0b10;
         absent otherwise;
      - Assigned alias field:
         present and set to any value if Assigned Alias present sub-field of the Options field was set to 0b1; 
         absent otherwise;
      - Forwarding Radius field:
         present and set to any value <= 0x1E if Forwarding Radius present sub-field of the Options field was set to 0b1; 
         absent otherwise.

    If DUT-GPS sets CommunicationMode sub-field of the Options field of the GP Pairing command to value 
    other than 0b11 (lightweight unicast): 
      DUT-GPS sends out Device_annce for the alias (assigned, if it was included in the GP Pairing command, 
      and else the derived), with NWK source address equal to the alias, and NWK sequence number = 0x00
      and APS Counter = 0x00;
    if DUT-GPS set CommunicationMode sub-field of the Options field of the GP Pairing command to 0b11 
    (lightweight unicast), Device_annce SHALL not be sent;

    does NOT send GP Response carrying GPD Commissioning Reply command (since the RxAfterTx sub-field of the
      received GP Commissioning Notification command is set to 0b0).

    On reception of ZCL Read Attributes command, DUT-GPS responds with Read Attributes Response with Sink Table
      having one entry for the TH-GPD with ApplicationID = 0b000, GPD SrcID= N, Endpoint field absent and all relevant 
      parameters exchanged in the commissioning process are stored there.

    If DUT-GPS supports Translation Table functionality, on receipt of GP Translation Table Request command, DUT-GPS 
      responds with GP Translation Table Response command, containing default entries for GPD commands corresponding 
      to TH-GPD DeviceID, being generic entries (with ApplicationID = 0b000 and SrcID = 0xffffffff) and/or entries for
      GPD ID = Z and the corresponding ApplicationID; with Endpoint field set to value other than 0xfd or 0x00 
      (e.g. 0xff or specific application endpoint).

    DUT-GPS correctly executes the GPD command in the Data GPDF.

1B: DUT-GPS does send GP Proxy Commissioning Mode command with Action sub-field of the Options field set to Enter.

    On reception of GP Commissioning Notification carrying GPD Commissioning command with RxAfterTx = 0b0, 
    DUT-GPS: provides success indication, sends out GP Pairing for TH-GPD with:
    • APS header fields:
      - ProfileID: 0xA1E0;
      - ClusterID: 0x0021;
      - source endpoint: GPEP (242);
      - destination endpoint, if present: GPEP (242);
    • ZCL header fields:
      - Command Identifier: 0x01;
      - The sub-fields of the Frame Control field set to:
        Frame type = 0b01 (cluster-specific command),
        Manufacturer specific = 0b0, Direction = 0b1 (server to client),
        Disable default response = 0b1;
    • ZCL payload with:
      - Sub-fields of the Options field set to: 
	 ApplicationID = 0b010, AddSink=0b1, Remove GPD = 0b0,
	 Communication Mode: set according to DUT-GPS capabilities
	 GPD Fixed and GPD MAC sequence number capabilities – copied from the GPD Commissioning command payload;
	 SecurityLevel as exchanged during commissioning;
	 SecurityKeyType as exchanged during commissioning (if DUT-GPS supports security: 0b100);
	 Assigned Alias present and Forwarding Radius present: set according to DUT-GPS capabilities;
      - GPD ID of 8B length set to N; 
      - Endpoint = X;
      - Sink IEEE address and Sink NWK address fields:
	 present if Communication Mode sub-field of the Options field was set to 0b00 or 0b11; 
	 absent otherwise;
      - Sink GroupID field:
	 present and set to derived alias, if Communication Mode sub-field of the Options field was set to 0b01;
	 present and set any value, if Communication Mode sub-field of the Options field was set to 0b10;
	 absent otherwise;
      - DeviceID field - copied from the GPD Commissioning command payload;
      - GPD security Frame Counter field – as in GPD Comissioning command;
      - GPD key field:
	 present and carrying TH-GPD’s OOB key in the clear, if SecurityLevel >= 0b10;
	 absent otherwise;
      - Assigned alias field:
        - present and set to any value if Assigned Alias present sub-field of the Options field was set to 0b1; 
        - absent otherwise;
      - Forwarding Radius field:
	 present and set to any value <= 0x1E if Forwarding Radius present sub-field of the Options field was set to 0b1; 
	 absent otherwise.

    If DUT-GPS sets CommunicationMode sub-field of the Options field of the GP Pairing command to value
    other than 0b11 (lightweight unicast):
      DUT-GPS sends out Device_annce for the alias (assigned, if it was included in the GP Pairing command,
      and else the derived), with NWK source address equal to the alias, NWK sequence number = 0x00
      and APS Counter = 0x00; 
    if DUT-GPS set CommunicationMode sub-field of the Options field of the GP Pairing command to 0b11
    (lightweight unicast), Device_annce SHALL not be sent;

    does NOT send GP Response carrying GPD Commissioning Reply command (since the RxAfterTx sub-field
    of the received GP Commissioning Notification command is set to 0b0) .

    On reception of ZCL Read Attributes command, DUT-GPS responds with Read Attributes Response with 
    Sink Table having one entry for the TH-GPD with ApplicationID = 0b010 , GPD IEEE address = N, 
    Endpoint = X, and all relevant parameters exchanged in the commissioning process are stored there.

    If DUT-GPS supports Translation Table functionality, on receipt of GP Translation Table Request
    command, DUT-GPS responds with GP Translation Table Response command, containing default entries
    for GPD commands corresponding to TH-GPD DeviceID, being generic entries 
    (with ApplicationID = 0b000 and SrcID = 0xffffffff) and/or entries for ApplicationID = 0b010,
    GPD IEEE address = N, GPD Endpoint = X; with Endpoint field set to value other than 0xfd or 0x00 
    (e.g. 0xff or specific application endpoint).

    DUT-GPS correctly executes the GPD command in the Data GPDF with Endpoint X.

Fail verdict:

1A: DUT-GPS does not send GP Proxy Commissioning Mode command with Action sub-field of the Options field set to Enter.
    AND/OR On reception of GP Commissioning Notification carrying the 
    GPD Commissioning command with RxAfterTx = 0b0, DUT-GPS:
    • does not provide success indication, 
    • AND/OR does not send out GP Pairing for TH-GPD  with ApplicationID= 0b000,
      SrcID = N, Endpoint field absent, AddSink=0b1, Remove GPD = 0b0, appropriate
      communication mode and security level for the DUT-GPS, and other parameters
      as derived from the Commissioning GPDF;
    • AND/OR:
      - In case of Communication Mode !=0b11: does not send out Device_annce for
        the alias (assigned, if it was included in the GP Pairing command, and else
        the derived), with NWK source address equal to the alias, NWK sequence
        number = 0x00  and APS Counter = 0x00;
      - In case of Communication Mode = 0b11: does send Device_annce for the alias;
    • AND/OR does send GP Response carrying GPD Commissioning Reply command 
      (despite the RxAfterTx sub-field of the received GP Commissioning Notification command being set to 0b0);
    AND/OR On reception of ZCL Read Attributes command, DUT-GPS does not respond with Read Attributes 
      Response with Sink Table having one entry for the TH-GPD with ApplicationID= 0b000, SrcID = N, 
      Endpoint field absent, and all relevant parameters established in the commissioning process.
    AND/OR DUT-GPS supporting Translation Table functionality does not have Translation Table entries,
      as described in 4.4.2.2.4/item 1 above.

1B: DUT-GPS does NOT send GP Proxy Commissioning Mode command with Action sub-field of the Options field set to Enter.
    AND/OR On reception of GP Commissioning Notification carrying the
    GPD Commissioning command with RxAfterTx = 0b0, DUT-GPS:
    • does NOT provide success indication, 
    • AND/OR does NOT send out GP Pairing for TH-GPD with AddSink=0b1, Remove GPD = 0b0, 
      ApplicationID = 0b010, GPD ID = N, Endpoint field = X, appropriate communication 
      mode and security level for the DUT-GPS, and other parameters as derived from the Commissioning GPDF;
    • AND/OR: 
      - In case of Communication Mode != 0b11: does NOT send out Device_annce for the alias
        (assigned, if it was included in the GP Pairing command, and else the derived),
        with NWK source address equal to the alias, NWK sequence number = 0x00 and  APS Counter = 0x00; 
      - In case of Communication Mode = 0b11: does send Device_annce for the alias;
    • AND/OR sends GP Response carrying GPD Commissioning Reply command 
      (since the RxAfterTx sub-field of the received GP Commissioning Notification command is set to 0b0) .
    AND/OR On reception of ZCL Read Attributes command, DUT-GPS does NOT respond with Read Attributes
      Response with Sink Table having one entry for the TH-GPD with ApplicationID = 0b010 ,
      GPD IEEE address = N, Endpoint = X, and all relevant parameters exchanged in the 
      commissioning process are stored there.
    AND/OR If DUT-GPS supports Translation Table functionality, on receipt of GP Translation Table Request command, 
      DUT-GPS does NOT respond with GP Translation Table Response command, containing default entries for 
      GPD commands corresponding to TH-GPD DeviceID, being generic entries 
      (with ApplicationID = 0b000 and SrcID = 0xffffffff) and/or
      entries for ApplicationID = 0b010, GPD IEEE address = N, GPD Endpoint = X; 
      with Endpoint field set to value other than 0xfd or 0x00 (e.g. 0xff or specific application endpoint).
    AND/OR DUT-GPS does NOT correctly execute the GPD command in the Data GPDF with Endpoint X.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
