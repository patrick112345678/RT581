11.14 TP/APS/BV-14-I Group Management-Group Addition-Rx
Objective: Verify that DUT can read a group-destined broacast to an implicit endpoint.

    GZC
|       |
GZR1    DUTZR2

Group Address 0x0001: gZR1, ZR2 members with endpoint 0xF0.

-gZR1 and DUT ZR2 join the network at gZC.
-Assign group address 0x0001 to gZR1 group members, with endpoint assignment of 0xf0, by implementation-specific means.

Test procedure:
1. gZC shall NWK broadcast (NWK address = 0xffff) a Test Buffer Request (cluster Id=0x001c) to endpoint 0xF0, to Group 0x0001;
2. DUT ZR2 shall, by implementation specific means, enter group address 0x0001 and correlate it to endpoint 0xF0;
3. gZC shall NWK broadcast (NWK address = 0xfff) a Test Buffer Request (cluster Id=0x001c) to group address 0x0001;

Pass verdict:
1) gZC broadcasts a Test Buffer Req to gZR1 and DUT ZR2
2) gZR1 responds with Test Buffer Rsp to gZC from endpoint 0xF0; gZR1 will rebroadcast.
***ZR2 will rebroadcast this buffer too (nwk multicast flag is enabled) - see member-mode multicast communication;
3) gZC broadcasts a Test Buffer Req to Group 0x0001.
4) DUT ZR2 and gZR1 responds with Test Buffer Rsp to gZC from endpoint 0xF0; DUT ZR2 and gZR1 will rebroadcast.

Fail verdict:
1) gZC broadcasts a Test Buffer Req to gZR1 and DUT ZR2. DUT ZR2
responds with Test Buffer Rsp to gZC from endpoint 0xF0.
2) gZC broadcasts a Test Buffer Req to Group 0x0001. DUT ZR2 does not respond with Test Buffer Rsp to gZC from endpoint 0xF0. 
DUT does not rebroadcast.

Comments:
* Ember provides special macro for changing destination address in NWK header: EMBER_SEND_MULTICASTS_TO_SLEEPY_ADDRESS
+ comment:
/** @brief This defines the behavior for what address multicasts are sent to
 *    The normal address is RxOnWhenIdle=TRUE (0xFFFD).  However setting this
 *    to true can change locally generated multicasts to be sent to the sleepy
 *    broadcast address (0xFFFF). Changing the default is NOT ZigBee Pro 
 *    compliant and may not be interoperable.
 */






