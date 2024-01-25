5.3.5 FB-INI-TC-05: Finding and binding on FN device
This test verifies the behavior of factory-new device, on which finding and binding is triggered. 
This test shall only be performed, if the DUT supports triggering of finding & binding alone, separately from the network formation and network steering.


Required devices:
DUT - ZC, ZR, or ZED; 
      supporting generation of device discovery request primitives as mandated by the BDB specification


Test procedure:
Before: Finding& binding (only) is triggered on the DUT.

After: If a target, DUT does NOT start identifying.
If an initiator, DUT does NOT send Identify cluster, Identify Query command.
DUT does NOT send any Beacon Requests, in an attempt to join or form a network. 


Additional info: run test with argument - runng.sh $dut_role, where $dut_role can be zr, zc or zed
i.e: sh runng.sh zed - run test for ZED