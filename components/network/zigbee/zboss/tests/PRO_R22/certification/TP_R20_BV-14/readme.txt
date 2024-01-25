10.68 TP/R20/BV-14 NWK Unknown NWK Protocol Version (CCB 1361)
Objective: To confirm that a device will correctly ignore NWK frames with an unknown protocol version.

gZC
DUT ZR (or DUT ZED)

gZC:
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = 0x0000
Long Address =
0x AAAA AAAA AAAA AAAA

DUT:
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = Generated in a random manner within the range 1 to 0xFFF7
Long Address = 0x0000 0000 0000 0001

Initial conditions:
1. Reset all nodes
2. Set ZC under target stack profile, gZC as coordinator
starts a PAN = 0x1AAA network; it is the Trust Center for the PAN
3. ZR/ZED is configured with
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE

Note: Since either the ZC or the ZR/ZED can be the DUT, the test procedure is left mostly generic.
Steps that require specifically the DUT or golden unit, are marked as such.

Test procedure:
1. Join device ZR/ZED to gZC.
2. If end device is in use it shall be an RxOnWhenIdle=TRUE end device.
3. If the DUT does NOT support the ZigBee Greenpower protocol, execute the following step: 
Golden unit sends a NWK data message with nwkProtocol = 0x03. Payload is all 0’s.
4. The Golden unit sends a unicast buffer test request message to the DUT using nwkProtocol = 0x02 (Zigbee Pro).
5. Golden unit sends a NWK data message with nwkProtocol = 0x04. Payload is all 0’s.
6. The Golden unit sends a unicast buffer test request message to the DUT using nwkProtocol=0x02 (Zigbee Pro).

Pass verdict:
1. ZR/ZED shall issue an MLME Beacon Request MAC command frame, and ZC shall respond with a beacon.
2. ZR/ZED is able to complete the MAC association sequence with ZC and gets a new short address, randomly generated.
3. ZR/ZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
4. DUT does not respond to the message, and does not crash.
5. The DUT responds to the buffer test request message with a buffer test response.
6. DUT does not respond to the message and does not crash.
7. The DUT responds to the buffer test request message with a buffer test response.

Fail verdict:
1. ZR/ZED does not issue an MLME Beacon Request MAC command frame, or ZC does not respond with a beacon.
2. ZR/ZED is not able to complete the MAC association sequence.
3. ZR/ZED does not issue a device announcement.
4. DUT responds to the message, or DUT crashes.
5. The DUT does not respond to the buffer test request with a buffer test response. 
6. DUT responds to the message, or DUT crashes.
7. The DUT does not respond to the buffer test request with a buffer test response.







