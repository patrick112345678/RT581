
11.23 TP/APS/ BV-23-I Binding with Groups (HC V1 RX DUT)
Objective: DUT as Router maintains a binding table as source in regular bind while also implementing group bindings.

gZC
DUT ZR1
ZR2 (ep 0xF0, gr 0x0003)
ZR3 (ep 0xFO, gr 0x0004)

       ZC
 |     |      |
ZR1   ZR2    ZR3


ZB_NIB().nwk_use_multicast = NO;


Initial Conditions:
1 DUT ZR1 joins the network at gZC
2 gZR2 joins the network
3 gZR3 joins the network


Test procedure:
1. gZC sends Bind_Request to DUT ZR1 for Cluster Id=0x001c, SrcEndpt=0x01, with Group address=0x0003
2. By implementation specific means, additionally register endpoint 0xF0 on gZR2 to Group address 0x0003.
3. Transmit Test Buffer Request with length 0x10 from DUT ZR1 to Group address=0x0003.
4. By implementation specific means, additionally register endpoint 0xF0 on gZR3 to Group address 0x0004.
5. gZC sends Bind_Request to DUT ZR1 for Cluster Id=0x001c, SrcEndpt=0x01, with Group address=0x0004
6. Transmit Test Buffer Request with length 0x10 from DUT ZR1 to Group address=0x0004.
7. gZC sends UnBind_Request to DUT ZR1 for Cluster Id=0x0001, SrcEndpt=0x01, Group address=0x0003
8. Transmit Test Buffer Request with length 0x10 from DUT ZR1 to Group address=0x0003.
9. Transmit Test Buffer Request with length 0x10 from DUT ZR1 to Group address=0x0004.


Pass verdict:
1) Upon gZC sending Bind_Request to DUT ZR1, DUT ZR1 responds with Bind_Response with Status=SUCCESS;
2) DUT ZR1 is able to transmit broadcast Test Buffer request to group address 0x0003;
3) DUT ZR2 responds to DUT ZR1 with a Test Buffer response;
4) Upon gZC sending Bind_Request to DUT ZR1. DUT ZR1 responds with Bind_Response with Status=SUCCESS; 
IEEE address request is issued from DUT to earch for bind entries;
5) DUT ZR1 sends out a Test Buffer request broadcast to group address 0x0004;
6) gZR3 responds to DUT ZR1 with a Test Buffer response;
7) Upon gZC sending UnBind_Request to DUT ZR1. DUT ZR1 responds with UnBind_Response with Status=SUCCESS;
8) DUT ZR1 does not send out a Test Buffer Request broadcast to group address 0x0003;
9) DUT ZR1 sends out a Test Buffer Request broadcast to group address 0x0004


Fail verdict:
1) Upon gZC sending Bind_Request to DUT ZR1, DUT ZR1 responds with Bind_Response with Status<>SUCCESS;
2) Upon gZC sending Bind_Request to DUT ZR1. DUT ZR1 responds with Bind_Response with Status<>SUCCESS;
3) DUT ZR1 cannot send out a Test Buffer request broadcast to group address 0x0003;
4) Upon gZC sending UnBind_Request to DUT ZR1. DUT ZR1 responds with UnBind_Response with Status<>SUCCESS;
5) DUT ZR1 sends out a Test Buffer Request broadcast to group address 0x0003;
6) DUT ZR1 does not send out a Test Buffer Request broadcast to group address 0x0004;

Comments:
ZC after startup completition:
after 30 sec. timeout bind_req group 0x0003   -->ZR1
after 40 sec. timeout bind_req group 0x0004   -->ZR1
after 60 sec. timeout unbind_req group 0x0003 -->ZR1

ZR1 after startup completition:
after 30 sec. send test buffer request by group address 0x0003;
after 40 sec. send test buffer request by group address 0x0004;
after 60 sec. send test buffer request by group address 0x0003;
after 70 sec. send test buffer request by group address 0x0004;

ZR2 after startup completition:
add_group_req: group_addr = 0x0003; endpoint = 0xF0;

ZR3 after startup completition:
add_group_req: group_addr = 0x0004; endpoint = 0xF0;

Note: send all test buffer requests using mode ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT - use binding table.
That means, at step 5 ZR1 sends to both group 3 and 4, and both gZR2 and gZR3 answers.
