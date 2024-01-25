2.7 TP-PRE-TC-07: Security feature with ZR as DUT
This test case verifies the accordance to security with the DUT as a ZR. A typical scenario might be if
an outside attacker, wishing to control a lamp within a lighting network, sends a frame with no security
at all. This test verifies that this simple attack will fail.

Note: Need set Transiver volume to 0dB
Its set on MAC layer and need create global MAC function.
For added information see technical documentation on H/W.

Note: Need define NVRAM - see zb_config.h ZB_USE_NVRAM


ZED - ZLL ED, namely Dimmable light (single endpoint).
ZR  - ZLL router, namely Non-Color Scene Controller. The device is address-assignment-capable
      (i. e. compiled with ZB_ROUTER_ROLE defined).

1.  ZED1 and DUT ZR1 are powered on.
2.  Observe touchlinking commands between ZED1 and DUT ZR1.
3a. ZED1 unicasts a NWK rejoin request command frame to DUT ZR1.
3b. DUT ZR1 unicasts a NWK rejoin response command frame to ZED1.
3c. ZED1 broadcasts a ZDO device_annce command frame.
3d. DUT ZR1 broadcasts the ZDO device_annce command frame from ZED1.
4a. ZED1 unicasts a ZDO read attributes request command frame to DUT ZR1 with security
    disabled. This frame contains the cluster ID of the basic cluster and the attribute ID
    of the ZCL version attribute.
4b. DUT ZR1 transmits a MAC acknowledgement frame. DUT ZR1 does not respond further, i.e.
    a read attributes response command frame is not transmitted.
5a. ZED1 unicasts a ZDO read attributes request command frame to DUT ZR1 with security
    enabled. This frame contains the cluster ID of the basic cluster and the attribute ID
    of the ZCL version attribute.
5b. DUT ZR1 unicasts a ZDO read attributes response command frame to ZED1 containing a
    status of SUCCESS and the value of the ZCL version attribute from its basic cluster.

To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
