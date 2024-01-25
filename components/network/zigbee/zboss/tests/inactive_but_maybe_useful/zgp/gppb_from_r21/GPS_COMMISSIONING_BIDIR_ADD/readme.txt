Test described in GP test specification, 
    clause 4.4.1 Proximity commissioning
         sub-clause 4.4.1.7 Bidirectional proximity commissioning: additional tests

4.4.1.7.1 Initial Conditions

DUT-GPS (GPT, GPT+, GPC, GPCB)  *PANId= Generated in a random manner (within the range 0x0001 to 0x3FFF)
                                 Logical Address = Generated in a random manner(within the range 0x0000 to 0xFFF7)
                                 IEEE address=manufacturer-specific 
                                 DUT-GPS supports proximity commissioning.
TH-GPS                          *PANId= Generated in a random manner (within the range 0x0001 to 0x3FFF)
                                 Logical Address = Generated in a random manner(within the range 0x0000 to 0xFFF7)
                                 IEEE address=manufacturer-specific 
TH-GPD                           GPD ID = N
                                *GPD security usage according to DUT-GPS capabilities; 
                                 TH-GPD application functionality matches that of the DUT-GPS’s GPEP.
                                 TH-GPD implements bidirectional commissioning procedure.

A packet sniffer shall be observing the communication over the air interface.


4.4.1.7.2 Test Set-Up

The DUT-GPS, TH-GPS and TH-GPD shall be in wireless communication proximity. 
A packet sniffer shall be observing the communication over the air interface. 

4.4.1.7.3 Test Procedure

Negative tests

1A	[R3] A.3.9, A.3.3.5.3, A.3.3.4.4, A.3.3.5.4, A.3.3.5.2, A.3.4.2.2
	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.
	
	TH-GPD sends on the operational channel a correctly formatted GPD Channel Request with:
	•	Auto-Commissioning sub-field of the NWK Frame Control field set to 0b1;
	•	Channel Toggling Behavior field listing a channel other than the operational channel.

Negative test: Channel Request on the operational channel: 
	Asap within the 5s TransmitChannel timeout, make the TH-GPD send Channel Request GPDF 
	on the operational channel, with:
	•	Frame Type sub-field of the NWK Frame Control field set to 0b01,
	•	 Auto-Commissioning = 0b0,
	•	Frame Control Extension = 0b0; and the Extended NWK Frame Control field absent, 
	•	GPD SrcID field, Endpoint, Security frame counter and MIC field absent. 	

1B	[R3] A.1.4.1.2	Handling on Auto-Commissioning flag in GPD Channel Request:
	Clear Sink Table of DUT-GPS.
	
	DUT-GPS enters commissioning mode.
	
	TH-GPD sends on the operational channel a correctly formatted GPD Channel Request with:
	•	Auto-Commissioning sub-field of the NWK Frame Control field set to 0b1;
	•	Channel Toggling Behavior field listing the operational channel.
	
	Within 5s TransmitChannel timeout, TH-GPD sends on the operational channel a 
	    correctly formatted GPD Channel Request with:
	•	Auto-Commissioning sub-field of the NWK Frame Control field set to 0b1;
	•	Channel Toggling Behavior field listing the operational channel.
	
	Within 5s TransmitChannel timeout, TH-GPD sends on the operational channel a 
	    correctly formatted GPD Channel Request with:
	•	Auto-Commissioning sub-field of the NWK Frame Control field set to 0b0;
	•	Channel Toggling Behavior field listing the operational channel.
	
1C      Handling on Auto-Commissioning flag in GPD Channel Request on TransmitChannel:

	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	TH-GPD sends on the operational channel a correctly formatted GPD Channel Request with:
	•	Auto-Commissioning sub-field of the NWK Frame Control field set to 0b1;
	•	Channel Toggling Behavior field listing a channel X other than the operational channel.
	
	Within 5s TransmitChannel timeout, TH-GPD sends on the TransmitChannel X a 
	    correctly formatted GPD Channel Request with:
	•	Auto-Commissioning sub-field of the NWK Frame Control field set to 0b1;
	•	Channel Toggling Behavior field listing channel X.
	
	Within 5s TransmitChannel timeout, TH-GPD sends on the TransmitChannel X a 
	    correctly formatted GPD Channel Request with:
	•	Auto-Commissioning sub-field of the NWK Frame Control field set to 0b0;
	•	Channel Toggling Behavior field listing channel X.	

1D	State test (Channel Request): 

	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	TH-GPD sends on the operational channel a correctly formatted GPD Channel Request with:
	•	Auto-Commissioning sub-field of the NWK Frame Control field set to 0b1;
	•	Channel Toggling Behavior field listing the operational channel.
	
	within the 5s TransmitChannel timeout, TH-GPD sends on the operational channel a 
	    correctly formatted GPD Channel Request with:
	•	Auto-Commissioning sub-field of the NWK Frame Control field set to 0b0;
	•	Channel Toggling Behavior field listing the operational channel.
	
	After DUT-GPS sends GPD Channel Configuration command to the TH-GPD, TH-GPD sends 
	    correctly formatted Channel Request GPDF on the operational channel with Auto-Commissioning = 0b0.	

2A	State test (Commissioning): 

	Clear Sink Table of DUT-GPS.
	
	Set gpSharedSecurityKeyType of DUT-GPS to 0b010, and gpSharedSecurityKey of DUT-GPS to a non-zero value.
	Set gpsCommissioningModeExit to 0x02 (first pairing success).
	
	DUT-GPS enters commissioning mode.
	
	After successful exchange of GPD Channel Request and GPD Channel Configuration between the TH-GPD and DUT-GPS, 
	TH-GPD sends two, spaced at least 1 second apart, GPD Commissioning command with RxAfterTx = 0b1 and 
	Security Key Request sub-field of the Options field set to 0b1 and GPD key encryption sub-field of the 
	Extended Options field set to 0b1, and with application and security settings as supported by the DUT-GPS.
	On receipt of GPD Commissioning Reply, TH-GPD sends Commissioning GPDF, formatted as before.
	
2B	On receipt of GPD Commissioning Reply, TH-GPD sends correctly protected GPD Success command.

	Read out Sink Table entry of the DUT-GPS.

2C		Confirm DUT-GPS exited commissioning mode:

	TH-GPD sends correctly formatted and protected Success GPDF.

Additional tests

3	[R3] A.3.9.1, A.1.4.1	Negative test: reception of Data GPDF carrying Channel Request:
	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.
	
	TH-GPD sends on the operational channel a correctly formatted GPD Channel Request with:
	•	Auto-Commissioning sub-field of the NWK Frame Control field set to 0b1;
	•	Channel Toggling Behavior field listing the operational channel.
	
	Within 5s, TH-GPD sends on the operational channel:
	• Channel Request GPDF, using Frame Type = 0b00 (Data frame), 
	    with Extended NWK Frame Control field absent, SrcID = 0x00000000.

	• Channel Request GPDF, with Frame Type = 0b01 (Maintenance frame), 
	    but using ZigbeeProtocolVersion = 0x2;

	• Channel Request GPDF, with Frame Type = 0b01 (Maintenance frame), 
	    but using NWK Frame Control Extension sub-field set to 0b1; and 
	    with Extended NWK Frame Control field present.

4	Negative test: reception of GPDF other than Channel Request on TransmitChannel:
	    (following step 3, within the commissioning window of DUT-GPS)

	TH-GPD sends on the operational channel a Channel Request GPDF, 
	    listing channel other than the operational channel in both Rx channel 
	    in the (second) next attempt sub-fields.

	Within the TransmitChannel 5s timeout, TH-GPD sends on the TransmitChannel channel:
	•	Channel Configuration GPDF with Auto-Commissioning = 0b0;
	•	Commissioning GPDF with RxAfterTx = 0b1. 
	•	Commissioning GPDF with RxAfterTx = 0b0, 
	•	Success GPDF,
	•	Finally (still within the 5s timeout), Channel Request GPDF with Auto-Commissioning = 0b0. 

***********************************************************************
* NOTE!!! Monitoring on both operational and TransmitChannel required *
***********************************************************************

5	Negative test: reception of Commissioning GPDF with Auto-Commissioning sub-field 0b1:

	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	After successful exchange of GPD Channel Request and GPD Channel Configuration 
	    between the TH-GPD and DUT-GPS, TH-GPD sends a GPD Commissioning command with 
	    RxAfterTx = 0b1 with application and security settings as supported by the DUT-GPS.

	Then, TH-GPD sends on the operational channel:
	• and then a Commissioning GPDF, with Auto-Commissioning sub-field set to 0b1 and RxAfterTx sub-field set to 0b1,
	• and then a Commissioning GPDF, with Auto-Commissioning sub-field set to 0b1 and RxAfterTx sub-field set to 0b0,
	• and then a Success GPDF, with Auto-Commissioning sub-field set to 0b1 and RxAfterTx sub-field set to 0b0,
	• and finally a Commissioning GPDF, with Auto-Commissioning sub-field set to 0b0 and 
	    RxAfterTx sub-field set to 0b1,

	Incorrectly protected Success GPDF (if sink supports security)

6A	Negative test: GPD Success incorrectly protected:

	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	After exchange of GPD Channel Request and GPD Channel Configuration commands, 
	TH-GPD and DUT-GPS sends on the operational channel of DUT-GPS a Commissioning 
	GPDF ApplicationID = 0b010, with all settings as supported by DUT-GPS.

	DUT-GPS responds with GPD Commissioning Reply, with the settings as requested by the TH-GPD.
	TH-GPD sends unprotected GPD Success.

	Read out Sink Table of DUT-GPS.

6B	Negative test: GPD Success incorrectly protected:
    NOTE!!! Immediately after 6A, within commissioning window of DUT-GPS

	TH-GPD sends GPD Success protected with frame counter value lower than 
	that of the last GPD Commissioning command.

	Read out Sink Table of DUT-GPS.


6C	Negative test: GPD Success incorrectly protected:
    NOTE!!! Immediately after 6B, within commissioning window of DUT-GPS

	TH-GPD sends GPD Success protected correctly, but with incorrect key type (individual vs. shared)
	    indicted in the NWK Extended FC.

	Read out Sink Table of DUT-GPS.
	

6D	Negative test: GPD Success incorrectly protected:
    NOTE!!! Immediately after 6C, within commissioning window of DUT-GPS

	TH-GPD sends GPD Success protected correctly, but with incorrect key value (i.e. wrong MIC).

	Read out Sink Table of DUT-GPS.


6E	Negative test: GPD Success incorrectly protected:
    NOTE!!! Immediately after 6D, within commissioning window of DUT-GPS

	TH-GPD sends GPD Success protected correctly, but sent with incorrect GPD ID value (!=N).

	Read out Sink Table of DUT-GPS.


6F	[R3] A.1.7.5, A.3.9.1, A.4.2.1.2
	Negative test: Not accepting reset frame counter in Success GPDF:
    NOTE!!! Immediately after 6E, within commissioning window of DUT-GPS

	TH-GPD sends GPD Success command correctly protected, but with reset
	SecurityFrameCounter (if SecurityLevel 0b10 or 0b11) value 0x00000000. 

	Read out Sink Table entry of the DUT-GPS.

6G	Negative test: Unexpected GPD Success:

	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	TH-GPD sends GPD Success, correctly protected with the GPD shared key.

	Read out Sink Table of DUT-GPS.

6H	Negative test: GPD Success incorrectly protected, ApplicationID = 0b010:

	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode. Perform commissioning between DUT-GPS and TH-GPD.
	Upon reception of the GPD Commissioning Reply command, with the settings as 
	requested by the TH-GPD, TH-GPD sends GPD Success protected correctly, 
	but with incorrect key value (i.e. wrong MIC).

	Read out Sink Table of DUT-GPS.

6I	TH-GPD sends GPD Success protected correctly, but with incorrect GPD IEEE address value (!=N).

	Read out Sink Table of DUT-GPS.

6J	TH-GPD sends GPD Success protected correctly, but with incorrect Endpoint Y (!=X, !=0x00, !=0xff).

	Read out Sink Table of DUT-GPS.
	Conditional on DUT-GPS NOT being capable of immediate response (PICS item GPSF2 NOT supported): 

TempMaster behavior

7A		Clear Sink Table of DUT-GPS.
	DUT-GPS enters commissioning mode.
	TH-GPD sends GPD Channel Request, indicating TxChannel = operational channel.
	Within 5s, TH-GPS broadcasts a GP Response for SrcID = 0x00000000, carrying 
	    GPD Channel Configuration, with TempMasterShortAddress NOT equal to the network address of DUT-GPS.
	Make TH-GPD send correctly formatted GPD Channel Request on the operational channel.

7B	Clear Sink Table of DUT-GPS.
	
	DUT-GPS enters commissioning mode.

	TH-GPD sends GPD Channel Request, indicating TxChannel = operational channel.
	
	Within 5s, TH-GPS broadcasts a GP Response for SrcID != 0x00000000, carrying GPD 
	    Channel Configuration, and with TempMasterShortAddress NOT equal to the network address of DUT-GPS.
	
	Make TH-GPD send correctly formatted GPD Channel Request.

7C	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	After successful exchange of GPD Channel Request and GPD Channel Configuration between 
	the TH-GPD and DUT-GPS, TH-GPD sends a GPD Commissioning command with RxAfterTx = 0b1 
	with application and security settings as supported by the DUT-GPS.

	Within 5s, TH-GPS broadcasts a GP Response, for GPD ID of TH-GPD, but with 
	TempMasterShortAddress NOT equal to the network address of DUT-GPS.

	Make TH-GPD send correctly formatted GPD Commissioning with RxAfterTx = 0b1.

7D	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	After successful exchange of GPD Channel Request and GPD Channel Configuration
	between the TH-GPD and DUT-GPS, TH-GPD sends a GPD Commissioning command with 
	RxAfterTx = 0b1 with application and security settings as supported by the DUT-GPS.

	Within 5s, TH-GPS broadcasts a GP Response, for GPD ID NOT equal to that of TH-GPD,
	and with TempMasterShortAddress NOT equal to the network address of DUT-GPS.

	Make TH-GPD send correctly formatted GPD Commissioning with RxAfterTx = 0b1.

TempMaster timing

8A	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	TH-GPD sends GPD Channel Request on the operational channel, indicating TxChannel = X 
	NOT equal to the operational channel.

	At approx. 4.8 seconds after transmission of the first GPD Channel Request, 
	make TH-GPD send correctly formatted GPD Channel Request on channel X.

8B	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	TH-GPD sends GPD Channel Request on the operational channel, indicating TxChannel = X 
	NOT equal to the operational channel.

	At approx. 5.1 seconds after transmission of the GP Response, make TH-GPD send correctly
	formatted GPD Channel Request on channel X.

8C	Conditional on DUT-GPS NOT being capable of immediate response (PICS item GPSF2 NOT supported): 

	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	TH-GPD sends GPD Channel Request, indicating TxChannel = X NOT equal to the operational channel.

	At approx. 5.1 seconds after transmission of the GP Response, make TH-GPD send correctly 
	formatted GPD Channel Request on the operational channel; the MAC sequence number is 
	different than that in the previous Channel Request GPDF.

	No possibility to establish a security key:

9A	Negative test: No possibility to establish a security key:

	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode.

	After exchange of GPD Channel Request and GPD Channel Configuration commands,
	TH-GPD sends on the operational channel of DUT-GPS a Commissioning GPDF with all 
	settings as supported by DUT-GPS, only with SecurityLevelCapabilities sub-field of 
	the Extended Options field set to >=0b10, the GPD Key present sub-field of the 
	Extended Options field set to 0b0, the GP security Key request sub-field of the 
	Options field set to 0b0, and the GPD Key field absent.

	Read out Sink Table of DUT-GPS.

	If DUT-GPS supports Translation Table functionality, send GP Translation Table request to DUT-GPS.

GPD ID = 0x00..00

10A	Negative test: SrcID = 0x00000000; 

	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode. Perform commissioning between DUT-GPS and TH-GPD.

	Upon reception of the GPD Channel Configuration command, TH-GPD sends GPD Commissioning 
	command, with all settings as required by the DUT-GPS, but with ApplicationID = 0b000 and 
	with SrcID set to 0x00000000, and then another one, after ~2sec.

10B	Negative test: IEEE = 0x0000000000000000; 

	Clear Sink Table of DUT-GPS.

	DUT-GPS enters commissioning mode. Perform commissioning between DUT-GPS and TH-GPD.

	Upon reception of the GPD Channel Configuration command, TH-GPD sends GPD Commissioning 
	command, will all settings as required by the DUT-GPS, but with ApplicationID = 0b010 
	and with MAC header Source address set to 0x0000000000000000, and then another one, after ~2sec.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log

