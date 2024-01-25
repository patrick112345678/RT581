
ZOI-84 - Provide new features to allow:
         . ZED accepts parent_info bitmask with both keepalive methods enabled.
         . ZED chooses ED_TIMEOUT_REQUEST_KEEPALIVE in such case.
         . ZC/ZR sets keepalive method using public function instead of macro.

ZOI-93 - Fix bug found when:
         . ED_TIMEOUT_REQUEST_KEEPALIVE method is used and
         . Turbo Poll feature is disabled,
       - which causes:
         . polling for SED to be disabled.


RTP_NWK_05 - Given the related nature of both Jira issues, a single regression
             test easily checks both with a simple setup.


Objective:

    To confirm that:
[ZOI-84] a) ZR/ZC is able to set both keepalive methods using public API function
            zb_set_keepalive_mode().
[ZOI-84] b) ZED is capable of handling it - i.e. recognize 'parent_info' bitmask
            with value of BOTH_KEEPALIVE_METHODS as valid.
[ZOI-84] c) ZED selects the ED_TIMEOUT_REQUEST_KEEPALIVE method in such case.
[ZOI-93] d) SED with Turbo Poll feature disabled, is able to Poll its parent
            when ED_TIMEOUT_REQUEST_KEEPALIVE is being used.


Devices:

    1. TH    - ZC
    2. DUT   - ZED
    3. DUT_2 - ZED_2


Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:
    1. Power on TH ZC
    2. Power on DUT ZED
    3. Wait for DUT ZED association with TH ZC
    4. Power on TH ZED_2
    5. Wait for TH ZED_2 association with TH ZR

Expected outcome:

    1. TH ZC creates a network (it supports both keepalive methods).

    2. DUT ZED, which is SED (Association Request shall have .... 0...), gets on
       the network established by TH ZC.
      2.1. Association Response shall be successful i.e. Association status
           shall be 0x00.

    3. DUT ZED sends command frame with ID 0x0b. This is an 'End Device Timeout
       Request'.
      3.1. Afterwards, parent shall respond with command frame 0x0c. This is an
           'End Device Timeout Response'. Status shall be 0, indicating success.
      3.2. Moreover, 'Parent Information' shall carry the value 0x03. This
           indicates that both keepalive methods are supported by the Parent.

    4. After above steps, DUT ZED shall keep sending 'End Device Timeout
       Requests' and receiving successful 'End Device Timeout Responses'.
       Interleaved, Data request frames shall be sent and acknowledged.

    5. 60 seconds later, DUT_2 ZED_2 comes into play.
      5.1. It mimics all steps above, exception being that it has Turbo Poll
      disabled!

    6. This (5.1.) shall not be an issue, therefore exactly the same command
       exchange is expected (in regard of keepalive and data poll):
      6.1. Same sequence found in 2. and 2.1.
      6.2. Same sequence found in 3. and 3.1.
      6.3. Same repeated sequence of events described in 4.
