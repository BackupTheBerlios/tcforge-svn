/*
 * test-tcframfifo.c -- testsuite for TCFrameFifo code; 
 *                      everyone feel free to add more tests and improve
 *                      existing ones.
 * (C) 2008 - Francesco Romani <fromani -at- gmail -dot- com>
 *
 * This file is part of transcode, a video stream processing tool.
 *
 * transcode is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * transcode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "libtc/libtc.h"
#include "src/framebuffer.h"


/*
 * yes, that's cheesy.
 */

typedef struct tcframefifo_ TCFrameFifo;

void tc_frame_fifo_dump_status(TCFrameFifo *F, const char *tag);

int tc_frame_fifo_empty(TCFrameFifo *F);
int tc_frame_fifo_size(TCFrameFifo *F);

TCFramePtr tc_frame_fifo_pull(TCFrameFifo *F);

TCFramePtr tc_frame_fifo_get(TCFrameFifo *F);
int tc_frame_fifo_put(TCFrameFifo *F, TCFramePtr ptr);

void tc_frame_fifo_del(TCFrameFifo *F);
TCFrameFifo *tc_frame_fifo_new(int size, int sorted);


/*************************************************************************/

#define TC_TEST_BEGIN(NAME, SIZE, SORTED) \
static int tcframefifo_ ## NAME ## _test(void) \
{ \
    const char *TC_TEST_name = # NAME ; \
    const char *TC_TEST_errmsg = ""; \
    int TC_TEST_step = -1; \
    \
    TCFrameFifo *F = NULL; \
    \
    tc_log_info(__FILE__, "running test: [%s]", # NAME); \
    F = tc_frame_fifo_new((SIZE), (SORTED)); \
    if (F) {


#define TC_TEST_END \
        tc_frame_fifo_del(F); \
        return 0; \
    } \
TC_TEST_failure: \
    if (TC_TEST_step != -1) { \
        tc_log_warn(__FILE__, "FAILED test [%s] at step %i", TC_TEST_name, TC_TEST_step); \
    } \
    tc_log_warn(__FILE__, "FAILED test [%s] NOT verified: %s", TC_TEST_name, TC_TEST_errmsg); \
    return 1; \
}

#define TC_TEST_SET_STEP(STEP) do { \
    TC_TEST_step = (STEP); \
} while (0)

#define TC_TEST_UNSET_STEP do { \
    TC_TEST_step = -1; \
} while (0)

#define TC_TEST_IS_TRUE(EXPR) do { \
    int err = (EXPR); \
    if (!err) { \
        TC_TEST_errmsg = # EXPR ; \
        goto TC_TEST_failure; \
    } \
} while (0)


#define TC_RUN_TEST(NAME) \
    errors += tcframefifo_ ## NAME ## _test()

#define TCFRAMEPTR_IS_NULL(ptr) (ptr.generic == NULL)

/*************************************************************************/

enum {
    UNSORTED = 0,
    SORTED   = 1,
    FIFOSIZE = 10
};

static void init_frames(int num, frame_list_t *frames, TCFramePtr *ptrs)
{
    int i;

    for (i = 0; i < num; i++) {
        frame_list_t *cur = &(frames[i]);
        memset(cur, 0, sizeof(frame_list_t));

        cur->bufid      = i;

        ptrs[i].generic = cur;
    }
}


TC_TEST_BEGIN(U_init_empty, FIFOSIZE, UNSORTED)
    TC_TEST_IS_TRUE(tc_frame_fifo_empty(F));
    TC_TEST_IS_TRUE(tc_frame_fifo_size(F) == 0);
TC_TEST_END

TC_TEST_BEGIN(U_get1, FIFOSIZE, UNSORTED)
    TCFramePtr fp = { .generic = NULL };

    TC_TEST_IS_TRUE(tc_frame_fifo_empty(F));

    fp = tc_frame_fifo_get(F);
    TC_TEST_IS_TRUE(TCFRAMEPTR_IS_NULL(fp));
    TC_TEST_IS_TRUE(tc_frame_fifo_size(F) == 0);
TC_TEST_END


TC_TEST_BEGIN(U_put1, FIFOSIZE, UNSORTED)
    frame_list_t frame[1];
    TCFramePtr ptr[1];

    int wakeup = 0;
    init_frames(1, frame, ptr);
    
    wakeup = tc_frame_fifo_put(F, ptr[0]);
    TC_TEST_IS_TRUE(wakeup);
    TC_TEST_IS_TRUE(tc_frame_fifo_size(F) == 1);
TC_TEST_END

TC_TEST_BEGIN(U_put1_get1, FIFOSIZE, UNSORTED)
    frame_list_t frame[1];
    TCFramePtr ptr[1];
    TCFramePtr fp = { .generic = NULL };

    int wakeup = 0;
    init_frames(1, frame, ptr);
    
    wakeup = tc_frame_fifo_put(F, ptr[0]);
    TC_TEST_IS_TRUE(wakeup);
    TC_TEST_IS_TRUE(tc_frame_fifo_size(F) == 1);

    fp = tc_frame_fifo_get(F);
    TC_TEST_IS_TRUE(!TCFRAMEPTR_IS_NULL(fp));
    TC_TEST_IS_TRUE(tc_frame_fifo_size(F) == 0);
TC_TEST_END

/*************************************************************************/

TC_TEST_BEGIN(S_init_empty, FIFOSIZE, SORTED)
    TC_TEST_IS_TRUE(tc_frame_fifo_empty(F));
    TC_TEST_IS_TRUE(tc_frame_fifo_size(F) == 0);
TC_TEST_END

TC_TEST_BEGIN(S_get1, FIFOSIZE, SORTED)
    TCFramePtr fp = { .generic = NULL };

    TC_TEST_IS_TRUE(tc_frame_fifo_empty(F));

    fp = tc_frame_fifo_get(F);
    TC_TEST_IS_TRUE(TCFRAMEPTR_IS_NULL(fp));
    TC_TEST_IS_TRUE(tc_frame_fifo_size(F) == 0);
TC_TEST_END


TC_TEST_BEGIN(S_put1, FIFOSIZE, SORTED)
    frame_list_t frame[1];
    TCFramePtr ptr[1];

    int wakeup = 0;
    init_frames(1, frame, ptr);
    
    wakeup = tc_frame_fifo_put(F, ptr[0]);
    TC_TEST_IS_TRUE(wakeup);
    TC_TEST_IS_TRUE(tc_frame_fifo_size(F) == 1);
TC_TEST_END

TC_TEST_BEGIN(S_put1_get1, FIFOSIZE, SORTED)
    frame_list_t frame[1];
    TCFramePtr ptr[1];
    TCFramePtr fp = { .generic = NULL };

    int wakeup = 0;
    init_frames(1, frame, ptr);
    
    wakeup = tc_frame_fifo_put(F, ptr[0]);
    TC_TEST_IS_TRUE(wakeup);
    TC_TEST_IS_TRUE(tc_frame_fifo_size(F) == 1);

    fp = tc_frame_fifo_get(F);
    TC_TEST_IS_TRUE(!TCFRAMEPTR_IS_NULL(fp));
    TC_TEST_IS_TRUE(tc_frame_fifo_size(F) == 0);
TC_TEST_END


/*************************************************************************/

static int test_frame_fifo_all(void)
{
    int errors = 0;

    TC_RUN_TEST(U_init_empty);
    TC_RUN_TEST(U_get1);
    TC_RUN_TEST(U_put1);
    TC_RUN_TEST(U_put1_get1);

    TC_RUN_TEST(S_init_empty);
    TC_RUN_TEST(S_get1);
    TC_RUN_TEST(S_put1);
    TC_RUN_TEST(S_put1_get1);

    return errors;
}

/* stubs */
int verbose = TC_INFO;
int tc_running(void);
int tc_running(void)
{
    return TC_TRUE;
}

int main(int argc, char *argv[])
{
    int errors = 0;
    
    if (argc == 2) {
        verbose = atoi(argv[1]);
    }

    errors = test_frame_fifo_all();

    putchar('\n');
    tc_log_info(__FILE__, "test summary: %i error%s (%s)",
                errors,
                (errors > 1) ?"s" :"",
                (errors > 0) ?"FAILED" :"PASSED");
    return (errors > 0) ?1 :0;
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
