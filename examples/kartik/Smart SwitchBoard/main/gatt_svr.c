/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "ble_spp_server.h"

/* 16 Bit Alert Notification Service UUID */
#define BLE_SVC_ANS_UUID16                                  0x1811

/* 16 Bit Alert Notification Service Characteristic UUIDs */
#define BLE_SVC_ANS_CHR_UUID16_SUP_NEW_ALERT_CAT            0x2a47
#define BLE_SVC_ANS_CHR_UUID16_NEW_ALERT                    0x2a46
//#define BLE_SVC_ANS_CHR_UUID16_SUP_UNR_ALERT_CAT          0x2a48
#define BLE_SVC_ANS_CHR_UUID16_UNR_ALERT_STAT               0x2a45
#define BLE_SVC_ANS_CHR_UUID16_ALERT_NOT_CTRL_PT            0x2a44

#define BLE_SVC_SPP_SYSTEM_UUID16                           0x2a48

#define BLE_SVC_SPP_CURRENT_STATUS_UUID16                   0x2a49

#define BLE_SVC_SPP_DEVICE_STATE_UUID16                     0x2a50

#define BLE_SVC_SPP_ONOFF_STATE_UUID16                      0x2a51

/**
 * The vendor specific security test service consists of two characteristics:
 *     o random-number-generator: generates a random 32-bit number each time
 *       it is read.  This characteristic can only be read over an encrypted
 *       connection.
 *     o static-value: a single-byte characteristic that can always be read,
 *       but can only be written over an encrypted connection.
 */

static uint8_t gatt_svr_sec_test_static_val;

// static int
// gatt_svr_chr_access_sec_test(uint16_t conn_handle, uint16_t attr_handle,
//                              struct ble_gatt_access_ctxt *ctxt,
//                              void *arg);

static int
gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                   void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

// /*/*static int
// gatt_svr_chr_access_sec_test(uint16_t conn_handle, uint16_t attr_handle,
//                              struct ble_gatt_access_ctxt *ctxt,
//                              void *arg)
// {
//     const ble_uuid_t *uuid;
//     int rand_num;
//     int rc;

//     uuid = ctxt->chr->uuid;

//     /* Determine which characteristic is being accessed by examining its
//      * 128-bit UUID.
//      */

//     if (ble_uuid_cmp(uuid, &gatt_svr_chr_sec_test_rand_uuid.u) == 0) {
//         assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);

//          Respond with a 32-bit random number. 
//         rand_num = rand();
//         rc = os_mbuf_append(ctxt->om, &rand_num, sizeof rand_num);
//         return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
//     }

//     if (ble_uuid_cmp(uuid, &gatt_svr_chr_sec_test_static_uuid.u) == 0) {
//         switch (ctxt->op) {
//         case BLE_GATT_ACCESS_OP_READ_CHR:
//             rc = os_mbuf_append(ctxt->om, &gatt_svr_sec_test_static_val,
//                                 sizeof gatt_svr_sec_test_static_val);
//             return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

//         case BLE_GATT_ACCESS_OP_WRITE_CHR:
//             rc = gatt_svr_chr_write(ctxt->om,
//                                     sizeof gatt_svr_sec_test_static_val,
//                                     sizeof gatt_svr_sec_test_static_val,
//                                     &gatt_svr_sec_test_static_val, NULL);
//             return rc;

//         default:
//             assert(0);
//             return BLE_ATT_ERR_UNLIKELY;
//         }
//     }

//     /* Unknown characteristic; the nimble stack should not have called this
//      * function.
//      */
//     assert(0);
//     return BLE_ATT_ERR_UNLIKELY;
// }*/*/

void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(DEBUG, "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int
new_gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();
/*
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }*/

    return 0;
}
