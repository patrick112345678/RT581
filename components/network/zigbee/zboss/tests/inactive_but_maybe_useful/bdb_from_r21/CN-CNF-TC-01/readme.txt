4.2.1 CN-CNF-TC-01: Network formation in the presence of an open network, DUT: ZC

This test covers centralized network formation. 
This test verifies the operation of the ZC that forms the centralized network even in the presence of an open network; the other role can be performed by a golden unit or test harness, as specified below.


Required devices:
DUT - ZC, capable of centralized network formation
THc1 - TH ZC, capable of centralized network formation on pre-configured channel(s)
this role can be performed by a golden unit device or a TH
THr1 - TH ZR, capable of joining a centralized network
this role can be performed by a golden unit device or a TH


Test preparation:
P1 - THc1 is powered on and triggered to form a centralized network on a primary channel as supported by the DUT (DUT’s bdbPrimaryChannelSet), or – if bdbPrimaryChannelSet = 0x00000000, on a secondary channel as supported by the DUT (DUT’s bdbSecondaryChannelSet). 
THc1 successfully forms a centralized network. 
THc1’s PANId and short address = 0x0000 are known from THc1’s link status messages.
P2 - DUT is placed at 1m distance from the ZC1.


Test Procedure:
1
Before: Network steering is triggered on the THc1.
Power on DUT.
Network formation is triggered on the DUT.
After: DUT performs a scan to find a suitable channel.

2a
Before: Since THc1’s network was formed on one of the DUT’s channels, THc1 receives a Beacon Request and unicasts to the DUT a Beacon frame with AssociationPermit = TRUE on the channel the THc1 operates on.
After: Despite the reception of a beacon of an open network from THc1, DUT does NOT join the network of THc1, since DUT’s network steering is disabled; DUT does NOT send Association Request to THc1.

2b
After: DUT forms a centralized network:
Wait at most 15 seconds. Within the 15 seconds, DUT broadcasts to 0xfffc a NWK Link Status command, with:
MAC header carrying:
DUT’s PANId is NOT equal to the PANId of THc1;
DUT’s short address = 0x0000;
correctly protected with the network key
source IEEE address is included in the NWK header and carries IEEE address of the DUT; 
Radius = 0x01, 
the sub-fields of the CommandOptions field set to: EntryCount = 0x00, FirstFrame = LastFrame = 0b1, 
and the LinkStatusList field NOT present..
Note: since the NWK key of DUT’s network was NOT sent over the air, the payload of the Link Status command likely cannot be decrypted by the sniffer at this point. It will be verified in step 3b, after a key is delivered OTA.

2c
After: Zigbee PRO/2007 Layer PICS and Stack Profiles, Zigbee Alliance document 08-0006r05 or later. TCC2

For information: DUT starts the Trust Centre operation.
As part of its Trust Centre role, DUT may also start its operation as a concentrator, sending NWK Route Request command, with the Many-to-one option set.
Note: since the NWK key of DUT’s network was NOT sent over the air, the payload of the command likely cannot be decrypted by the sniffer at this point. It will be verified in step 3b, after a key is delivered OTA.

3a
Before: Network steering is triggered on the DUT.
Network steering is triggered on the THr1.
After: THr1 and DUT successfully complete association, incl. NWK key delivery. THr1 does NOT request TC-LK update.
Note: network steering behaviour is tested in detail in sectionCentralized network: network steering. The current step is used only for transporting the NWK key over thae air, so that the messages sent by the DUT upon network formation can be verified.

3b
Before: Verify the presence and the correctness of DUT messages from step 2b and 2c.
1Note: a device has joined the DUT’s network and thus the NWK key was sent over the air, making it possible for the sniffer to decrypt the protected message transmitted by the DUT after network formation. They can be verified now.
After: DUT did send messages of step 2b and 2c, formatted exactly as described there.
