Bug 12585 - zero-transition-time problem in Color control cluster commands 
RTP_ZCL_13 - Handling of Color Control commands with zero transition time 

Objective:

	To confirm that the device can handle Color Control commands with zero transition time.

Devices:

	1. TH - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
    2. Power on DUT ZR
    3. Wait for DUT ZR and TH ZC complete finding and binding procedure
	4. Wait for DUT ZR receive Color Control "Move to Hue and Saturation" command and respond with Default Response
    5. Wait for DUT ZR receive Read Attributes command for Hue and Saturation attributes and respond with Default Response
	6. Wait for DUT ZR receive Color Control "Move to Color Temperature" command and respond with Default Response
    7. Wait for DUT ZR receive Read Attributes command for Color Temperature attribute and respond with Default Response

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

	3. TH ZC performs finding and binding (TH ZC as initiator, DUT ZR as target):
       TH ZC binds to the Color Control cluster of DUT ZR.

	4.1. DUT ZR receives ZCL Color Control "Move to Hue and Saturation" command with transition time equal to 0
	4.2. DUT ZR responds with Default Response with the Success status (0x00)

    5.1. DUT ZR receives Read Attributes command for Hue and Saturation attributes
	5.2. DUT ZR responds with Read Attributes Response with two records for Hue and Saturation attributes. 
         Values of these attributes should be equal to values from step 4.1.

	6.1. DUT ZR receives ZCL Color Control "Move to Color Temperature" command with transition time equal to 0
	6.2. DUT ZR responds with Default Response with the Success status (0x00)

    7.1. DUT ZR receives Read Attributes command for Color Temperature attribute
	7.2. DUT ZR responds with Read Attributes Response with record for Color Temperature attribute. 
         Value of this attribute should be equal to value from step 6.1.