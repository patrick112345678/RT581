

sdk_generate_library(ruci)

sdk_add_include_directories(
    include
)

sdk_library_add_sources(
    Src/ruci.c
    Src/ruci_cmn_event.c
    Src/ruci_cmn_hal_cmd.c
    Src/ruci_cmn_sys_cmd.c
    Src/ruci_pci15p4_mac_cmd.c
    Src/ruci_pci_ble_cmd.c
    Src/ruci_pci_common_cmd.c
    Src/ruci_pci_data.c
    Src/ruci_pci_event.c
    Src/ruci_pci_fsk_cmd.c
    Src/ruci_pci_oqpsk_cmd.c
    Src/ruci_pci_slink_cmd.c
    Src/ruci_sf_event.c
    Src/ruci_sf_host_cmd.c
    Src/ruci_sf_host_event.c
    Src/ruci_sf_host_sys_cmd.c
)