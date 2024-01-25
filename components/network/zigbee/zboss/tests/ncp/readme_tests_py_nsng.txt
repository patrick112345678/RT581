    How to run tests under NSNG

Please add a description for your test if it requires special
conditions for successful completion.

    First, run build/linux-nsng-ncp-dev-py/build_kernel.sh from platform_linux_nsng repo.

    It may be required https://gitlab.dsr-corporation.com/zboss/internal_key_storage
    Don't distribute these certificates!


zbs_rejoin_zed_secure_rejoin.py
zbs_rejoin_zed_unsecure_rejoin.py
zbs_rejoin_zr_secure_rejoin.py
zbs_rejoin_zr_unsecure_rejoin.py

    copy ncp_sample_cfg.prod to ncp/high_level/dev/ncpfw/ncp_fw.prod
    or make symbolic link
    standalone side: tests/ncp/ncp_feature_tests/zbs_test_desc/ 
    no .prod file required on standalone side

    remove *.nvram files in ncp_fw and standalone dirs

    command sequence:
    nsng
    zbs_test_desc_zc
    python3 zbs_NNN.py
    ncp_fw

    to stop the test, shutdown nsng, other parties automatically die

    Errors:
        'not authorized' - check .prod files on right places and restart the test


zbs_address_update_ze.py

    copy ncp_sample_cfg.prod to ncp/high_level/dev/ncpfw/ncp_fw.prod
    or make symbolic link
    standalone side: tests/ncp/ncp_feature_tests/zbs_test_conflict/ 
    no .prod file required on standalone side

    remove *.nvram files in ncp_fw and standalone dirs

    command sequence:
    nsng
    zbs_test_conflict_zc
    python3 zbs_address_update_ze.py
    ncp_fw


zbs_se_zed_cs2_read_time.py
zbs_se_zr_cs2_read_time.py

    Standalone side: application/samples/se/energy_service_interface/esi_device_ncp

    Check application/samples/se/common/se_common.h - it must be defined SE_CRYPTOSUITE_2
    Attention! So that the necessary configuration files are in place,
    make standalone side after run platform/build/linux-nsng-ncp-dev-py/build_kernel.sh

    copy esi_cs2.prod to application/samples/se/energy_service_interface/esi_device.prod
    or make symbolic link
    Attention! The file naming rule is different from the usual one,
    .prod is esi_device.prod, and the executable are esi_device_ncp

    remove *.nvram files in ncp_fw and standalone dirs

    command sequence:
    nsng
    esi_device_ncp
    python3 zbs_se_zr_cs2_read_time.py (or python3 zbs_se_zed_cs2_read_time.py)
    ncp_fw


zbs_se_key_est_zr_cs1.py
zbs_se_key_est_zr_cs2.py

    Three-way tests

    Standalone side ZC:  application/samples/se/energy_service_interface/esi_device_ncp
    Standalone side ZED: application/samples/se/metering/metering_device_ncp

    Check application/samples/se/common/se_common.h -
        it must be defined SE_CRYPTOSUITE_1 for zbs_se_key_est_zr_cs1.py test,
        and SE_CRYPTOSUITE_2 for zbs_se_key_est_zr_cs2.py test
    and rebuild a standalone applications

    copy esi_cs1_cs2.prod to application/samples/se/energy_service_interface/esi_device.prod
    or make symbolic link
    copy elmet_cs1_cs2.prod to application/samples/se/metering/metering_device.prod
    or make symbolic link

    Attention! The file naming rule is different from the usual one,
    .prod is *_device.prod, and the executable are *_device_ncp

    remove *.nvram files in ncp_fw and standalones dirs

    command sequence:
    nsng
    esi_device_ncp
    python3 zbs_se_key_est_zr_cs1.py (or python3 zbs_se_key_est_zr_cs2.py)
    ncp_fw
    metering_device_ncp

zbs_unauth_aps_keys_zr.py

    Standalone side: tests/ncp/ncp_feature_tests/zbs_diagnostics_tests/zbs_diag_unauth_aps_keys
    .prod file for NCP is ihd_cs1_cs2.prod
    .prod file for ZC is esi_cs1_cs2.prod
