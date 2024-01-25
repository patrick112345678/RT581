## Standalone Testing of TC swapout

a) 2 devices: ZC and Joiner (joined directly to ZC)
b) 3 devices: ZC, ZR, Joiner (joined thru ZR)
Joiner ZR / ZED.
ZB 3.0 build.

1. ZC start - Formation
2. Joiner joins and setup TCLK
3. ZC backs up TC Swap DB. Backup is into NVRAM (see below).
4. Joiner once per 5 sec sends Simple desc req to ZC. If ZD does not answer, it is dead.
5. After timeout ZC kills itself erasing in NVRAM all but 
6. When Joiner loss ZC, it starts TC Rejoin.
7. Start ZC again. It starts with no_autostart, after "first boot" signal ZC loads TC Swap DB and continues start - must complete Formation.
8. Joiner finally got TC Swapped signalm then establishes TCLK again.
9. Joiner continues to poll ZC by Simple Desc req.

### ZC details

Use same ZC for both "swapped" and "don't swapped" ZC (same binary for NSNG, same device for HW).

The only common storage for nsng and ZC at HW is nvram - let's use it.

TC swapout DB save - into ZB_NVRAM_APP_DATA1.
To simplify implementation, Load entire DB into RAM buffer (use statically allocated 1k buf), then save into APP_DATA1 in one chunk.

When it is time to die, ZC removes from NVRAM all except ZB_NVRAM_APP_DATA1.
Use zb_nvram_clear() call, then call zb_reset() - it calls exec() for nsng.

ZC registers callbacks to handle ZB_NVRAM_APP_DATA1.
See simple_gw.c as an example:
  zb_nvram_register_app1_read_cb(simple_gw_nvram_read_app_data);
  zb_nvram_register_app1_write_cb(simple_gw_nvram_write_app_data, simple_gw_get_nvram_data_size);

ZC starts calling zb_zdo_start_no_autostart().
It loads NVRAM so, if this is not first ZC start, app dataset read callback is called. That cb reads app1 dataset into the static buffer ans sets "TC DB loaded" flag.
When ZC got ZB_ZDO_SIGNAL_SKIP_STARTUP in zboss_signal_handler(), it checks for "tc DB load" flag and, if it set, loads TC DB.
ZC then updates its IEEE address (may increment low byte).
Then just call zboss_start_continue() - it will do Formation.
