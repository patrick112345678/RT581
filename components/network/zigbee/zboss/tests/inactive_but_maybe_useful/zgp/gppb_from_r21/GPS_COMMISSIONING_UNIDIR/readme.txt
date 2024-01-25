Test described in GP test specification, 
    clause 4.4.1 Proximity commissioning 
	subclause 4.4.1.2 Unidirectional in-range commissioning

Test Harnes GPD   - as ZGPD
DUT GPS           - GPS as ZC

dut_gps starts as ZC

Initial Conditions:
DUT-GPS: PANId= Generated in a random manner (within the range 0x0001 to 0x3FFF)
	 Logical Address = Generated in a random manner(within the range 0x0000 to 0xFFF7)
	 IEEE address=manufacturer-specific 
	 DUT-GPS supports proximity commissioning.
TH-GPD:  GPD ID = N
	 GPD security usage according to DUT-GPS capabilities; 
	 TH-GPD application functionality matches that of the DUT-GPS’s GPEP.
	 TH-GPD implements bidirectional commissioning procedure.

- No pairing exists between DUT-GPS and TH-GPD.
- TH-GPD uses ApplicationID = 0b000 and SrcID = N.
- If GPS supports security, TH-GPD shall request the shared key and shall provide OOB key; 
    TC-LK encryption shall be used. Initially, TH-GPD has no shared key.
- If GPS supports security, TH-Tool sets the 
    gpSharedSecurityKeyType attribute of the DUT-GPS to 0b010 and the 
    gpSharedSecurityKey attribute to { 0xc0 0xc1 0xc2 0xc3 0xc4 0xc5 0xc6 0xc7 0xc8 0xc9 0xca 0xcb 0xcc 0xcd 0xce 0xcf }.
- If GPS supports security, TH-GPD shall provide OOB key TC-LK encryption shall be used.

The DUT-GPS and TH-GPD shall be in wireless communication proximity. 
A packet sniffer shall be observing the communication over the air interface. 

4.4.1.2 Test procedure
commands sent by TH-gpd:

1: 
- DUT-GPS enters commissioning mode.
- TH-GPD is triggered to send the Commissioning GPDF with:
    MAC Sequence number having a value unrelated to the value of the 
     GPD outgoing counter in the Commissioning command payload; 
    sub-fields of the NWK Frame Control field set to: Frame type = 0b00, Auto-Commissioning = 0b0, 
    the Extended NWK Frame Control field present and its 
     RxAfterTx sub-field set to 0b0, and all other fields set as required by the DUT-GPS.
    If DUT-GPS supports security, the payload of the GPD Commissioning command 
     carries OOB key (KeyType 0b100), encrypted.
    If required, DUT-GPS is put back into operational mode.

  Read out Sink Table entry of the DUT-GPS.

  If DUT-GPS supports Translation Table functionality, send GP Translation Table request to DUT-GPS.

Pass verdict:
1: DUT-GPS may send GP Proxy Commissioning Mode command (Action=Enter).
   On reception of Commissioning GPDF, DUT-GPS:
    - does NOT send a Commissioning Reply (since RxAfterTx of the received Commissioning GPDF is set to 0b0) ;  
    - provides success indication, 
    - sends out GP Pairing with:
    - APS header fields:
    - ProfileID: 0xA1E0 ;
    - ClusterID: 0x0021;
    - source endpoint: GPEP (242);
    - destination endpoint, if present: GPEP (242);
    - ZCL header fields:
    - Command Identifier: 0x01;
    - The sub-fields of the Frame Control field set to: 
	Frame type = 0b01 (cluster-specific command), 
	Manufacturer specific = 0b0, 
	Direction = 0b1 (server to client), 
	Disable default response = 0b1;
       ZCL payload fields:
	- Sub-fields of the Options field set to: 
	- ApplicationID = as supported by TH-GPD, 
	- AddSink=0b1, RemoveGPD = 0b0;
	- Communication Mode: set according to DUT-GPS capabilities
	- GPD Fixed and GPD MAC sequence number capabilities – copied from the GPD Commissioning command payload;
	 SecurityLevel as exchanged during commissioning;
	 SecurityKeyType as exchanged during commissioning (if DUT-GPS supports security: 0b100);
	 Assigned Alias present and Forwarding Radius present: set according to DUT-GPS capabilities;
	- GPD ID and Endpoint fields according to the ApplicationID of TH-GPD;
	- Sink IEEE address and Sink NWK address fields:
	    present if Communication Mode sub-field of the Options field was set to 0b00 or 0b11; 
	    absent otherwise;
	- Sink GroupID field:
	    present and set to derived alias, if Communication Mode sub-field of the Options field was set to 0b01;
	    present and set any value, if Communication Mode sub-field of the Options field was set to 0b10;
	    absent otherwise;
	- DeviceID field - copied from the GPD Commissioning command payload;
	- GPD security Frame Counter field – as in GPD Commissioning command;
	- GPD key field:
	    present and carrying TH-GPD’s OOB key in the clear, if SecurityLevel >= 0b10;
	    absent otherwise;
	- Assigned alias field:
	    present and set to any value if Assigned Alias present sub-field of the Options field was set to 0b1; 
	    absent otherwise;
	- Forwarding Radius field:
	    present and set to any value <= 0x1E if Forwarding Radius present sub-field of the Options field was set to 0b1; 
	    absent otherwise.
     if DUT-GPS sets CommunicationMode sub-field of the Options field of the GP Pairing command 
      to value other than 0b11 (lightweight unicast): 
        sends out Device_annce for the alias (assigned, if it was included in the GP Pairing command, 
        and else the derived), with NWK source address equal to the alias and NWK sequence number = 0x00;
     if DUT-GPS set CommunicationMode sub-field of the Options field of the GP Pairing command 
      to 0b11 (lightweight unicast), 
        Device_annce SHALL not be sent. On reception of ZCL Read Attributes command, 
        DUT-GPS responds with Read Attributes Response with Sink Table having one entry 
        for the TH-GPD GPD ID N and all relevant parameters exchanged in the commissioning 
        process are stored there.

     If DUT-GPS supports Translation Table functionality, on receipt of GP Translation Table Request command, 
     DUT-GPS responds with GP Translation Table Response command, containing default entries for GPD commands 
     corresponding to TH-GPD DeviceID, being generic entries (with ApplicationID = 0b000 and SrcID = 0xffffffff) 
     and/or entries for GPD ID = Z and the corresponding ApplicationID; 
     with Endpoint field set to value other than 0xfd or 0x00 (e.g. 0xff or specific application endpoint).

Fail verdict:
1:  On reception of Commissioning GPDF with RxAfterTx = 0b0, DUT-GPS:
    - does send Commissioning Reply GPDF (despite RxAfterTx of the received Commissioning GPDF being set to 0b0),
    - does not provide success indication, 
    - AND/OR does not send out GP Pairing for TH-GPD’s GPD ID with AddSink=0b1, RemoveGPD = 0b0, appropriate 
	communication mode and security level for the DUT-GPS, and other parameters as derived from the 
	Commissioning GPDF (as indicated in 4.4.1.2.4/pass verdict 1 above);
    - AND/OR:
	- In case of Communication Mode != 0b11: does not send out Device_annce for the alias (assigned,
	  if it was included in the GP Pairing command, and else the derived), with NWK source address
	  equal to the alias, NWK sequence number = 0x00  and APS Counter = 0x00;
	- In case of Communication Mode = 0b11: does send Device_annce for the alias. 
    - AND/OR On reception of ZCL Read Attributes command, DUT-GPS does not respond with Read Attributes 
      Response with Sink Table having one entry for the TH-GPD GPD ID N and all relevant parameters 
      established in the commissioning process.
    - AND/OR DUT-GPS supporting Translation Table functionality does not have Translation Table entries,
      as described in 4.4.1.2.4/item 1 above.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
