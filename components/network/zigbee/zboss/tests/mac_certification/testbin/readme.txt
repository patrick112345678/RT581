New tests addition or modification HowTo:

1. Delete ZB_TEST_NAME in the test source code
2. Run testbin/genlist.sh
3. Check files srcs_list, tests_list and mac_tests_table.h for update.
4. Check sources.
5. Run make
6. Commit updated source files and srcs_list, tests_list, mac_tests_table.h.

No need to run genlist.sh every time!