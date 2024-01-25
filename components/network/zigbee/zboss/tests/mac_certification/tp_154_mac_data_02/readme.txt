
Test TP/154/MAC/DATA-02

DUT as coordinator shall set up a non-beacon enabled PAN, and DUT and Tester shall be associated.
Follow the steps to association in TP/154/MAC/ASSOCIATION?01; Tester shall associate with capability 0x80

Tests with DUT as coordinator for Intra-PAN, and Tester as device associated to DUT. Perform the following tests:
1. Tester to D.U.T.: Short Address to Short Address, Unicast, no ACK.
   Pass:
   DUT returns MAC Primitive:
   MCPS-DATA.indication(
   SrcAddrMode = 0x02 = ShortAddress,
   SrcPANId =0x1aaa,
   SrcAddr= 0x3344,
   DstAddrMode=0x02 = ShortAddress,
   DstPANId = 0x1aaa,
   DstAddr = 0x1122,
   msduLength=5,
   msdu = 0x00 0x01 0x02 0x03 0x04,
   mpduLinkQuality = xx,
   DSN = xx,
   Timestamp = xx xx xx,
   SecurityLevel = 0x00,
   KeyIdMode = xx,
   KeySource = xx,
   KeyIndex = xx
   )
   Fail:
   DUT transmits ACK frame
   DUT does not return MCPS-DATA.indication primitive
   
2. Tester to D.U.T.: Short Address to Short Address, Unicast, with ACK .
   Pass:
   DUT transmits ACK frame
   DUT returns MAC Primitive:
   MCPS-DATA.indication(
   SrcAddrMode = 0x02 = ShortAddress,
   SrcPANId =0x1aaa,
   SrcAddr= 0x3344,
   DstAddrMode=0x02 = ShortAddress,
   DstPANId = 0x1aaa,
   DstAddr = 0x1122,
   msduLength=5,
   msdu = 0x00 0x01 0x02 0x03 0x04,
   mpduLinkQuality = xx,
   DSN = xx,
   Timestamp = xx xx xx,
   SecurityLevel = 0x00,
   KeyIdMode = xx
   KeySource = xx,
   KeyIndex = xx
   )
   Fail:
   DUT does not transmit ACK frame
   DUT transmits ACK frame at incorrect time
   DUT does not return MCPS-DATA.indication primitive

3. Tester to D.U.T.: Short Address to Extended Address, Unicast with ACK .
   Pass:
   DUT transmits ACK frame
   DUT returns MAC Primitive:
   MCPS-DATA.indication(
   SrcAddrMode = 0x02 = ShortAddress,
   SrcPANId =0x1aaa,
   SrcAddr= 0x3344,
   DstAddrMode=0x03 = ExtendedAddress,
   DstPANId = 0x1aaa,
   DstAddr = 0xACDE480000000001,
   msduLength=5,
   msdu = 0x00 0x01 0x02 0x03 0x04,
   mpduLinkQuality = xx,
   DSN = xx,
   Timestamp = xx xx xx,
   SecurityLevel = 0x00,
   KeyIdMode = xx,
   KeySource = xx,
   KeyIndex = xx
   )
   Fail:
   DUT does not return MCPS-DATA.indication primitive
   
4. Tester to D.U.T.: Extended Address to Short Address , Unicast with ACK.
   Pass:
   DUT transmits ACK frame
   DUT returns MAC Primitive:
   MCPS-DATA.indication(
   SrcAddrMode = 0x03 = ExtendedAddress,
   SrcPANId =0x1aaa,
   SrcAddr= 0xACDE480000000002,
   DstAddrMode=0x02 = ShortAddress,
   DstPANId = 0x1aaa,
   DstAddr =0x1122,
   msduLength=5,
   msdu = 0x00 0x01 0x02 0x03 0x04,
   mpduLinkQuality = xx,
   DSN = xx,
   Timestamp = xx xx xx,
   SecurityLevel = 0x00,
   KeyIdMode = xx,
   KeySource = xx,
   KeyIndex = xx
   )
   Fail:
   DUT does not send ACK
   DUT does not send ACK at correct time
   DUT does not return MCPS-DATA.indication primitive

5. Tester to D.U.T.: Extended Address to Extended Address , Unicast with ACK.
   Pass:
   DUT transmits ACK frame
   DUT returns MAC Primitive:
   MCPS-DATA.indication(
   SrcAddrMode = 0x03 = ExtendedAddress,
   SrcPANId =0x1aaa,
   SrcAddr= 0xACDE480000000002,
   DstAddrMode=0x03 = ExtendedAddress,
   DstPANId = 0x1aaa,
   DstAddr =0xACDE480000000001,
   msduLength=5,
   msdu = 0x00 0x01 0x02 0x03 0x04,
   mpduLinkQuality = xx,
   DSN = xx,
   Timestamp = xx xx xx,
   SecurityLevel = 0x00,
   KeyIdMode = xx,
   KeySource = xx,
   KeyIndex = xx
   )
   Fail:
   DUT does not send ACK
   DUT does not send ACK at correct time
   DUT does not return MCPS-DATA.indication primitive

6. Tester to D.U.T.: Extended Address to Broadcast Address.
   Pass:
   DUT returns MAC Primitive:
   MCPS-DATA.indication(
   SrcAddrMode = 0x03 = ExtendedAddress,
   SrcPANId =0x1aaa,
   SrcAddr= 0xACDE480000000002,
   DstAddrMode=0x02 = ShortAddress,
   DstPANId = 0x1aaa,
   DstAddr =0xffff,
   msduLength=5,
   msdu = 0x00 0x01 0x02 0x03 0x04,
   mpduLinkQuality = xx,
   DSN = xx,
   Timestamp = xx xx xx,
   SecurityLevel = 0x00,
   KeyIdMode = xx,
   KeySource = xx,
   KeyIndex = xx
   )
   Fail:
   DUT sends ACK frame
   DUT does not return MCPS-DATA.indication primitive
   
7. Tester to D.U.T.: Short Address to Broadcast Address.
   Pass:
   DUT transmits ACK frame
   DUT returns MAC Primitive:
   MCPS-DATA.indication(
   SrcAddrMode = 0x02 = ShortAddress,
   SrcPANId =0x1aaa,
   SrcAddr= 0x3344,
   DstAddrMode=0x02 = ShortAddress,
   DstPANId = 0x1aaa,
   DstAddr =0xffff,
   msduLength=5,
   msdu = 0x00 0x01 0x02 0x03 0x04,
   mpduLinkQuality = xx,
   DSN = xx,
   Timestamp = xx xx xx,
   SecurityLevel = 0x00,
   KeyIdMode = xx,
   KeySource = xx,
   KeyIndex = xx
   )
   Fail:
   DUT does not send ACK
   DUT does not send ACK at correct time
   DUT does not return MCPS-DATA.indication primitive

8. D.U.T. to tester: Short Address to Short Address, Indirect, with ACK.
   Fail:
   DUT returns MCPS-DATA.confirm primitive
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
   ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
   00.. .... .... .... = Source Addressing Mode: None
   Sequence Number: xx
   Frame Check Sequence: Correct
   Pass:
   It is possible to verify by a PAN analyzer that a data frame was issued from the DUT.
   Frame Length: 16 bytes
   IEEE 802.15.4
   Frame Control: 0x8861
   .... .... .... .001 = Frame Type: Data (0x0001)
   .... .... .... 0... = Security Enabled: Disabled
   .... .... ...0 .... = Frame Pending: No more data
   .... .... ..1. .... = Acknowledgment Request: Ack required
   .... .... .1.. .... = Intra-PAN: Within the PAN
   .... ..00 0... .... = Reserved
   .... 10.. .... .... = Destination Addressing Mode: Address field contains a 16-bit short address
   ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
   10.. .... .... .... = Source Addressing Mode: Address field contains a 16-bit short address
   Sequence Number: xx
   Destination PAN Identifier: 0x1aaa
   Destination Address: 0x3344
   Source Address: 0x1122
   MAC Payload
   0x00 0x01 0x02 0x03 0x04
   Frame Check Sequence: Correct
   The Tester will transmit an ACK frame
   The DUT shall return a Success result, such that:
   DUT returns MAC primitive
   MPCS-DATA.confirm(
   msduHandle = 0x0c,
   status = 0x00 = SUCCESS,
   Timestamp = xx xx xx
   )
   Fail:
   DUT does not send ACK in response to MAC data request command frame
   ACK does not have Frame pending field set
   DUT does not send data frame
   DUT does not return MCPS-DATA.confirm primitive

9 .D.U.T. to tester: Short Address to Extended Address, Indirect, with ACK.
  Fail:
  DUT returns MCPS-DATA.confirm primitive
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
  .... .... .0.. .... = Intra-PAN: Not within the PAN
  .... ..00 0... .... = Reserved
  .... 00.. .... .... = Destination Addressing Mode: None
  ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
  00.. .... .... .... = Source Addressing Mode: None
  Sequence Number: xx
  Frame Check Sequence: Correct
  Pass:
  It is possible to verify by a PAN analyzer that a data frame was issued from the DUT.
  Frame Length: 22 bytes
  IEEE 802.15.4
  Frame Control: 0x8C61
  .... .... .... .001 = Frame Type: Data (0x0001)
  .... .... .... 0... = Security Enabled: Disabled
  .... .... ...0 .... = Frame Pending: No more data
  .... .... ..1. .... = Acknowledgment Request: Ack required
  .... .... .1.. .... = Intra-PAN: Within the PAN
  .... ..00 0... .... = Reserved
  .... 11.. .... .... = Destination Addressing Mode: Address field contains a 64-bit extended address
  ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
  10.. .... .... .... = Source Addressing Mode: Address field contains a 16-
  bit short address
  Sequence Number: xx
  Destination PAN Identifier: 0x1aaa
  Destination Address: 0xACDE480000000002
  Source Address: 0x1122
  MAC Payload
  0x00 0x01 0x02 0x03 0x04
  Frame Check Sequence: Correct
  The Tester will transmit an ACK frame
  The DUT shall return a Success result, such that:
  DUT returns MAC primitive
  MPCS-DATA.confirm(
  msduHandle = 0x0c,
  status = 0x00 = SUCCESS,
  Timestamp = xx xx xx
  )
  Fail:
  DUT does not send ACK in response to MAC data request command frame
  ACK does not have Frame pending field set
  DUT does not send data frame
DUT does not return MCPS-DATA.confirm primitive


10. D.U.T. to tester: Extended Address to Short Address, Indirect, with ACK.
    Fail:
    DUT returns MCPS-DATA.confirm primitive
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
    .... .... .0.. .... = Intra-PAN: Not within the PAN
    .... ..00 0... .... = Reserved
    .... 00.. .... .... = Destination Addressing Mode: None
    ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
    00.. .... .... .... = Source Addressing Mode: None
    Sequence Number: xx
    Frame Check Sequence: Correct
    Pass:
    It is possible to verify by a PAN analyzer that a data frame was issued from the DUT.
    Frame Length: 22 bytes
    IEEE 802.15.4
    Frame Control: 0xC861
    .... .... .... .001 = Frame Type: Data (0x0001)
    .... .... .... 0... = Security Enabled: Disabled
    .... .... ...0 .... = Frame Pending: No more data
    .... .... ..1. .... = Acknowledgment Request: Ack required
    .... .... .1.. .... = Intra-PAN: Within the PAN
    .... ..00 0... .... = Reserved
    .... 10.. .... .... = Destination Addressing Mode: Address field contains a 16 bit short address
    ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
    11.. .... .... .... = Source Addressing Mode: Address field contains a 64-
    bit extended address
    Sequence Number: xx
    Destination PAN Identifier: 0x1aaa
    Destination Address: 0xACDE480000000002
    Source Address: 0xACDE480000000001
    MAC Payload
    0x00 0x01 0x02 0x03 0x04
    Frame Check Sequence: Correct
    The Tester will transmit an ACK frame
    The DUT shall return a Success result, such that:
    DUT returns MAC primitive
    MPCS-DATA.confirm(
    msduHandle = 0x0c,
    status = 0x00 = SUCCESS,
    Timestamp = xx xx xx
    )
    Fail:
    DUT does not send ACK in response to MAC data request command frame
    ACK does not have Frame pending field set
    DUT does not send data frame
    DUT does not return MCPS-DATA.confirm primitive


11. D.U.T. to tester: Extended Address to Extended Address, Indirect, with ACK.
    Fail:
    DUT returns MCPS-DATA.confirm primitive
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
    .... .... .0.. .... = Intra-PAN: Not within the PAN
    .... ..00 0... .... = Reserved
    .... 00.. .... .... = Destination Addressing Mode: None
    ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
    00.. .... .... .... = Source Addressing Mode: None
    Sequence Number: xx
    Frame Check Sequence: Correct
    Pass:
    It is possible to verify by a PAN analyzer that a data frame was issued from the DUT.
    Frame Length: 28 bytes
    IEEE 802.15.4
    Frame Control: 0xCC61
    .... .... .... .001 = Frame Type: Data (0x0001)
    .... .... .... 0... = Security Enabled: Disabled
    .... .... ...0 .... = Frame Pending: No more data
    .... .... ..1. .... = Acknowledgment Request: Ack required
    .... .... .1.. .... = Intra-PAN: Within the PAN
    .... ..00 0... .... = Reserved
    .... 11.. .... .... = Destination Addressing Mode: Address field contains a 64-bit extended address
    ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
    11.. .... .... .... = Source Addressing Mode: Address field contains a 64-bit extended address
    Sequence Number: xx
    Destination PAN Identifier: 0x1aaa
    Destination Address: 0xACDE480000000002
    Source Address: 0xACDE480000000001
    MAC Payload
    0x00 0x01 0x02 0x03 0x04
    Frame Check Sequence: Correct
    The Tester will transmit an ACK frame
    The DUT shall return a Success result, such that:
    DUT returns MAC primitive
    MPCS-DATA.confirm(
    msduHandle = 0x0c,
    status = 0x00 = SUCCESS,
    Timestamp = xx xx xx
    )
    Fail:
    DUT does not send ACK in response to MAC data request command frame
    ACK does not have Frame pending field set
    DUT does not send data frame
    DUT does not return MCPS-DATA.confirm primitive

12. D.U.T. to tester: Short Address to Short Address, Indirect, with ACK (Tester does NOT poll, hence transaction expires).
    Fail:
    DUT returns MCPS-DATA.confirm primitive with Status = 0x00 = SUCCESS
    Pass:
    DUT returns MAC primitive
    MPCS-DATA.confirm(
        msduHandle = 0x0c,
        status = 0xf0 = TRANSACTION_EXPIRED
        Timestamp = xx xx xx
        )
        
    Tester will transmit MAC data request command frame to coordinator, DUT
    Two possible pass conditions are valid:
    Pass 1:
    It is possible to verify by a PAN analyzer that an ACK frame was issued from
    the DUT with the frame pending field set to 0.
    Frame Length: 5 bytes
    IEEE 802.15.4
    Frame Control: 0x0002
    .... .... .... .010 = Frame Type: ack (0x0002)
    .... .... .... 0... = Security Enabled: Disabled
    .... .... ...0 .... = Frame Pending: no data
    .... .... ..0. .... = Acknowledgment Request: Ack not required
    .... .... .0.. .... = Intra-PAN: Not within the PAN
    .... ..00 0... .... = Reserved
    .... 00.. .... .... = Destination Addressing Mode: None
    ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
    00.. .... .... .... = Source Addressing Mode: None
    Sequence Number: xx
    Frame Check Sequence: Correct
    Pass 2:
    It is possible to verify by a PAN analyzer that an ACK frame was issued from
    the DUT with the frame pending field set to 1, followed by a data frame
    with no payload.
    Frame Length: 5 bytes
    IEEE 802.15.4
    Frame Control: 0x0012
    .... .... .... .010 = Frame Type: ack (0x0002)
    .... .... .... 0... = Security Enabled: Disabled
    .... .... ...1 .... = Frame Pending: data
    .... .... ..0. .... = Acknowledgment Request: Ack not required
    .... .... .0.. .... = Intra-PAN: Not within the PAN
    .... ..00 0... .... = Reserved
    .... 00.. .... .... = Destination Addressing Mode: None
    ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
    00.. .... .... .... = Source Addressing Mode: None
    Sequence Number: xx
    Frame Check Sequence: Correct
    Frame Length: 12 bytes
    IEEE 802.15.4
    Frame Control: 0x8861
    .... .... .... .001 = Frame Type: Data (0x0001)
    .... .... .... 0... = Security Enabled: Disabled
    .... .... ...0 .... = Frame Pending: No more data
    .... .... ..1. .... = Acknowledgment Request: Ack required
    .... .... .1.. .... = Intra-PAN: Within the PAN
    .... ..00 0... .... = Reserved
    .... 10.. .... .... = Destination Addressing Mode: Address field contains a 16-bit short address
    ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
    10.. .... .... .... = Source Addressing Mode: Address field contains a 16-bit short address
    Sequence Number: xx
    Destination PAN Identifier: 0x1aaa
    Destination Address: 0x3344
    Source Address: 0x1122
    MAC Payload
    Not present
    Frame Check Sequence: Correct
    Fail:
    DUT does not send ACK in response to MAC data request command frame
    ACK has Frame pending field set
    DUT sends data frame

13. D.U.T. to tester: Short Address to Short Address, Indirect, with ACK (Tester does NOT poll, DUT purges).
    Fail:
    DUT returns MCPS-DATA.confirm primitive with Status = 0x00 = SUCCESS
    Pass:
    DUT returns MAC primitive
    MLME-PURGE.confirm(
    msduHandle = 0x0c,
    Staus = 0x00 - SUCCESS
    )
    Tester will transmit MAC data request command frame to coordinator, DUT
    One of two Pass conditions is possible as follows:
    Pass 1:
    It is possible to verify by a PAN analyzer that an ACK frame was issued from
    the DUT with the frame pending field set to 0.
    Frame Length: 5 bytes
    IEEE 802.15.4
    Frame Control: 0x0012
    .... .... .... .010 = Frame Type: ack (0x0002)
    .... .... .... 0... = Security Enabled: Disabled
    .... .... ...0 .... = Frame Pending: data
    .... .... ..0. .... = Acknowledgment Request: Ack not required
    .... .... .0.. .... = Intra-PAN: Not within the PAN
    .... ..00 0... .... = Reserved
    .... 00.. .... .... = Destination Addressing Mode: None
    ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
    00.. .... .... .... = Source Addressing Mode: None
    Sequence Number: xx
    Frame Check Sequence: Correct
    Pass 2:
    It is possible to verify by a PAN analyzer that an ACK frame was issued from
    the DUT with the frame pending field set to 1, followed by a data frame
    with no payload.
    Frame Length: 5 bytes
    IEEE 802.15.4
    Frame Control: 0x0012
    .... .... .... .010 = Frame Type: ack (0x0002)
    .... .... .... 0... = Security Enabled: Disabled
    .... .... ...1 .... = Frame Pending: data
    .... .... ..0. .... = Acknowledgment Request: Ack not required
    .... .... .0.. .... = Intra-PAN: Not within the PAN
    .... ..00 0... .... = Reserved
    .... 00.. .... .... = Destination Addressing Mode: None
    ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
    00.. .... .... .... = Source Addressing Mode: None
    Sequence Number: xx
    Frame Check Sequence: Correct
    Frame Length: 12 bytes
    IEEE 802.15.4
    Frame Control: 0x8861
    .... .... .... .001 = Frame Type: Data (0x0001)
    .... .... .... 0... = Security Enabled: Disabled
    .... .... ...0 .... = Frame Pending: No more data
    .... .... ..1. .... = Acknowledgment Request: Ack required
    .... .... .1.. .... = Intra-PAN: Within the PAN
    .... ..00 0... .... = Reserved
    .... 10.. .... .... = Destination Addressing Mode: Address field contains a 16-bit short address
    ..00 .... .... .... = Reserved for Zigbee/Zigbee PRO/RF4CE, Frame Ver. for Zigbee IP
    10.. .... .... .... = Source Addressing Mode: Address field contains a 16-
    bit short address
    Sequence Number: xx
    Destination PAN Identifier: 0x1aaa
    Destination Address: 0x3344
    Source Address: 0x1122
    MAC Payload
    Not present
    Frame Check Sequence: Correct
    Fail:
    DUT does not send ACK in response to MAC data request command frame
    ACK has Frame pending field set
    DUT sends data frame
