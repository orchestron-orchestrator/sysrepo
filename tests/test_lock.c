/**
 * @file test_lock.c
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @brief test for sysrepo API and internal mod locks
 *
 * @copyright
 * Copyright (c) 2019 CESNET, z.s.p.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _GNU_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdarg.h>

#include <cmocka.h>
#include <libyang/libyang.h>

#include "tests/config.h"
#include "sysrepo.h"

struct state {
    sr_conn_ctx_t *conn;
};

static int
setup(void **state)
{
    struct state *st;

    st = calloc(1, sizeof *st);
    *state = st;

    if (sr_connect("test1", 0, &(st->conn)) != SR_ERR_OK) {
        return 1;
    }

    if (sr_install_module(st->conn, TESTS_DIR "/files/test.yang", TESTS_DIR "/files", NULL, 0) != SR_ERR_OK) {
        return 1;
    }
    if (sr_install_module(st->conn, TESTS_DIR "/files/ietf-interfaces.yang", TESTS_DIR "/files", NULL, 0) != SR_ERR_OK) {
        return 1;
    }
    if (sr_install_module(st->conn, TESTS_DIR "/files/iana-if-type.yang", TESTS_DIR "/files", NULL, 0) != SR_ERR_OK) {
        return 1;
    }
    if (sr_install_module(st->conn, TESTS_DIR "/files/when1.yang", TESTS_DIR "/files", NULL, 0) != SR_ERR_OK) {
        return 1;
    }
    if (sr_install_module(st->conn, TESTS_DIR "/files/when2.yang", TESTS_DIR "/files", NULL, 0) != SR_ERR_OK) {
        return 1;
    }

    return 0;
}

static int
teardown(void **state)
{
    struct state *st = (struct state *)*state;

    sr_remove_module(st->conn, "when2");
    sr_remove_module(st->conn, "when1");
    sr_remove_module(st->conn, "ietf-interfaces");
    sr_remove_module(st->conn, "iana-if-type");
    sr_remove_module(st->conn, "test");

    sr_disconnect(st->conn);
    free(st);
    return 0;
}

/* TEST 1 */
static void
test_one_session(void **state)
{
    struct state *st = (struct state *)*state;
    sr_session_ctx_t *sess;
    int ret;

    ret = sr_session_start(st->conn, SR_DS_RUNNING, &sess);
    assert_int_equal(ret, SR_ERR_OK);

    /* lock all modules */
    ret = sr_lock(sess, NULL);
    assert_int_equal(ret, SR_ERR_OK);

    /* try to lock all modules again */
    ret = sr_lock(sess, NULL);
    assert_int_equal(ret, SR_ERR_LOCKED);

    /* try to lock already locked module */
    ret = sr_lock(sess, "test");
    assert_int_equal(ret, SR_ERR_LOCKED);

    /* unlock all modules */
    ret = sr_unlock(sess, NULL);
    assert_int_equal(ret, SR_ERR_OK);

    /* lock a module */
    ret = sr_lock(sess, "test");
    assert_int_equal(ret, SR_ERR_OK);

    /* lock another module */
    ret = sr_lock(sess, "when1");
    assert_int_equal(ret, SR_ERR_OK);

    /* try to unlock a non-locked module */
    ret = sr_unlock(sess, "when2");
    assert_int_equal(ret, SR_ERR_OPERATION_FAILED);

    /* try to lock all modules */
    ret = sr_lock(sess, NULL);
    assert_int_equal(ret, SR_ERR_LOCKED);

    /* try to unlock all modules */
    ret = sr_unlock(sess, NULL);
    assert_int_equal(ret, SR_ERR_OPERATION_FAILED);

    /* unlock a locked module */
    ret = sr_unlock(sess, "test");
    assert_int_equal(ret, SR_ERR_OK);

    /* unlock last locked module */
    ret = sr_unlock(sess, "when1");
    assert_int_equal(ret, SR_ERR_OK);

    sr_session_stop(sess);
}

/* TEST 2 */
static void
test_two_sessions(void **state)
{
    struct state *st = (struct state *)*state;
    sr_session_ctx_t *sess1, *sess2;
    struct lyd_node *subtree;
    int ret;

    ret = sr_session_start(st->conn, SR_DS_RUNNING, &sess1);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_session_start(st->conn, SR_DS_RUNNING, &sess2);
    assert_int_equal(ret, SR_ERR_OK);

    /* lock all modules */
    ret = sr_lock(sess1, NULL);
    assert_int_equal(ret, SR_ERR_OK);

    /* try to lock all modules again */
    ret = sr_lock(sess2, NULL);
    assert_int_equal(ret, SR_ERR_LOCKED);

    /* read some data while the module is locked */
    ret = sr_get_subtree(sess2, "/test:cont", &subtree);
    assert_int_equal(ret, SR_ERR_OK);
    lyd_free_withsiblings(subtree);

    /* unlock all modules */
    ret = sr_unlock(sess1, NULL);
    assert_int_equal(ret, SR_ERR_OK);

    sr_session_stop(sess1);
    sr_session_stop(sess2);
}

/* MAIN */
int
main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_one_session),
        cmocka_unit_test(test_two_sessions),
    };

    sr_log_stderr(SR_LL_INF);
    return cmocka_run_group_tests(tests, setup, teardown);
}
