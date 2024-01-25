TP-CST-TC-21: Color control cluster (color temperature) with server as DUT

This test applies to the color loop mechanism in the color control cluster.

ZED - ZLL ED, namely Dimmable light (single endpoint).
ZR  - ZLL router, namely Non-Color Scene Controller. The device is address-assignment-capable
      (i. e. compiled with ZB_ROUTER_ROLE defined).

Prepare steps
1.  Start ZR and ZED both as factory new ZLL devices. Both of them check that they
    are in initial state and idle (i. e. not connected to any network). Both
    of them are limited to use the first ZLL primary channel only.
2.  ZED starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
3.  ZR responds with ZLL.Commissioning.Scan response.
5.  ZED starts Start New Network procedure with all needs parameters
6.  ZR start new network and response ZED.
7.  ZED rejoin this network.
8.  ZED send On command.
9.  ZED send Remove all groups command.
10. ZED unicasts a ZCL Move to Color command frame to ZR with the ColorX and ColorY
    fields set to appropriate values for a red color and the transition time field
    set to 0x000a (1s).

Test steps
1   ZED unicasts a ZCL read attributes command frame for the ColorTemperature,
    ColorTempPhysicalMin and ColorTempPhysicalMax attributes to ZR. ZR unicasts a
    ZCL read attributes response command frame to ZED. Note the values of these
    attributes.
2a  ZED unicasts a ZCL move to color temperature command frame to ZR with the color
    temperature field set to 0x03e8 (1000 = 1000K) and the transition time field
    set to 0x0064 (100 ≡ 10s). ZR transitions to a color temperature of 1000K over 10s.
2b  ZED unicasts a ZCL read attributes command frame for the ColorTemperature,
    ColorMode and EnhancedColorMode attributes to ZR. ZR unicasts a ZCL read
    attributes response command frame to ZED. ColorTemperature attribute has a value
    about 0x03e8 (1000). ColorMode attribute has the value 0x02. EnhancedColorMode
    attribute has the value 0x02.
3a  ZED unicasts a ZCL move to color temperature command frame to ZR with the color
    temperature field set to a value < ColorTempPhysicalMin (determined in step 1)
    and the transition time field set to 0x0064 (10s). ZR unicasts a ZCL default
    response command frame with the status field set to 0x87 (INVALID_VALUE). ZR 
    does not change color.
3b  ZED unicasts a ZCL read attributes command frame for the ColorTemperature,
    ColorMode and EnhancedColorMode attributes to ZR. ZR unicasts a ZCL read
    attributes response command frame to ZED. ColorTemperature attribute has a
    value about 0x03e8 (1000). ColorMode attribute has the value 0x02.
    EnhancedColorMode attribute has the value 0x02.
4a  ZED unicasts a ZCL move to color temperature command frame to ZR with the color
    temperature field set to a value > ColorTempPhysicalMax (determined in step 1)
    and the transition time field set to 0x0064 (10s).  ZR unicasts a ZCL default
    response command frame with the status field set to 0x87 (INVALID_VALUE).
    ZR does not change color.
4b  ZED unicasts a ZCL read attributes command frame for the ColorTemperature,
    ColorMode and EnhancedColorMode attributes to ZR. ZR unicasts a ZCL read
    attributes response command frame to ZED. ColorTemperature attribute has a
    value about 0x03e8 (1000). ColorMode attribute has the value 0x02.
    EnhancedColorMode attribute has the value 0x02.
5a  ZED unicasts a ZLL move color temperature command frame to ZR with the move
    mode field set to 0x01 (move up), the rate field set to 0x03e8 (1000 units
    per second), the color temperature minimum field set to 0x0000 (minimum is
    ColorTempPhysicalMin) and the color temperature maximum field set to 0x7530
    (30000 = minimum 33.3K). ZR begins to increment its color temperature.
5b  After 10sec ZED unicasts a ZLL move color temperature command frame to ZR with
    the move mode field set to 0x00 (stop the move), the rate field set to 0x0000,
    the color temperature minimum field set to 0x0000 and the color temperature
    maximum field set to 0x0000. ZR stops incrementing its color temperature.
5c  ZED unicasts a ZCL read attributes command frame for the ColorTemperature
    attribute to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    ColorTemperature attribute has a value about 0x2af8 (11000).
6a  ZED unicasts a ZLL move color temperature command frame to ZR with the move mode
    field set to 0x01 (move up), the rate field set to 0x03e8 (1000 units per second),
    the color temperature minimum field set to 0x0000 (minimum is ColorTempPhysicalMin)
    and the color temperature maximum field set to 0x3a98 (15000 = minimum  66.6K).
    ZR begins to increment its color temperature.
6b  After 6s, ZR stops incrementing its color temperature. ZED unicasts a ZCL read
    attributes command frame for the ColorTemperature attribute to ZR. ZR unicasts
    a ZCL read attributes response command frame to ZED. ColorTemperature attribute
    has a value about 0x3a98 (15000).
7a  ZED unicasts a ZLL move color temperature command frame to ZR with the move mode
    field set to 0x03 (move down), the rate field set to 0x01f4 (500 units per second),
    the color temperature minimum field set to 0x01f4 (500 = maximum 2000K) and the
    color temperature maximum field set 0x0000 (maximum is ColorTempPhysicalMax).
    ZR begins to decrement its color temperature.
7b  After 10 sec ZED unicasts a ZLL move color temperature command frame to ZR with
    the move mode field set to 0x00 (stop the move), the rate field set to 0x0000,
    the color temperature minimum field set to 0x0000 and the color temperature maximum
    field set to 0x0000. ZR stops decrementing its color temperature.
7c  ZED unicasts a ZCL read attributes command frame for the ColorTemperature attribute
    to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    ColorTemperature attribute has a value about 0x2710 (10000).
8a  ZED unicasts a ZLL move color temperature command frame to ZR with the move mode
    field set to 0x03 (move down), the rate field set to 0x03e8 (1000 units per second),
    the color temperature minimum field set to 0x1b58 (7000 =  maximum 142.9K) and the
    color temperature maximum field set 0x0000 (maximum is ColorTempPhysicalMax). ZR
    begins to decrement its color temperature.
8b  After 3s, ZR stops decrementing its color temperature. ZED unicasts a ZCL read
    attributes command frame for the ColorTemperature attribute to ZR. ZR unicasts a
    ZCL read attributes response command frame to ZED. ColorTemperature attribute has
    a value about 0x1b58 (7000).
9a  ZED unicasts a ZLL step color temperature command frame to ZR with the step mode
    field set to 0x01 (step up), the step size field set to 0x2710 (10000 units per
    step), the transition time field set to 0x00c8 (20s), the color temperature
    minimum field set to 0x0000 (minimum is ColorTempPhysicalMin) and the color
    temperature maximum field set to 0x7530 (30000 = minimum 33.3K). ZR begins to
    increment its color temperature.
    Note: for 10 sec before next test step has no color step (10 sec < 20 sec per step).
    May be step size = 0x01f4 (500 unit per step), transition time = 0x000a (1s).
9c  ZED unicasts a ZCL read attributes command frame for the ColorTemperature
    attribute to ZR. ZR unicasts a ZCL read attributes response command frame
    to ZED. ColorTemperature attribute has a value about 0x2ee0 (12000).
10a ZED unicasts a ZLL step color temperature command frame to ZR with the step
    mode field set to 0x01 (step up), the step size field set to 0x1388 (5000
    units per step), the transition time field set to 0x0032 (5s), the color
    temperature minimum field set to 0x0000 (minimum is ColorTempPhysicalMin) and
    the color temperature maximum field set to 0x3a98 (15000 = minimum  66.6K).
    ZR begins to increment its color temperature.
10b After 3s, ZR stops incrementing its color temperature. ZED unicasts a ZCL read
    attributes command frame for the ColorTemperature attribute to ZR. ZR unicasts
    a ZCL read attributes response command frame to ZED. ColorTemperature attribute
    has a value about 0x3a98 (15000).
11a ZED unicasts a ZLL step color temperature command frame to ZR with the step mode
    field set to 0x03 (step down), the step size field set to 0x4e20 (20000 units
    per step), the transition time field set to 0x00c8 (20s), the color temperature
    minimum field set to 0x01f4 (500 = maximum 2000K) and the color temperature
    maximum field set to 0x0000 (maximum is ColorTempPhysicalMax). ZR begins to
    decrement its color temperature.
    Note: for 10 sec before next test step has no color step (10 sec < 20 sec per step).
    May be step size = 0x07d0 (1000 unit per step), transition time = 0x0014 (2s)
11b ZED unicasts a ZLL stop move step command frame to ZR. ZR stops decrementing
    its color temperature.
11c ZED unicasts a ZCL read attributes command frame for the ColorTemperature
    attribute to ZR. ZR unicasts a ZCL read attributes response command frame to
    ZED. ColorTemperature attribute has a value about 0x1388 (5000).
12a ZED unicasts a ZLL step color temperature command frame to ZR with the step
    mode field set to 0x03 (step down), the step size field set to 0x1388 (5000
    units per step), the transition time field set to 0x0032 (5s), the color
    temperature minimum field set to 0x07d0 (2000 = maximum 500K)  and the color
    temperature maximum field set to 0x0000 (maximum is ColorTempPhysicalMax).
    ZR begins to decrement its color temperature.
12b After 3s, ZR stops decrementing its color temperature. ZED unicasts a ZCL read
    attributes command frame for the ColorTemperature attribute to ZR. ZR unicasts
    a ZCL read attributes response command frame to ZED. ColorTemperature attribute
    has a value about 0x07d0 (2000).


To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
