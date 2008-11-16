/*
 * test_tcrund.c -- tests for tcrund's auth facilities
 * (C) 2008 Francesco Romani <fromani at gmail dot com>
 *
 * This file is part of tcrund, the transcode remote control daemon.
 *
 * tcrund is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * tcrund is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include <check.h>

#include "config.h"

#include "tcrauth.h"
#include "tcrund.h"


/*************************************************************************/

START_TEST(test_none_init_fini)
{
    int ret = tcr_auth_init(TCR_AUTH_NONE, NULL);
    fail_if(ret != TCR_AUTH_OK, 
	        "`None' auth engine can't fail");
    tcr_auth_fini();
}
END_TEST

/*************************************************************************/

static Suite *tcrauth_suite(void)
{
    Suite *s = suite_create("tcrauth");

    TCase *tc_none = tcase_create("none");
    tcase_add_test(tc_none, test_none_init_fini);
    suite_add_tcase(s, tc_none);

    TCase *tc_plainpass = tcase_create ("plainpass");
    suite_add_tcase(s, tc_plainpass);

    return s;
}

/*************************************************************************/

int main(int argc, char *argv[])
{
    int number_failed = 0;

    Suite *s = tcrauth_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*************************************************************************/
/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */

