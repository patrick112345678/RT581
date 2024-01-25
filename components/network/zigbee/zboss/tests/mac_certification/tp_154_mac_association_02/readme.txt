
Test TP/154/MAC/ASSOCIATION-02

Check D.U.T. (as coordinator) correctly receives association request frame
and generates negative response (PanAtCapacity, PanAccessDenied)

Tester transmits MAC association request command frame .
Pass:
DUT returns MAC Primitive:
MLME?ASSOCIATE.indication(
DeviceAddress=0xACDE480000000002,
CapabilityInformation=0x80,
SecurityLevel = 0x00,
KeyIdMode = xx,
KeySource = xx,
KeyIndex = xx
)
Fail:
DUT does not return MLME?ASSOCIATE.indication primitive

Tester will transmit MAC data request command frame to coordinator, DUT
Pass:
It is possible to verify by a PAN analyzer that an ACK frame was issued from
the DUT with the frame pending field set to 1.
Frame Length: 5 bytes
IEEE 802.15.4
Frame Control: 0x0012
.... .... .... .010 = Frame Type: ack (0x0002)
.... .... .... 0... = Security Enabled: Disabled
.... .... ...1 .... = Frame Pending: data
.... .... ..0. .... = Acknowledgment Request: Ack not required
.... .... .0.. .... = Intra?PAN: Not within the PAN
.... ..00 0... .... = Reserved
.... 00.. .... .... = Destination Addressing Mode: None
..00 .... .... .... = Reserved
00.. .... .... .... = Source Addressing Mode: None
Sequence Number: xx
Frame Check Sequence: Correct
Pass:
It is possible to verify by a PAN analyzer that an association response MAC
command frame was issued from the DUT.
Frame Length: 27 bytes
IEEE 802.15.4
Frame Control: 0xCC63
.... .... .... .011 = Frame Type: Command (0x0003)
.... .... .... 0... = Security Enabled: Disabled
.... .... ...0 .... = Frame Pending: No more data
.... .... ..1. .... = Acknowledgment Request: Ack required
.... .... .1.. .... = Intra?PAN: Within the PAN
.... ..00 0... .... = Reserved
.... 11.. .... .... = Destination Addressing Mode: Address field contains a 64-bit extended address
..00 .... .... .... = Reserved
11.. .... .... .... = Source Addressing Mode: Address field contains a 64-bit extended address
Sequence Number: xx
Destination PAN Identifier: 0x1aaa
Destination Address: 0xACDE480000000002
Source Address: 0xACDE480000000001
MAC Payload
MAC Command = 0x02 = Association Response
ShortAddress = 0xffff,
AssociationStatus = 0x0x1 = PAN at capacity
Frame Check Sequence: Correct
Tester will transmit Ack frame in response to association response MAC
command frame from DUT
The DUT shall return a Success result, such that:
DUT returns MAC primitive
MLME-COMM-STATUS.indication(
PANid = 0x1aaa,
SrcAddrMode = 0x03 = ExtendedAddress,
SrcAddr = 0xACDE480000000001,
DstAddrMode = 0x03= ExtendedAddress,
DstAddr = 0xACDE480000000002,
Status = 0x00 = SUCCESS,
SecurityLevel = 0x00,
KeyIdMode = xx,
KeySource = xx,
KeyIndex = xx
)
Fail:
DUT does not send ACK in response to MAC data request command frame
ACK does not have Frame pending field set
DUT does not send association response frame
DUT does not return MLME-COMM-STATUS.indication primitive
Status is not SUCCESS

