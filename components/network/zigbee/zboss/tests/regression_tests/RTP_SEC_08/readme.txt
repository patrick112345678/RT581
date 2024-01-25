ZOI-221 - install-codes management API changes
RTP_SEC_08 - install-codes management functions testing

Objective:

    To confirm that IC (install-codes) management functions allows to add, delete, get install-codes

Devices:

    1. DUT - ZC

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on DUT ZC
    2. Wait for DUT ZC to print string "test procedure is completed" to its trace

Expected outcome:

    1. DUT ZC tries to remove non-existing IC:
        test_remove_ic_by_device_idx_cb: status 213

    2. DUT ZC fills IC table with 5 IC entries:
        test_fill_ic_table, param 0, idx 0 of 4
        test_fill_ic_table_cb: status 0

        test_fill_ic_table, param 0, idx 1 of 4
        test_fill_ic_table_cb: status 0

        test_fill_ic_table, param 0, idx 2 of 4
        test_fill_ic_table_cb: status 0

        test_fill_ic_table, param 0, idx 3 of 4
        test_fill_ic_table_cb: status 0

        test_fill_ic_table, param 0, idx 4 of 4
        test_fill_ic_table_cb: status 0

    3. DUT ZC requests and prints IC table:
        ===========================

        test_get_ic_list_cb: status 0
        test_get_ic_list_cb: ic_table_entries 5
        test_get_ic_list_cb: start_index 0
        test_get_ic_list_cb: ic_table_list_count 2
        test_get_ic_list_cb: entry 0
            ic_type: 3
            index 0, ic 83:fe:d3:40:7a:93:97:23:a5:c6:39:b2:69:16:d5:05, device_address 00:00:00:01:00:00:00:01

        test_get_ic_list_cb: entry 1
            ic_type: 3
            index 1, ic 83:fe:d3:40:7a:93:97:23:a5:c6:39:b2:69:16:d5:05, device_address 00:00:00:01:00:00:00:02

        ===========================

        test_get_ic_list_cb: status 0
        test_get_ic_list_cb: ic_table_entries 5
        test_get_ic_list_cb: start_index 2
        test_get_ic_list_cb: ic_table_list_count 2
        test_get_ic_list_cb: entry 0
            ic_type: 3
            index 0, ic 83:fe:d3:40:7a:93:97:23:a5:c6:39:b2:69:16:d5:05, device_address 00:00:00:01:00:00:00:03

        test_get_ic_list_cb: entry 1
            ic_type: 3
            index 1, ic 83:fe:d3:40:7a:93:97:23:a5:c6:39:b2:69:16:d5:05, device_address 00:00:00:01:00:00:00:04

        ===========================

        test_get_ic_list_cb: status 0
        test_get_ic_list_cb: ic_table_entries 5
        test_get_ic_list_cb: start_index 4
        test_get_ic_list_cb: ic_table_list_count 1
        test_get_ic_list_cb: entry 0
            ic_type: 3
            index 0, ic 83:fe:d3:40:7a:93:97:23:a5:c6:39:b2:69:16:d5:05, device_address 00:00:00:01:00:00:00:05

    4. DUT ZC removes the first IC entry:
        test_remove_ic_by_device_idx_cb: status 0

    5. DUT ZC requests and prints IC table:

        ===========================

        test_get_ic_list_cb: status 0
        test_get_ic_list_cb: ic_table_entries 4
        test_get_ic_list_cb: start_index 0
        test_get_ic_list_cb: ic_table_list_count 2
        test_get_ic_list_cb: entry 0
            ic_type: 3
            index 0, ic 83:fe:d3:40:7a:93:97:23:a5:c6:39:b2:69:16:d5:05, device_address 00:00:00:01:00:00:00:02

        test_get_ic_list_cb: entry 1
            ic_type: 3
            index 1, ic 83:fe:d3:40:7a:93:97:23:a5:c6:39:b2:69:16:d5:05, device_address 00:00:00:01:00:00:00:03

        ===========================

        test_get_ic_list_cb: status 0
        test_get_ic_list_cb: ic_table_entries 4
        test_get_ic_list_cb: start_index 2
        test_get_ic_list_cb: ic_table_list_count 2
        test_get_ic_list_cb: entry 0
            ic_type: 3
            index 0, ic 83:fe:d3:40:7a:93:97:23:a5:c6:39:b2:69:16:d5:05, device_address 00:00:00:01:00:00:00:04

        test_get_ic_list_cb: entry 1
            ic_type: 3
            index 1, ic 83:fe:d3:40:7a:93:97:23:a5:c6:39:b2:69:16:d5:05, device_address 00:00:00:01:00:00:00:05

    6. DUT ZC removes all IC entries:
        test_remove_all_ic_cb: status 0

    7. DUT ZC requests and prints IC table:
        test_get_ic_list_cb: status 0
        test_get_ic_list_cb: ic_table_entries 0
        test_get_ic_list_cb: start_index 0
        test_get_ic_list_cb: ic_table_list_count 0
