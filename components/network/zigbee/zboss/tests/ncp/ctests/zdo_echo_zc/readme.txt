/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * www.dsr-zboss.com
 * www.dsr-corporation.com
 * All rights reserved.
 *
 * This is unpublished proprietary source code of DSR Corporation
 * The copyright notice does not evidence any actual or intended
 * publication of such source code.
 *
 * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
 * Corporation
 *
 * Commercial Usage
 * Licensees holding valid DSR Commercial licenses may use
 * this file in accordance with the DSR Commercial License
 * Agreement provided with the Software or, alternatively, in accordance
 * with the terms contained in a written agreement between you and
 * DSR.
 */

ZR joins NCP implementing ZC. After joining APS data packet transfers between devices,
ZC replies with the same packet, which it recieves.

Sources:
ZC - tests/ncp/host/ctests/zdo_echo_zc/main.c
ZR - tests/zdo/zdo_startup/zdo_startup_zr.c

Test set works on 0 page 11 channel. ZC uses production configuration file.

To execute the test follow these steps:
1. Build ZBOSS with different settings for NCP Host, NCP Device and common NS application. It should be tree separate builds.
  - For NCP Host use platform_linux_nsng/build/linux-nsng-ncp-host/build_kernel.sh
  - For NCP Device use platform_linux_nsng/build/linux-nsng-ncp-dev/build_kernel.sh
  - For common NS use platform_linux_nsng/build/linux-nsng/build_kernel.sh
2. Build current NCP application in the NCP Host build, run make in the tests/ncp/host/ctests/zdo_echo_zc folder
3. Build NS application in the common NS build. Run make in the tests/zdo/zdo_startup folder
4. Run test set, execute applications in the following order:
  - Start NSNG from the common NS build.
  - Execute NCP Host application in the NCP Host build. Run ./zdo_echo_zc from tests/ncp/host/ctests/zdo_echo_zc folder
  - Execute NCP Device application in the NCP Device build. Run ./ncp_fw from ncp folder. After this step in the zdo_echo_zc.log and in the ncp_fs.log files you could see exchange of packets between NCP Host and Device. 
  - Execute ZR router from the common NS build. Run ./zdo_start_zr from tests/zdo/zdo_startup folder. After completing this step you could also analyze packet exchange between ZR and ZC, open created .pcap file with Wireshark (this .pcap file is placed in the same folder, as NSNG application)
