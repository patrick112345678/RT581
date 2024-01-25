Test for optional routing features (force route discovery and force send route record)

In this test, ZC starts with ax_children == 1 and open network
After that ZR joins to ZC, and ZC opens full network again, for joining other devices to ZR.

We need topology ZC<->ZR<->ZR, so test are using address assignment callbacks and setting visibility by short addr