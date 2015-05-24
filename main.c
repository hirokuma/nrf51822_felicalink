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

/**************************************************************************
 * include
 **************************************************************************/

#include <string.h>

#include "boards.h"
#include "main.h"
#include "dev.h"

#include "st7032i.h"
#include "rcs730.h"

#include "app_error.h"
#include "app_trace.h"


/**************************************************************************
 * macro
 **************************************************************************/

/**************************************************************************
 * declaration
 **************************************************************************/

static RCS730_callbacktable_t           m_rcs730_cbtbl;


/**************************************************************************
 * prototype
 **************************************************************************/

/* RCS-730 callback */
static bool rcs730cb_read(void *pUser, uint8_t *pData, uint8_t Len);
static bool rcs730cb_write(void *pUser, uint8_t *pData, uint8_t Len);


/**************************************************************************
 * main entry
 **************************************************************************/

/**@brief main
 */
int main(void)
{
    int ret;

    // 初期化
    dev_init();

    RCS730_init();
    m_rcs730_cbtbl.pUserData = NULL;
    m_rcs730_cbtbl.pCbRxHTRDone = rcs730cb_read;
    m_rcs730_cbtbl.pCbRxHTWDone = rcs730cb_write;
    RCS730_setCallbackTable(&m_rcs730_cbtbl);
    ret = RCS730_initFTMode(RCS730_OPMODE_PLUG);
    if (ret != 0) {
        APP_ERROR_HANDLER(ret);
    }

    ST7032I_init();

    app_trace_init();
    app_trace_log("START\r\n");

    // 処理開始
    //timers_start();
    ble_advertising_start();

    ST7032I_writeString("(^_^);");

    // メインループ
    while (1) {
        dev_event_exec();
    }
}


/**************************************************************************
 * public function
 **************************************************************************/

/**@brief エラーハンドラ
 *
 * APP_ERROR_HANDLER()やAPP_ERROR_CHECK()から呼び出される(app_error.h)。
 * 引数は見ず、ASSERT LEDを点灯させて処理を止める。
 *
 * @param[in] error_code  エラーコード
 * @param[in] line_num    エラー発生行(__LINE__など)
 * @param[in] p_file_name エラー発生ファイル(__FILE__など)
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t *p_file_name)
{
#if 0
    //リセットによる再起動
    NVIC_SystemReset();
#else
    //ASSERT LED点灯
    led_on(LED_PIN_NO_ASSERT);

    //留まる
    while(1) {
        __WFI();
    }
#endif
}


/**
 * @brief IRQ検知
 *
 * IRQ立ち下がりでコールバックされる。
 */
void gpiote_irq_handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low)
{
    app_trace_log("irq\r\n");
    RCS730_isrIrq();
}


/**************************************************************************
 * private function
 **************************************************************************/

/**********************************************
 * RC-S730
 **********************************************/

static bool rcs730cb_read(void *pUser, uint8_t *pData, uint8_t Len)
{
    app_trace_log("read\r\n");
    ST7032I_clear();

    if (!ble_is_connected()) {
        ST7032I_writeString("read err");

        pData[0] = 13;
        pData[10] = 0xff;  //ST1
        pData[11] = 0x70;  //ST2(データ読み出しエラー)
        return true;
    }

    ST7032I_writeString("read");

    ble_nofify(pData, Len);

    uint8_t nob = pData[13] << 4;       //16byte * NoB
    pData[0] = (uint8_t)(13 + nob);
    pData[10] = 0;  //ST1
    pData[11] = 0;  //ST2
    for (int i = 0; i < nob; i++) {
        pData[13 + i] = (uint8_t)(0x30 + i);
    }

    return true;
}


static bool rcs730cb_write(void *pUser, uint8_t *pData, uint8_t Len)
{
    app_trace_log("write\r\n");
    ST7032I_clear();

    pData[0] = 12;

    if (!ble_is_connected()) {
        ST7032I_writeString("write err");

        pData[10] = 0xff;  //ST1
        pData[11] = 0x70;  //ST2
        return true;
    }

    ST7032I_writeString("write");

    pData[16 + 16] = '\0';
    pData[10] = 0;  //ST1
    pData[11] = 0;  //ST2

    return true;
}
