Most frequently used, basic test. Imitates device behavior after power on.
Depending of device type creates or joins the network.

zdo_start_zc start as ZC, zdo_start_ze joins to it, then tests sends APS
packets to each other infinitely.


This test can be used to check the APS retransmission.


End device (zdo_start_ze) send packets to the coordinator (zdo_start_zc).
Coordinator, in response sends ACK packages for confirmation.
Through time ACK packets contain incorrect data and the end device must in this case 
repeat the package with wrong ACK.
This can be verified with Wireshark.
