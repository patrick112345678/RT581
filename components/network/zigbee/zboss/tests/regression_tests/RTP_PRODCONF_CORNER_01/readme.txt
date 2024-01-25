RTP_PRODCONF_CORNER_01 - Production config application.

Objective:

    To confirm that production config applied correctly if values are corner cases values after ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY returned.
        Corner cases are manufacturer name and model id strings with maximal length.

Devices:

    1. TH - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.
    2. APS trace level shall be enabled.
    3. Production config shall loaded to flash address 0xC8000
    3.1 Production config binary shall be generated from prodconfig_01_dut_zed.txt
    3.2 Can be done by calling ./generate_prodconfig.sh
        - Monolithic builds:
            ./generate_prodconfig.sh
        - NCP builds:
            ./generate_prodconfig.sh ncp
    3.3 Then it shall be converted to hex and flushed to DUT device.

Test procedure:

  1. Power on DUT ZED
  3. Wait for 30 seconds for devices to complete writing trace

Expected outcome:
	1. DUT ZED wrote to trace:
    [APP1] Manufacturer name - test OK
    [APP1] Model id - test OK
    [APP1] Manufacturer code - test OK
    [APP1] Install code: 83:fe:d3:40
    [APP1] Install code: 7a:93:97:23
    [APP1] Install code: a5:c6:39:b2
    [APP1] Install code: 69:16:d5:5
    [APP1] Install code: c3:b5
    [APP1] IEEE 64-bit address: ca.fe.be.ef.50.c0.ff.ea
    [APP1] Channel mask is : 7fff800
    [APP1] BDB Primary channel mask: 7fff800

  2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC
    3. Check that there is DUT devices with long address ca:fe:be:ef:50:c0:ff:ea
