    ZBOSS Zigbee software protocol stack

    Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
    www.dsr-zboss.com
    www.dsr-corporation.com
    All rights reserved.

    This is unpublished proprietary source code of DSR Corporation
    The copyright notice does not evidence any actual or intended
    publication of such source code.

    ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
    Corporation

    Commercial Usage
    Licensees holding valid DSR Commercial licenses may use
    this file in accordance with the DSR Commercial License
    Agreement provided with the Software or, alternatively, in accordance
    with the terms contained in a written agreement between you and
    DSR.



ZBOSS Custom Cluster set of applications
========================================

This set of applications demonstrates user defined Custom Cluster implementation.
The set contains two applications:

  - Zigbee Coordinator (which implements Custom Cluster Client)
  - Zigbee Router (which implements Custom Cluster Client)

These applications implements Zigbee 3.0 specification, Base Device Behavior specification and Zigbee Cluster Library 7 revision specification.
By default, both devices work on the 0 page 21 channel.

The application set structure
------------------------------

 - Makefile
 - custom_cluster_zc - *Custom Cluster coordinator application*
   - custom_cluster_zc.c
   - custom_cluster_zc.h
 - custom_cluster_zr - *Custom Cluster router application*
   - custom_cluster_zr.c
   - custom_cluster_zr.h
 - readme.txt

Zigbee Coordinator application
-------------------------------

Zigbee Coordinator includes following ZCL clusters:

 - Basic (s)
 - Identify (s/c)
 - Custom (c)

Zigbee Router application
--------------------------

Zigbee Router includes following ZCL clusters:

 - Basic (s)
 - Identify (s)
 - Custom (s)

Applications behavior
----------------------

- Zigbee Coordinator creates network on ZB_CUSTOM_CHANNEL_MASK channel
- Zigbee Router joins Zigbee Coordinator using the BDB commissioning
  - Zigbee Coordinator sends Simple Descriptor Request and Zigbee Router replies with the Simple Descriptor Response
  - Zigbee Coordinator saves the endpoint and the short address of the connected device in 'finding_binding_cb'
- If the connected device supports the Custom Cluster, Zigbee Coordinator schedules 'send_custom_cluster_cmd'
  to send the Custom Cluster commands.
- Values of the Custom Cluster commands fields are generate randomly
  Command 1 and Command 2 will have the incorrect values with 20% probability
  (can be customized using PROBABILITY_OF_INCORRECT_VALUE)
- Zigbee Router replies with the Custom Cluster responses on Commands 1 and 2 if they have correct parameters,
  otherwise it replies with the Default Response with status code


Test Script - SDK Samples
=============================
 Objective:
   Test and test scripts under SDK Samples context, performs high-level/functionality checks.

 Devices:
   1. ZC - custom_cluster_zc
   2. ZR - custom_cluster_zr

 Initial conditions:
   1. All devices are factory new and powered off until used.

 Test procedure:
   1. ZC start.
   2. ZR start.

 Expected outcome:
   For 'Test procedure' item 1:
     1.1. ZC -> Broadcast: Beacon Request
   For 'Test procedure' item 2:
     2.2. ZC -> Broadcast: Beacon
     2.3. ZR -> ZC: Association
     2.4. ZR -> ZC: BDB commissioning
     2.5. ZC -> ZED: Identify Query
     2.6. ZR -> ZC: Identify Query Response
     2.7. ZC -> ZR: Simple Descriptor Request
     2.8. ZR -> ZC: Simple Descriptor Response
     2.9. ZC -> ZR: Command 0x21
     2.10. ZR -> ZC: Command 0xf1
     2.11. ZC -> ZR: Command 0x22
     2.12. ZR -> ZC: Command 0xf2
     2.13. ZC -> ZR: Command 23
     2.13. ZR -> ZC: Default Response
     2.14. ZC -> ZR: Read Attribute 0x0001
     2.15. ZR -> ZC: Read Attribute Response 0x0001
     2.16. ZC -> ZR: Read Attribute 0x0002
     2.17. ZR -> ZC: Read Attribute Response 0x0002
     2.18. ZC -> ZR: Read Attribute 0x0003
     2.19. ZR -> ZC: Read Attribute Response 0x0003
     2.20. ZC -> ZR: Read Attribute 0x0004
     2.21. ZR -> ZC: Read Attribute Response 0x0004
     2.22. ZC -> ZR: Read Attribute 0x0005
     2.21. ZR -> ZC: Read Attribute Response 0x0005
     2.23. ZC -> ZR: Read Attribute 0x0006
     2.24. ZR -> ZC: Read Attribute Response 0x0006
     2.25. ZC -> ZR: Read Attribute 0x0007
     2.26. ZR -> ZC: Read Attribute Response 0x0007
     2.27. ZC -> ZR: Read Attribute 0x0008
     2.28. ZR -> ZC: Read Attribute Response 0x0008
     2.29. ZC -> ZR: Read Attribute 0x0009
     2.30. ZR -> ZC: Read Attribute Response 0x0009
     2.31. ZC -> ZR: Read Attribute 0x000a
     2.32. ZR -> ZC: Read Attribute Response 0x000a
     2.33. ZC -> ZR: Read Attribute 0x0001
     2.34. ZR -> ZC: Read Attribute Response 0x0001
     2.35. ZC -> ZR: Read Attribute 0x0002
     2.36. ZR -> ZC: Read Attribute Response 0x0002
     2.37. ZC -> ZR: Write Attributes Undivided:
     2.38. ZR -> ZC: Command: Write Attributes Response (0x04)
     2.39. ZC -> ZR: Read Attribute 0x0001
     2.40. ZR -> ZC: Read Attribute Response 0x0001
     2.41. ZC -> ZR: Read Attribute 0x0002
     2.42. ZR -> ZC: Read Attribute Response 0x0002
     2.43. ZC -> ZR: Write Attributes:  Attribute: 0x0001
     2.44. ZR -> ZC: Command: Write Attributes Response (0x04)
     2.43. ZC -> ZR: Write Attributes:  Attribute: 0x0002
     2.44. ZR -> ZC: Command: Write Attributes Response (0x04)
     2.43. ZC -> ZR: Write Attributes: Attribute: 0x0003
     2.44. ZR -> ZC: Command: Write Attributes Response (0x04)
     2.43. ZC -> ZR: Write Attributes: Attribute: 0x0004
     2.44. ZR -> ZC: Command: Write Attributes Response (0x04)
     2.43. ZC -> ZR: Write Attributes: Attribute: 0x0005
     2.44. ZR -> ZC: Command: Write Attributes Response (0x04)
     2.43. ZC -> ZR: Write Attributes: Attribute: 0x0006
     2.44. ZR -> ZC: Command: Write Attributes Response (0x04)
     2.43. ZC -> ZR: Write Attributes: Attribute: 0x0007
     2.44. ZR -> ZC: Command: Write Attributes Response (0x04)
     2.43. ZC -> ZR: Write Attributes: Attribute: 0x0008
     2.44. ZR -> ZC: Command: Write Attributes Response (0x04)
     2.43. ZC -> ZR: Write Attributes: Attribute: 0x0009
     2.44. ZR -> ZC: Command: Write Attributes Response (0x04)
     2.43. ZC -> ZR: Write Attributes: Attribute: 0x000a
     2.44. ZR -> ZC: Command: Write Attributes Response (0x04)
