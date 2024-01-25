11.5 TP/NWK/BV-05 NWK Maximum Depth
Verify that DUT, in position of a router in a larger network interoperability scenario, acts
correctly after joining at maximum depth (Pro Max depth=15; OK to join at depth 0xF).
Zigbee Pro devices are allowed to join devices in Zigbee Pro networks who are already at
the maximum depth.
Zigbee 2007 DUT: ZRmax
Zigbee Pro DUTs: ZRmax, ZRmax+1

Max = 5;

gZC
gZR1
gZR2
gZR3
gZR4
DUT ZR5
DUT ZR6 (for Pro)

Initial conditions:
gZC:
PANid= 0x1AAA
Logical Address = 0x0000
0x aa aa aa aa aa aa aa aa

ZRmax DUT:
PANid=0x1AAA
Logical Address= 0x000max
(e.g. Residential Network stack profile: 0x0005, 0x 00 00 00 05 00 00 00 00)

gZR1 to gZRmax-1, ZRmax+1 (DUT for Zigbee Pro, Golden for 2007):
PANid= 0x1AAA
Logical Address=0x0001 to 0x000max-1, 0x000max+1 (e.g. RN stack profile 0x0004, 0x0006)
0x 00 00 00 01 00 00 00 00 to
0x 00 00 00 0max-1 00 00 00 00,
0x 00 00 00 0max+1 00 00 00 00
(e.g. 0x 00 00 00 04 00 00 00 00, 0x00 00 00 06 00 00 00 00)reb

1 Reset all nodes
2 Set ZC under target stack profile, ZC as coordinator starts a PAN = 0x1AAA network
3 gZR1 to gZRmax(DUT) join the PAN


test Procedure:
1 gZRmax+1 requests to join at ZRmax (DUT) where max = maximum depth limit(5).
2 ZigBee Pro DUT Only: Initiate a beacon request from a golden unit that device ZRmax+1(DUT) can hear.


Pass verdict:
1) gZRmax+1 issues beacon request and then Associate_Request to ZRmax(DUT)
2) The behaviour will depend on the type of device of the DUT:
    A. For Zigbee 2007 Devices: ZR5 issues beacons where the beacon payload
has the “router capacity” and “device capacity” bits set to zero and the
“association permit” flag is set to zero. ZRmax (DUT) issues MAC
Associate_Response frame with status=PAN at capacity. Note that it is
allowable for the capacity bits not to be present in this case.
    B. For Zigbee Pro Devices: ZRmax(DUT) issues beacons where the beacon
payload has the “router capacity” and “device capacity” bits set to one and
the “association permit” flag is set to one. ZRmax (DUT) issues MAC
Associate_Response frame with status=success. Note that it is allowable
for Zigbee Pro devices to join in this case.
        i. The Beacon of ZRmax+1(DUT) shall have a NWK depth of nwkMaxDepth (0x0F).
Fail verdict:
1) The behaviour will depend on the type of device of the DUT:
    a. ZigBee 2007 Devices: ZRmax(DUT) allows gZRmax+1 to join the PAN.
    b. ZigBee Pro Devices: ZRmax(DUT) does not join the network.
        i. ZRmax+1(DUT) does not issue a beacon, or the NWK depth of the beacon is a value other than nwkMaxDepth (0x0F).


Notes:
PICS Covered:
A list of PICS that are covered by the Test case.
AZD48 - Mgmt_Direct_Join (Indirect)
AZD32 - NLME-JOIN (Direct)


Comment:
Zigbee coordinator and each router  configured to have only one child. 
*For 2007: Check that net do not accept new child when it reaches the maximum depth. 
**Check that MAX_DEPTH + 1 device couldn't join to the network.
