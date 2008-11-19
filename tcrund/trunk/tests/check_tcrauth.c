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

START_TEST(test_none_init_null)
{
    int ret;
    
    ret = tcr_auth_init(TCR_AUTH_NONE, NULL);
    fail_if(ret != TCR_AUTH_ERR_CONFIG, 
	        "`None' auth engine should fail with null usersfile");

    tcr_auth_fini();
}
END_TEST

START_TEST(test_none_init_passfile)
{
    int ret;
    
    ret = tcr_auth_init(TCR_AUTH_NONE, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`None' auth engine can't fail");

    tcr_auth_fini();
}
END_TEST


START_TEST(test_none_init_passfile_wrong)
{
    int ret;
    
    ret = tcr_auth_init(TCR_AUTH_NONE, "inexistent.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`None' auth engine can't fail with invalid passfile");

    tcr_auth_fini();
}
END_TEST

START_TEST(test_none_session_nulls)
{
    TCRAuthSession *as = NULL;
    int ret = 0;
    
    ret = tcr_auth_init(TCR_AUTH_NONE, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`None' auth engine can't fail");

    as = tcr_auth_session_new(NULL, NULL, NULL, NULL);
    fail_if(as != NULL,
            "NULL parameters should lead to session failure");

    tcr_auth_fini();
}
END_TEST

START_TEST(test_none_session_nulls_errparams)
{
    TCRAuthSession *as = NULL;
    int ret, error = 0;
    
    ret = tcr_auth_init(TCR_AUTH_NONE, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`None' auth engine can't fail");

    as = tcr_auth_session_new(NULL, NULL, NULL, &error);
    fail_if(as != NULL,
            "NULL parameters should lead to session failure");
    fail_if(error != TCR_AUTH_ERR_PARAMS,
            "NULL parameters given wrong error");

    tcr_auth_fini();
}
END_TEST

START_TEST(test_none_session_all_empty_ok)
{
    TCRAuthSession *as = NULL;
    char reply[TCR_AUTH_REPLY_LEN] = { '\0' };
    int ret, error = 0;

    ret = tcr_auth_init(TCR_AUTH_NONE, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`None' auth engine can't fail");

    as = tcr_auth_session_new("", "", reply, &error);
    fail_if(as == NULL,
            "empty user/pass should succeed");
    fail_if(error != TCR_AUTH_OK,
            "returned error with valid session");

    tcr_auth_session_del(as, reply);
    tcr_auth_fini();
}
END_TEST

START_TEST(test_none_session_unknown_user)
{
    TCRAuthSession *as = NULL;
    char reply[TCR_AUTH_REPLY_LEN] = { '\0' };
    int ret, error = 0;

    ret = tcr_auth_init(TCR_AUTH_NONE, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`None' auth engine can't fail");

    as = tcr_auth_session_new("EVIL", "AreYouScared?", reply, &error);
    fail_if(as == NULL,
            "unknown user/any pass should succeed");
    fail_if(error != TCR_AUTH_OK,
            "returned error with valid session");

    tcr_auth_session_del(as, reply);
    tcr_auth_fini();
}
END_TEST

START_TEST(test_none_session_valid_user_no_pass)
{
    TCRAuthSession *as = NULL;
    char reply[TCR_AUTH_REPLY_LEN] = { '\0' };
    int ret, error = 0;

    ret = tcr_auth_init(TCR_AUTH_NONE, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`None' auth engine can't fail");

    as = tcr_auth_session_new("foo", "", reply, &error);
    fail_if(as == NULL,
            "valid user/empty pass should succeed");
    fail_if(error != TCR_AUTH_OK,
            "returned error with valid session");

    tcr_auth_session_del(as, reply);
    tcr_auth_fini();
}
END_TEST

START_TEST(test_none_session_valid_user_wrong_pass)
{
    TCRAuthSession *as = NULL;
    char reply[TCR_AUTH_REPLY_LEN] = { '\0' };
    int ret, error = 0;

    ret = tcr_auth_init(TCR_AUTH_NONE, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`None' auth engine can't fail");

    as = tcr_auth_session_new("foo", "WRONG!", reply, &error);
    fail_if(as == NULL,
            "valid user/wrong pass should succeed");
    fail_if(error != TCR_AUTH_OK,
            "returned error with valid session");

    tcr_auth_session_del(as, reply);
    tcr_auth_fini();
}
END_TEST

START_TEST(test_none_session_valid_user_valid_pass)
{
    TCRAuthSession *as = NULL;
    char reply[TCR_AUTH_REPLY_LEN] = { '\0' };
    int ret, error = 0;

    ret = tcr_auth_init(TCR_AUTH_NONE, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`None' auth engine can't fail");

    as = tcr_auth_session_new("foo", "bar", reply, &error);
    fail_if(as == NULL,
            "valid user/valid pass should succeed");
    fail_if(error != TCR_AUTH_OK,
            "returned error with valid session");

    ret = tcr_auth_session_del(as, reply);
    fail_if(ret != TCR_AUTH_OK,
            "valid session destruction failed");
    tcr_auth_fini();
}
END_TEST

/*************************************************************************/

START_TEST(test_plainpass_init_null)
{
    int ret;

    ret = tcr_auth_init(TCR_AUTH_PLAINPASS, NULL);
    fail_if(ret == TCR_AUTH_OK, 
	        "`Plainpass' auth engine must fail with null passfile (ret=%i)",
            ret);

    tcr_auth_fini();
}
END_TEST

START_TEST(test_plainpass_init_passfile)
{
    int ret;
    
    ret = tcr_auth_init(TCR_AUTH_PLAINPASS, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`Plainpass' auth engine can't fail with valid passfile");

    tcr_auth_fini();
}
END_TEST

START_TEST(test_plainpass_init_passfile_wrong)
{
    int ret;
    
    ret = tcr_auth_init(TCR_AUTH_PLAINPASS, "inexistent.cfg");
    fail_if(ret != TCR_AUTH_ERR_CONFIG, 
	        "`Plainpass' auth engine should fail with invalid passfile");

    tcr_auth_fini();
}
END_TEST


START_TEST(test_plainpass_session_nulls)
{
    TCRAuthSession *as = NULL;
    int ret = 0;
    
    ret = tcr_auth_init(TCR_AUTH_PLAINPASS, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`Plainpass' auth engine can't fail with valid passfile");

    as = tcr_auth_session_new(NULL, NULL, NULL, NULL);
    fail_if(as != NULL,
            "NULL parameters should lead to session failure");

    tcr_auth_fini();
}
END_TEST

START_TEST(test_plainpass_session_nulls_errparams)
{
    TCRAuthSession *as = NULL;
    int ret, error = 0;
    
    ret = tcr_auth_init(TCR_AUTH_PLAINPASS, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`Plainpass' auth engine can't fail with valid passfile");

    as = tcr_auth_session_new(NULL, NULL, NULL, &error);
    fail_if(as != NULL,
            "NULL parameters should lead to session failure");
    fail_if(error != TCR_AUTH_ERR_PARAMS,
            "NULL parameters given wrong error");

    tcr_auth_fini();
}
END_TEST

START_TEST(test_plainpass_session_all_empty_bad)
{
    TCRAuthSession *as = NULL;
    char reply[TCR_AUTH_REPLY_LEN] = { '\0' };
    int ret, error = 0;

    ret = tcr_auth_init(TCR_AUTH_PLAINPASS, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`Plainpass' auth engine should not fail");

    as = tcr_auth_session_new("", "", reply, &error);
    fail_if(as != NULL,
            "empty user/pass should NOT succeed!!!");
    fail_if(error != TCR_AUTH_ERR_USER,
            "returned wrong error (error=%i)", error);

    tcr_auth_session_del(as, reply);
    tcr_auth_fini();
}
END_TEST

START_TEST(test_plainpass_session_unknown_user)
{
    TCRAuthSession *as = NULL;
    char reply[TCR_AUTH_REPLY_LEN] = { '\0' };
    int ret, error = 0;

    ret = tcr_auth_init(TCR_AUTH_PLAINPASS, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`Plainpass' auth engine can't fail");

    as = tcr_auth_session_new("EVIL", "AreYouScared?", reply, &error);
    fail_if(as != NULL,
            "unknown user/any pass should fail");
    fail_if(error != TCR_AUTH_ERR_USER,
            "returned wrong error (error=%i)", error);

    tcr_auth_session_del(as, reply);
    tcr_auth_fini();
}
END_TEST

START_TEST(test_plainpass_session_valid_user_no_pass)
{
    TCRAuthSession *as = NULL;
    char reply[TCR_AUTH_REPLY_LEN] = { '\0' };
    int ret, error = 0;

    ret = tcr_auth_init(TCR_AUTH_PLAINPASS, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`Plainpass' auth engine can't fail");

    as = tcr_auth_session_new("foo", "", reply, &error);
    fail_if(as != NULL,
            "valid user/empty pass should fail");
    fail_if(error != TCR_AUTH_ERR_USER,
            "returned wrong error (error=%i)", error);

    tcr_auth_session_del(as, reply);
    tcr_auth_fini();
}
END_TEST

START_TEST(test_plainpass_session_valid_user_wrong_pass)
{
    TCRAuthSession *as = NULL;
    char reply[TCR_AUTH_REPLY_LEN] = { '\0' };
    int ret, error = 0;

    ret = tcr_auth_init(TCR_AUTH_PLAINPASS, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`Plainpass' auth engine can't fail");

    as = tcr_auth_session_new("foo", "WRONG!", reply, &error);
    fail_if(as != NULL,
            "valid user/empty pass should fail");
    fail_if(error != TCR_AUTH_ERR_USER,
            "returned wrong error (error=%i)", error);

    tcr_auth_session_del(as, reply);
    tcr_auth_fini();
}
END_TEST

START_TEST(test_plainpass_session_valid_user_valid_pass)
{
    TCRAuthSession *as = NULL;
    char reply[TCR_AUTH_REPLY_LEN] = { '\0' };
    int ret, error = 0;

    ret = tcr_auth_init(TCR_AUTH_PLAINPASS, "samplepass.cfg");
    fail_if(ret != TCR_AUTH_OK, 
	        "`Plainpass' auth engine can't fail");

    as = tcr_auth_session_new("foo", "bar", reply, &error);
    fail_if(as == NULL,
            "valid user/valid pass should succeed");
    fail_if(error != TCR_AUTH_OK,
            "returned error with valid session");

    ret = tcr_auth_session_del(as, reply);
    fail_if(ret != TCR_AUTH_OK,
            "valid session destruction failed");
    tcr_auth_fini();
}
END_TEST

/*************************************************************************/

static Suite *tcrauth_suite(void)
{
    Suite *s = suite_create("tcrauth");

    TCase *tc_none = tcase_create("none");
    tcase_add_test(tc_none, test_none_init_null);
    tcase_add_test(tc_none, test_none_init_passfile);
    tcase_add_test(tc_none, test_none_init_passfile_wrong);
    tcase_add_test(tc_none, test_none_session_nulls);
    tcase_add_test(tc_none, test_none_session_nulls_errparams);
    tcase_add_test(tc_none, test_none_session_all_empty_ok);
    tcase_add_test(tc_none, test_none_session_unknown_user);
    tcase_add_test(tc_none, test_none_session_valid_user_no_pass);
    tcase_add_test(tc_none, test_none_session_valid_user_wrong_pass);
    tcase_add_test(tc_none, test_none_session_valid_user_valid_pass);

    suite_add_tcase(s, tc_none);

    TCase *tc_plainpass = tcase_create ("plainpass");
    tcase_add_test(tc_none, test_plainpass_init_null);
    tcase_add_test(tc_none, test_plainpass_init_passfile);
    tcase_add_test(tc_none, test_plainpass_init_passfile_wrong);
    tcase_add_test(tc_none, test_plainpass_session_nulls);
    tcase_add_test(tc_none, test_plainpass_session_nulls_errparams);
    tcase_add_test(tc_none, test_plainpass_session_all_empty_bad);
    tcase_add_test(tc_none, test_plainpass_session_unknown_user);
    tcase_add_test(tc_none, test_plainpass_session_valid_user_no_pass);
    tcase_add_test(tc_none, test_plainpass_session_valid_user_wrong_pass);
    tcase_add_test(tc_none, test_plainpass_session_valid_user_valid_pass);


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

