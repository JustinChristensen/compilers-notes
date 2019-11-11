#include <stdlib.h>
#include <check.h>
#include "test_base.h"

int main(int argc, char *argv[])
{
    int number_failed;

    SRunner *sr = srunner_create(NULL);
    srunner_add_suite(sr, array_suite());
    srunner_add_suite(sr, hash_table_suite());
    srunner_add_suite(sr, intset_suite());
    srunner_add_suite(sr, btree_suite());
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
