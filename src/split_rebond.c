/*
 * Split central: recover when the split peripheral has lost its bond with us
 * (e.g. after a settings reset on that half).
 *
 * The peripheral then answers our encryption request with "PIN or key
 * missing", the link never encrypts, our subscriptions are dropped and none
 * of its key presses arrive. Deleting our stale keys for it makes the next
 * connection pair from scratch, which the peripheral accepts.
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(split_rebond, LOG_LEVEL_INF);

static bt_addr_le_t stale_peer;

static void unpair_stale_peer(struct k_work *work) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(&stale_peer, addr, sizeof(addr));

    // Also disconnects it; the split central then reconnects and pairs again.
    int err = bt_unpair(BT_ID_DEFAULT, &stale_peer);
    LOG_WRN("Peripheral %s lost its bond, cleared ours so it re-pairs (err %d)", addr, err);
}

static K_WORK_DEFINE(unpair_work, unpair_stale_peer);

static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err) {
    struct bt_conn_info info;

    if (err != BT_SECURITY_ERR_PIN_OR_KEY_MISSING) {
        return;
    }

    // We are central only towards our split peripherals; host links are left alone.
    if (bt_conn_get_info(conn, &info) || info.role != BT_CONN_ROLE_CENTRAL) {
        return;
    }

    bt_addr_le_copy(&stale_peer, bt_conn_get_dst(conn));
    k_work_submit(&unpair_work);
}

BT_CONN_CB_DEFINE(split_rebond_conn_callbacks) = {
    .security_changed = security_changed,
};
