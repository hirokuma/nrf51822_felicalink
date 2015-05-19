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
 * @file    ble_fps.h
 *
 * FeliCa Plugサービス
 */
#ifndef BLE_FPS_H__
#define BLE_FPS_H__

/**************************************************************************
 * include
 **************************************************************************/

#include "ble.h"


/**************************************************************************
 * macro
 **************************************************************************/

//8F82xxxx-7326-78CF-E5B0-F514E62BC594
//                                                                                  xxxxxxxxx
#define FPS_UUID_BASE { 0x94,0xC5,0x2B,0xE6,0x14,0xF5,0xB0,0xE5,0xCF,0x78,0x26,0x73,0x00,0x00,0x82,0x8F }
#define FPS_UUID_SERVICE        (0x5500)
#define FPS_UUID_CHAR_NDEF      (0x5501)
#define FPS_UUID_CHAR_READ      (0x5502)
#define FPS_UUID_CHAR_WRITE     (0x5503)


/**************************************************************************
 * definition
 **************************************************************************/

// Forward declaration of the ble_fps_t type.
typedef struct ble_fps_t ble_fps_t;


/**
 * @brief サービスイベントハンドラ
 *
 * @param[in]   p_fps   I/Oサービス構造体
 * @param[in]   p_value 受信バッファ
 * @param[in]   length  受信データ長
 */
typedef void (*ble_fps_evt_handler_t) (ble_fps_t *p_fps, const uint8_t *p_value, uint16_t length);


/**@brief サービス初期化構造体 */
typedef struct ble_fps_init_t {
    ble_fps_evt_handler_t           evt_handler_ndef;             /**< イベントハンドラ : Notify発生 */
} ble_fps_init_t;


/**@brief サービス構造体 */
typedef struct ble_fps_t {
    uint16_t                        service_handle;             /**< Handle of I/O Service (as provided by the BLE stack). */
    uint16_t                        conn_handle;                /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    uint8_t                         uuid_type;
    //
    ble_gatts_char_handles_t        char_handle_ndef;           /**< Handles related to the Input characteristic. */
    ble_fps_evt_handler_t           evt_handler_ndef;           /**< Event handler to be called for handling events in the I/O Service. */
} ble_fps_t;


/**************************************************************************
 * prototype
 **************************************************************************/

/**@brief サービス初期化
 *
 * @param[in]   p_fps       サービス構造体
 * @param[in]   p_fps_init  サービス初期化構造体
 * @retval      NRF_SUCCESS 成功
 */
uint32_t ble_fps_init(ble_fps_t *p_fps, const ble_fps_init_t *p_fps_init);


/**@brief BLEイベントハンドラ
 * アプリ層のBLEイベントハンドラから呼び出されることを想定している.
 *
 * @param[in]   p_fps       サービス構造体
 * @param[in]   p_ble_evt   イベント構造体
 */
void ble_fps_on_ble_evt(ble_fps_t *p_fps, ble_evt_t *p_ble_evt);


/**@brief Notify送信
 *
 * @param[in]   p_fps       サービス構造体
 * @param[in]   p_data      データ
 * @param[in]   length      データ長(21byte以上は20byteにされる)
 * @retval      NRF_SUCCESS 成功
 */
uint32_t ble_fps_notify(ble_fps_t *p_fps, const uint8_t *p_data, uint16_t length);

#endif // BLE_FPS_H__

