/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <string.h>
#include <errno.h>
#include "testutil/testutil.h"
#include "host/ble_uuid.h"
#include "host/ble_hs_test.h"
#include "ble_hs_test_util.h"

#define BLE_GATTS_READ_TEST_CHR_1_UUID    0x1111
#define BLE_GATTS_READ_TEST_CHR_2_UUID    0x2222

static uint8_t ble_gatts_read_test_peer_addr[6] = {2,3,4,5,6,7};

static int
ble_gatts_read_test_util_access_1(uint16_t conn_handle,
                                  uint16_t attr_handle, uint8_t op,
                                  struct ble_gatt_access_ctxt *ctxt,
                                  void *arg);

static int
ble_gatts_read_test_util_access_2(uint16_t conn_handle,
                                  uint16_t attr_handle, uint8_t op,
                                  struct ble_gatt_access_ctxt *ctxt,
                                  void *arg);
static void
ble_gatts_read_test_misc_reg_cb(uint8_t op,
                                union ble_gatt_register_ctxt *ctxt,
                                void *arg);

static const struct ble_gatt_svc_def ble_gatts_read_test_svcs[] = { {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid128 = BLE_UUID16(0x1234),
    .characteristics = (struct ble_gatt_chr_def[]) { {
        .uuid128 = BLE_UUID16(BLE_GATTS_READ_TEST_CHR_1_UUID),
        .access_cb = ble_gatts_read_test_util_access_1,
        .flags = BLE_GATT_CHR_F_READ
    }, {
        .uuid128 = BLE_UUID16(BLE_GATTS_READ_TEST_CHR_2_UUID),
        .access_cb = ble_gatts_read_test_util_access_2,
        .flags = BLE_GATT_CHR_F_READ
    }, {
        0
    } },
}, {
    0
} };


static uint16_t ble_gatts_read_test_chr_1_def_handle;
static uint16_t ble_gatts_read_test_chr_1_val_handle;
static uint8_t ble_gatts_read_test_chr_1_val[1024];
static int ble_gatts_read_test_chr_1_len;
static uint16_t ble_gatts_read_test_chr_2_def_handle;
static uint16_t ble_gatts_read_test_chr_2_val_handle;

static void
ble_gatts_read_test_misc_init(uint16_t *out_conn_handle)
{
    int rc;

    ble_hs_test_util_init();

    rc = ble_gatts_register_svcs(ble_gatts_read_test_svcs,
                                 ble_gatts_read_test_misc_reg_cb, NULL);
    TEST_ASSERT_FATAL(rc == 0);
    TEST_ASSERT_FATAL(ble_gatts_read_test_chr_1_def_handle != 0);
    TEST_ASSERT_FATAL(ble_gatts_read_test_chr_1_val_handle ==
                      ble_gatts_read_test_chr_1_def_handle + 1);
    TEST_ASSERT_FATAL(ble_gatts_read_test_chr_2_def_handle != 0);
    TEST_ASSERT_FATAL(ble_gatts_read_test_chr_2_val_handle ==
                      ble_gatts_read_test_chr_2_def_handle + 1);

    ble_gatts_start();

    ble_hs_test_util_create_conn(2, ble_gatts_read_test_peer_addr, NULL, NULL);

    if (out_conn_handle != NULL) {
        *out_conn_handle = 2;
    }
}

static void
ble_gatts_read_test_misc_reg_cb(uint8_t op,
                                union ble_gatt_register_ctxt *ctxt,
                                void *arg)
{
    uint16_t uuid16;

    if (op == BLE_GATT_REGISTER_OP_CHR) {
        uuid16 = ble_uuid_128_to_16(ctxt->chr.chr_def->uuid128);
        switch (uuid16) {
        case BLE_GATTS_READ_TEST_CHR_1_UUID:
            ble_gatts_read_test_chr_1_def_handle = ctxt->chr.def_handle;
            ble_gatts_read_test_chr_1_val_handle = ctxt->chr.val_handle;
            break;

        case BLE_GATTS_READ_TEST_CHR_2_UUID:
            ble_gatts_read_test_chr_2_def_handle = ctxt->chr.def_handle;
            ble_gatts_read_test_chr_2_val_handle = ctxt->chr.val_handle;
            break;

        default:
            TEST_ASSERT_FATAL(0);
            break;
        }
    }
}

static int
ble_gatts_read_test_util_access_1(uint16_t conn_handle,
                                  uint16_t attr_handle, uint8_t op,
                                  struct ble_gatt_access_ctxt *ctxt,
                                  void *arg)
{
    TEST_ASSERT_FATAL(op == BLE_GATT_ACCESS_OP_READ_CHR);
    TEST_ASSERT_FATAL(attr_handle == ble_gatts_read_test_chr_1_val_handle);

    TEST_ASSERT(ctxt->chr ==
                &ble_gatts_read_test_svcs[0].characteristics[0]);
    ctxt->att->read.data = ble_gatts_read_test_chr_1_val;
    ctxt->att->read.len = ble_gatts_read_test_chr_1_len;

    return 0;
}

static int
ble_gatts_read_test_util_access_2(uint16_t conn_handle,
                                  uint16_t attr_handle, uint8_t op,
                                  struct ble_gatt_access_ctxt *ctxt,
                                  void *arg)
{
    TEST_ASSERT_FATAL(op == BLE_GATT_ACCESS_OP_READ_CHR);
    TEST_ASSERT_FATAL(attr_handle == ble_gatts_read_test_chr_2_def_handle + 1);

    TEST_ASSERT(ctxt->chr ==
                &ble_gatts_read_test_svcs[0].characteristics[1]);

    TEST_ASSERT_FATAL(ctxt->att->read.data == ctxt->att->read.buf);
    TEST_ASSERT(ctxt->att->read.max_data_len == BLE_ATT_MTU_DFLT - 1);

    ctxt->att->read.buf[0] = 0;
    ctxt->att->read.buf[1] = 10;
    ctxt->att->read.buf[2] = 20;
    ctxt->att->read.buf[3] = 30;
    ctxt->att->read.buf[4] = 40;
    ctxt->att->read.buf[5] = 50;
    ctxt->att->read.len = 6;

    return 0;
}

static void
ble_gatts_read_test_once(uint16_t conn_handle, uint16_t attr_id,
                         void *expected_value, uint16_t expected_len)
{
    struct ble_att_read_req read_req;
    uint8_t buf[BLE_ATT_READ_REQ_SZ];
    int rc;

    read_req.barq_handle = attr_id;
    ble_att_read_req_write(buf, sizeof buf, &read_req);

    rc = ble_hs_test_util_l2cap_rx_payload_flat(conn_handle, BLE_L2CAP_CID_ATT,
                                                buf, sizeof buf);
    TEST_ASSERT(rc == 0);

    ble_hs_test_util_verify_tx_read_rsp(expected_value, expected_len);
}

TEST_CASE(ble_gatts_read_test_case_basic)
{
    uint16_t conn_handle;

    ble_gatts_read_test_misc_init(&conn_handle);

    /*** Application points attribute at static data. */
    ble_gatts_read_test_chr_1_val[0] = 1;
    ble_gatts_read_test_chr_1_val[1] = 2;
    ble_gatts_read_test_chr_1_val[2] = 3;
    ble_gatts_read_test_chr_1_len = 3;
    ble_gatts_read_test_once(conn_handle,
                             ble_gatts_read_test_chr_1_val_handle,
                             ble_gatts_read_test_chr_1_val,
                             ble_gatts_read_test_chr_1_len);

    /*** Application uses stack-provided buffer for dynamic attribute. */
    ble_gatts_read_test_once(conn_handle,
                             ble_gatts_read_test_chr_2_def_handle + 1,
                             ((uint8_t[6]){0,10,20,30,40,50}), 6);

}

TEST_SUITE(ble_gatts_read_test_suite)
{
    ble_gatts_read_test_case_basic();
}
