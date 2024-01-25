CN-NST-TC-06: Network steering on a factory new device; no network to join; DUT: ZR/ZED

This test verifies the behavior of a joiner device when no suitable open network is found.
Since network formation was not triggered, the device does not form its own network. 
This test cannot be performed, if the DUT does not offer the option to perform network steering only, i.e. if network steering, if unsuccessful, will be followed by an attempt to form a network.


Required devices
DUT - ZR/ZED


Test Procedure:
 - Power on DUT
 - Trigger steering on the DUT
 - Wait for 20s. DUT does NOT send Link Status command and does not form a network. (if DUT is router)

