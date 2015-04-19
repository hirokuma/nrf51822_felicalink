/*
 * Copyright (c) 2012-2014, hiro99ma
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *         this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *         this list of conditions and the following disclaimer
 *         in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/**
 * @file    ble_fps.c
 *
 * I/Oサービス
 */

/**************************************************************************
 * include
 **************************************************************************/

#include <string.h>
#include "nordic_common.h"
#include "ble_srv_common.h"

#include "ble_fps.h"

#include "app_trace.h"


/**************************************************************************
 * macro
 **************************************************************************/

#define DESC_NDEF           "NDEF data"


/**************************************************************************
 * prototype
 **************************************************************************/

static void on_connect(ble_fps_t *p_fps, ble_evt_t *p_ble_evt);
static void on_disconnect(ble_fps_t *p_fps, ble_evt_t *p_ble_evt);
static void on_write(ble_fps_t *p_fps, ble_evt_t *p_ble_evt);
static uint32_t char_add_ndef(ble_fps_t *p_fps, const ble_fps_init_t *p_fps_init);


/**************************************************************************
 * public function
 **************************************************************************/

/**
 * @brief サービス初期化
 *
 * @param[in]   p_fps       サービス構造体
 * @param[in]   p_fps_init  サービス初期化構造体
 * @retval      NRF_SUCCESS 成功
 */
uint32_t ble_fps_init(ble_fps_t *p_fps, const ble_fps_init_t *p_fps_init)
{
    uint32_t   err_code;

    //ハンドラ
    p_fps->evt_handler_ndef   = p_fps_init->evt_handler_ndef;
    p_fps->conn_handle      = BLE_CONN_HANDLE_INVALID;

    //Base UUIDを登録し、UUID typeを取得
    ble_uuid128_t   base_uuid = { FPS_UUID_BASE };
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_fps->uuid_type);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    //サービス登録
    ble_uuid_t ble_uuid;
    ble_uuid.uuid = FPS_UUID_SERVICE;
    ble_uuid.type = p_fps->uuid_type;
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid, &p_fps->service_handle);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    //キャラクタリスティック登録
    err_code = char_add_ndef(p_fps, p_fps_init);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    return NRF_SUCCESS;
}


/**
 * @brief BLEイベントハンドラ
 *
 * アプリ層のBLEイベントハンドラから呼び出されることを想定している.
 *
 * @param[in]   p_fps       サービス構造体
 * @param[in]   p_ble_evt   イベント構造体
 */
void ble_fps_on_ble_evt(ble_fps_t *p_fps, ble_evt_t *p_ble_evt)
{
    switch (p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONNECTED:
        app_trace_log("BLE_GAP_EVT_CONNECTED\r\n");
        on_connect(p_fps, p_ble_evt);
        break;

    case BLE_GAP_EVT_DISCONNECTED:
        app_trace_log("BLE_GAP_EVT_DISCONNECTED\r\n");
        on_disconnect(p_fps, p_ble_evt);
        break;

    case BLE_GATTS_EVT_WRITE:
        app_trace_log("BLE_GATTS_EVT_WRITE\r\n");
        on_write(p_fps, p_ble_evt);
        break;

    default:
        // No implementation needed.
        break;
    }
}


/**
 * @brief Notification送信
 *
 * BLEの仕様上、ATT_MTU-3(nRF51822では20byte)までしか送信できない。
 * それ以上やりとりしたい場合は、Client側にRead Blob Requestしてもらうこと。
 *
 * CCCDのValueにNotificationビットが立っていない場合はNRF_ERROR_INVALID_STATEになる。
 * Write許可している場合はCentralからビットを落とされている可能性があるので注意。
 *
 * @param[in]   p_fps       サービス構造体
 * @retval      NRF_SUCCESS 成功
 */
uint32_t ble_fps_notify(ble_fps_t *p_fps, const uint8_t *p_data, uint16_t length)
{
    ble_gatts_hvx_params_t params;

    app_trace_log("ble_fps_notify\r\n");

    memset(&params, 0, sizeof(params));
    params.handle = p_fps->char_handle_ndef.value_handle;
    params.type = BLE_GATT_HVX_NOTIFICATION;    //Notification
//    params.offset = 0;
    params.p_len = &length;
    params.p_data = (uint8_t *)p_data;

    return sd_ble_gatts_hvx(p_fps->conn_handle, &params);
}


/**************************************************************************
 * private function
 **************************************************************************/

/**
 * @brief CONNECT時
 *
 * @param[in]   p_fps       サービス構造体
 * @param[in]   p_ble_evt   イベント構造体
 */
static void on_connect(ble_fps_t *p_fps, ble_evt_t *p_ble_evt)
{
    p_fps->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**
 * @brief DISCONNECT時
 *
 * @param[in]   p_fps       サービス構造体
 * @param[in]   p_ble_evt   イベント構造体
 */
static void on_disconnect(ble_fps_t *p_fps, ble_evt_t *p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_fps->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/******************************************************************
 * Characteristic
 ******************************************************************/

/**
 * @brief Write時
 *
 * @param[in]   p_fps       サービス構造体
 * @param[in]   p_ble_evt   イベント構造体
 */
static void on_write(ble_fps_t *p_fps, ble_evt_t *p_ble_evt)
{
    ble_gatts_evt_write_t *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if ((p_evt_write->handle == p_fps->char_handle_ndef.value_handle) &&
      (p_fps->evt_handler_ndef != NULL)) {
        //callback
        p_fps->evt_handler_ndef(p_fps, p_evt_write->data, p_evt_write->len);
    }

    //Notify/Indicate有り、かつCCCD書込み可で、書込みをチェックしたい場合
#if 1
    if ((p_evt_write->handle == p_fps->char_handle_ndef.cccd_handle) &&
        (p_evt_write->len == 2)) {
        // CCCDへの書込みが発生(Notify/Indicateの許可ビット変化)
        if (p_fps->evt_handler_ndef != NULL) {
            if (ble_srv_is_notification_enabled(p_evt_write->data)) {
                //通知可能になった場合の処理
            }
            else {
                //通知不可になった場合の処理
            }
        }
    }
#endif
}


/**
 * @brief キャラクタリスティック登録：Input
 *
 *      permission : Write
 *
 * @param[in/out]   p_fps       サービス構造体
 * @param[in]       p_fps_init  サービス初期化構造体
 */
static uint32_t char_add_ndef(ble_fps_t *p_fps, const ble_fps_init_t *p_fps_init)
{
    ble_gatts_char_md_t char_md;
    ble_uuid_t          char_uuid;
    ble_gatts_attr_md_t attr_md;
    ble_gatts_attr_t    attr_char_value;
    ble_gatts_attr_md_t cccd_md;
//    ble_gatts_attr_md_t char_ud_md;

    ///////////////////////
    //Characteristicの設定
    ///////////////////////

    // CCCD(Notify/Indicate用)
    memset(&cccd_md, 0, sizeof(cccd_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    // メタデータ
    //      Read/Write without Res./Notify
    memset(&char_md, 0, sizeof(char_md));
    //property(アクセス方法)
    char_md.char_props.read   = 1;
    char_md.char_props.write  = 1;
    char_md.char_props.notify = 1;
    char_md.p_cccd_md         = &cccd_md;
    //user description
    char_md.p_char_user_desc        = (uint8_t *)DESC_NDEF;
    char_md.char_user_desc_size     = (uint8_t)strlen(DESC_NDEF);
    char_md.char_user_desc_max_size = char_md.char_user_desc_size;

    // UUID
    char_uuid.type = p_fps->uuid_type;
    char_uuid.uuid = FPS_UUID_CHAR_NDEF;


    ///////////////////////
    // Attributeの設定
    ///////////////////////

    // メタデータ
    //Readable/Writable
    memset(&attr_md, 0, sizeof(attr_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
//    attr_md.vlen       = 0;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
//    attr_md.rd_auth    = 0;       //0:without response
//    attr_md.wr_auth    = 0;       //0:without response

    // value
    memset(&attr_char_value, 0, sizeof(attr_char_value));
    attr_char_value.p_uuid       = &char_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = 10;
//    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = 128;
//    attr_char_value.p_value      = NULL;


    ///////////////////////
    // キャラクタリスティックの登録
    return sd_ble_gatts_characteristic_add(p_fps->service_handle,
                                                &char_md,
                                                &attr_char_value,
                                                &p_fps->char_handle_ndef);
}
