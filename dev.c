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
#include "boards.h"
#include "dev.h"
#include "main.h"

#include "app_error.h"

#include "softdevice_handler.h"
#include "app_timer.h"
#include "app_gpiote.h"
#include "app_button.h"
#include "twi_master.h"

#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "ble_gap.h"
#include "ble_hci.h"

#include "app_trace.h"

#include "st7032i.h"


/**************************************************************************
 * macro
 **************************************************************************/
#define ARRAY_SIZE(array)               (sizeof(array) / sizeof(array[0]))

/** Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#define DEAD_BEEF                       (0xDEADBEEF)


/**
 * Include or not the service_changed characteristic.
 * if not enabled, the server's database cannot be changed for the lifetime of the device
 */
#define IS_SRVC_CHANGED_CHARACT_PRESENT (0)


/*
 * Timer
 */
/** Value of the RTC1 PRESCALER register. */
#define APP_TIMER_PRESCALER             (0)

/** BLEが使用するタイマ数(BLEを使うなら1、使わないなら0) */
#define APP_TIMER_NUM_BLE               (1)

/** ボタンが使用するタイマ数(ボタンを使うなら1、使わないなら0) */
#define APP_TIMER_NUM_BUTTON            (0)

/** ユーザアプリで使用するタイマ数 */
#define APP_TIMER_NUM_USERAPP           (0)

/** 同時に生成する最大タイマ数 */
#define APP_TIMER_MAX_TIMERS            (APP_TIMER_NUM_BLE+APP_TIMER_NUM_BUTTON+APP_TIMER_NUM_USERAPP)

/** Size of timer operation queues. */
#define APP_TIMER_OP_QUEUE_SIZE         (4)

/*
 * GPIOTEおよびButton
 */
/** Maximum number of users of the GPIOTE handler. */
#define APP_GPIOTE_MAX_USERS            (1)

/** Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */
#define BUTTON_DETECTION_DELAY          (50)

/*
 * Scheduler
 */

/**
 * Maximum size of scheduler events.
 * Note that scheduler BLE stack events do not contain any data, as the events are being pulled from the stack in the event handler.
 */
#define SCHED_MAX_EVENT_DATA_SIZE       sizeof(app_timer_event_t)

/** Maximum number of events in the scheduler queue. */
#define SCHED_QUEUE_SIZE                (10)

/*
 * デバイス名
 *   UTF-8かつ、\0を含まずに20文字以内(20byte?)
 */
#define GAP_DEVICE_NAME                 "FelicaLinkDev"
//                                      "12345678901234567890"

/*
 * Appearance設定
 *  コメントアウト時はAppearance無しにするが、おそらくSoftDeviceがUnknownにしてくれる。
 *
 * Bluetooth Core Specification Supplement, Part A, Section 1.12
 * Bluetooth Core Specification 4.0 (Vol. 3), Part C, Section 12.2
 * https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.gap.appearance.xml
 * https://developer.nordicsemi.com/nRF51_SDK/nRF51_SDK_v7.x.x/doc/7.2.0/s110/html/a00837.html
 */

/*
 * BLE : Advertising
 */
/* Advertising間隔[msec単位] */
#define APP_ADV_INTERVAL                (1000)

/* Advertisingタイムアウト時間[sec単位] */
#define APP_ADV_TIMEOUT_IN_SECONDS      (60)

/*
 * Peripheral Preferred Connection Parameters(PPCP)
 *   パラメータの意味はCore_v4.1 p.2537 "4.5 CONNECTION STATE"を参照
 *
 * connInterval : Connectionイベントの送信間隔(7.5msec～4sec)
 * connSlaveLatency : SlaveがConnectionイベントを連続して無視できる回数
 * connSupervisionTimeout : Connection時に相手がいなくなったとみなす時間(100msec～32sec)
 *                          (1 + connSlaveLatency) * connInterval * 2以上
 *
 * note:
 *      connSupervisionTimeout * 8 >= connIntervalMax * (connSlaveLatency + 1)
 */
/* 最小時間[msec単位] */
#define CONN_MIN_INTERVAL               (2000)

/* 最大時間[msec単位] */
#define CONN_MAX_INTERVAL               (4000)

/* slave latency */
#define CONN_SLAVE_LATENCY              (0)

/* connSupervisionTimeout[msec単位] */
#define CONN_SUP_TIMEOUT                (4000)

/** sd_ble_gap_conn_param_update()を実行してから初回の接続イベントを通知するまでの時間[msec単位] */
/*
 * sd_ble_gap_conn_param_update()
 *   Central role時:
 *      Link Layer接続パラメータ更新手続きを初期化する。
 *
 *   Peripheral role時:
 *      L2CAPへ連絡要求(corresponding L2CAP request)を送信し、Centralからの手続きを待つ。
 *      接続パラメータとしてNULLが指定でき、そのときはPPCPキャラクタリスティックが使われる。
 *      ということは、Peripheralだったらsd_ble_gap_conn_param_update()は呼ばずに
 *      sd_ble_gap_ppcp_set()を呼ぶということでもよいということか？
 */
#define CONN_FIRST_PARAMS_UPDATE_DELAY  (5000)

/** sd_ble_gap_conn_param_update()を呼び出す間隔[msec単位] */
#define CONN_NEXT_PARAMS_UPDATE_DELAY   (30000)

/** Connectionパラメータ交換を諦めるまでの試行回数 */
#define CONN_MAX_PARAMS_UPDATE_COUNT    (3)

/*
 * BLE : Security
 */
/** ペアリング要求からのタイムアウト時間[sec] */
#define SEC_PARAM_TIMEOUT               (30)

/** 1:Bondingあり 0:なし */
#define SEC_PARAM_BOND                  (0)

/** 1:ペアリング時の認証あり 0:なし */
#define SEC_PARAM_MITM                  (0)

/** IO能力
 * - BLE_GAP_IO_CAPS_DISPLAY_ONLY     : input=無し/output=画面
 * - BLE_GAP_IO_CAPS_DISPLAY_YESNO    : input=Yes/No程度/output=画面
 * - BLE_GAP_IO_CAPS_KEYBOARD_ONLY    : input=キーボード/output=無し
 * - BLE_GAP_IO_CAPS_NONE             : input/outputなし。あるいはMITM無し
 * - BLE_GAP_IO_CAPS_KEYBOARD_DISPLAY : input=キーボード/output=画面
 */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE

/** 1:OOB認証有り 0:なし */
#define SEC_PARAM_OOB                   (0)

/** 符号化鍵サイズ:最小byte(7～) */
#define SEC_PARAM_MIN_KEY_SIZE          (7)

/** 符号化鍵サイズ:最大byte(min～16) */
#define SEC_PARAM_MAX_KEY_SIZE          (16)


//設定値のチェック
#if (APP_ADV_INTERVAL < 20)
#error connInterval(Advertising) too small.
#elif (APP_ADV_INTERVAL < 100)
#warning connInterval(Advertisin) is too small in non-connectable mode
#elif (10240 < APP_ADV_INTERVAL)
#error connInterval(Advertising) too large.
#endif  //APP_ADV_INTERVAL

#if (APP_ADV_TIMEOUT_IN_SECONDS == BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED)
//OK
#elif (BLE_GAP_ADV_TIMEOUT_LIMITED_MAX < APP_ADV_TIMEOUT_IN_SECONDS)
#warning Advertising Timeout is too large in limited discoverable mode
#endif  //APP_ADV_TIMEOUT_IN_SECONDS

#if (CONN_MIN_INTERVAL * 1000 < 7500)
#error connInterval_Min(Connection) too small.
#elif (4000 < CONN_MIN_INTERVAL)
#error connInterval_Min(Connection) too large.
#endif  //CONN_MIN_INTERVAL

#if (CONN_MAX_INTERVAL * 1000 < 7500)
#error connInterval_Max(Connection) too small.
#elif (4000 < CONN_MAX_INTERVAL)
#error connInterval_Max(Connection) too large.
#endif  //CONN_MAX_INTERVAL

#if (CONN_MAX_INTERVAL < CONN_MIN_INTERVAL)
#error connInterval_Max < connInterval_Min
#endif  //connInterval Max < Min

#if (BLE_GAP_CP_SLAVE_LATENCY_MAX < CONN_SLAVE_LATENCY)
#error connSlaveLatency too large.
#endif  //CONN_SLAVE_LATENCY

#if (CONN_SUP_TIMEOUT < 100)
#error connSupervisionTimeout too small.
#elif (32000 < CONN_SUP_TIMEOUT)
#error connSupervisionTimeout too large.
#endif  //CONN_SUP_TIMEOUT

#if (CONN_SUP_TIMEOUT * 8 < CONN_MAX_INTERVAL * (CONN_SLAVE_LATENCY + 1))
#error connSupervisionTimeout too small in manner.
#endif


/**************************************************************************
 * declaration
 **************************************************************************/

/** Handle of the current connection. */
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;

#ifdef BLE_DFU_APP_SUPPORT
static ble_dfu_t                        m_dfus;     /**< Structure used to identify the DFU service. */
#endif // BLE_DFU_APP_SUPPORT

static ble_fps_t                        m_fps;

static app_gpiote_user_id_t             m_gpiote_irq;


/**************************************************************************
 * prototype
 **************************************************************************/

/* GPIO */
static void gpio_init(void);

/* Timer */
static void timers_init(void);
//static void timers_start(void);

/* Scheduler */
static void scheduler_init(void);

/* GPIOTEおよびButton */
static void gpiote_init(void);
//static void buttons_init(void);
//static void button_event_handler(uint8_t pin_no, uint8_t button_event);

static void ble_stack_init(void);

static void conn_params_evt_handler(ble_conn_params_evt_t * p_evt);
static void conn_params_error_handler(uint32_t nrf_error);

static void svc_fps_handler_ndef(ble_fps_t *p_fps, const uint8_t *p_value, uint16_t length);

static void ble_evt_handler(ble_evt_t * p_ble_evt);
static void ble_evt_dispatch(ble_evt_t * p_ble_evt);

static void sys_evt_dispatch(uint32_t sys_evt);


/**************************************************************************
 * public function
 **************************************************************************/

void dev_init(void)
{
    gpio_init();
    twi_master_init();
    timers_init();      //app_button_init()やble_conn_params_init()よりも前に呼ぶこと!
                        //呼ばなかったら、NRF_ERROR_INVALID_STATE(8)が発生する。

    gpiote_init();
//    buttons_init();
    scheduler_init();
    ble_stack_init();
}


void dev_event_exec(void)
{
    uint32_t err_code;

    //スケジュール済みイベントの実行(mainloop内で呼び出す)
    app_sched_execute();
    err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Advertising開始
 */
void ble_advertising_start(void)
{
    uint32_t             err_code;
    ble_gap_adv_params_t adv_params;

    // Start advertising
    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
    adv_params.p_peer_addr = NULL;
    adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    adv_params.interval    = MSEC_TO_UNITS(APP_ADV_INTERVAL, UNIT_0_625_MS);
    adv_params.timeout     = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = sd_ble_gap_adv_start(&adv_params);
    APP_ERROR_CHECK(err_code);
    led_on(LED_PIN_NO_ADVERTISING);

    app_trace_log("advertising start\r\n");
}


#ifdef BLE_DFU_APP_SUPPORT
void ble_advertising_stop(void)
{
    uint32_t err_code;

    err_code = sd_ble_gap_adv_stop();
    APP_ERROR_CHECK(err_code);
    led_off(LED_PIN_NO_ADVERTISING);

    app_trace_log("advertising stop\r\n");
}
#endif // BLE_DFU_APP_SUPPORT

int ble_is_connected(void)
{
	return m_conn_handle != BLE_CONN_HANDLE_INVALID;
}

void ble_nofify(const uint8_t *p_data, uint16_t length)
{
	ble_fps_notify(&m_fps, p_data, length);
}


/**********************************************
 * LED
 **********************************************/

/**
 * @brief LED点灯
 *
 * @param[in]   pin     対象PIN番号
 */
void led_on(int pin)
{
    /* アクティブLOW */
    nrf_gpio_pin_clear(pin);
}


/**
 * @brief LED消灯
 *
 * @param[in]   pin     対象PIN番号
 */
void led_off(int pin)
{
    /* アクティブLOW */
    nrf_gpio_pin_set(pin);
}


/**************************************************************************
 * private function
 **************************************************************************/

/**********************************************
 * GPIO
 **********************************************/

/**
 * @brief GPIO初期化
 */
static void gpio_init(void)
{
    nrf_gpio_cfg_output(LED_PIN_NO_ADVERTISING);
    nrf_gpio_cfg_output(LED_PIN_NO_CONNECTED);
    nrf_gpio_cfg_output(LED_PIN_NO_ASSERT);
    nrf_gpio_cfg_input(RCS730_IRQ, NRF_GPIO_PIN_NOPULL);

    /* LED消灯 */
    led_off(LED_PIN_NO_ADVERTISING);
    led_off(LED_PIN_NO_CONNECTED);
    led_off(LED_PIN_NO_ASSERT);
}


/**********************************************
 * タイマ
 **********************************************/

/**
 * @brief タイマ初期化
 *
 * タイマ機能を初期化する。
 * 調べた範囲では、以下の機能を使用する場合には、その初期化よりも前に実行しておく必要がある。
 *  - ボタン
 *  - BLE
 */
static void timers_init(void)
{
    // Initialize timer module, making it use the scheduler
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, true);

#if 0
    /* YOUR_JOB: Create any timers to be used by the application.
                 Below is an example of how to create a timer.
                 For every new timer needed, increase the value of the macro APP_TIMER_MAX_TIMERS by
                 one.
     */
    err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
    APP_ERROR_CHECK(err_code);
#endif
}

#if 0
/**
 * @brief タイマ開始
 */
static void timers_start(void)
{
#if 0
    /* YOUR_JOB: Start your timers. below is an example of how to start a timer. */
    uint32_t err_code;

    err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
#endif
}
#endif

/**********************************************
 * Scheduler
 **********************************************/

/**
 * @brief スケジューラ初期化
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**********************************************
 * GPIOTE
 **********************************************/

/**
 * @brief GPIOTE初期化
 */
static void gpiote_init(void)
{
    uint32_t             err_code;

    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);

    //IRQ : active low
    err_code = app_gpiote_user_register(&m_gpiote_irq,
                        0,
                        1 << RCS730_IRQ,
                        gpiote_irq_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_gpiote_user_enable(m_gpiote_irq);
    APP_ERROR_CHECK(err_code);
}


/**********************************************
 * BLE : Advertising
 **********************************************/

/**
 * @brief BLEスタック初期化
 *
 * @detail BLE関連の初期化を行う。
 *      -# SoftDeviceハンドラ初期化
 *      -# システムイベントハンドラ初期化
 *      -# BLEスタック有効化
 *      -# BLEイベントハンドラ設定
 *      -# デバイス名設定
 *      -# Appearance設定(GAP_USE_APPEARANCE定義時)
 *      -# PPCP設定
 *      -# Service初期化
 *      -# Advertising初期化
 *      -# Connection初期化
 */
static void ble_stack_init(void)
{
    uint32_t err_code;

    /*
     * SoftDeviceの初期化
     *      スケジューラの使用：あり
     */
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_RC_250_PPM_4000MS_CALIBRATION, true);

    /* システムイベントハンドラの設定 */
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);

#if defined(S110) || defined(S310)
    /* BLEスタックの有効化 */
    {
        ble_enable_params_t ble_enable_params;

        memset(&ble_enable_params, 0, sizeof(ble_enable_params));
        ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
        err_code = sd_ble_enable(&ble_enable_params);
        APP_ERROR_CHECK(err_code);
    }
#endif  //S110 || S310

    /* BLEイベントハンドラの設定 */
    {
        err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
        APP_ERROR_CHECK(err_code);
    }

    /* デバイス名設定 */
    {
        //デバイス名へのWrite Permission(no protection, open link)
        ble_gap_conn_sec_mode_t sec_mode;

        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
        err_code = sd_ble_gap_device_name_set(&sec_mode,
                                            (const uint8_t *)GAP_DEVICE_NAME,
                                            strlen(GAP_DEVICE_NAME));
        APP_ERROR_CHECK(err_code);
    }

#ifdef GAP_USE_APPEARANCE
    /* Appearance設定 */
    err_code = sd_ble_gap_appearance_set(GAP_USE_APPEARANCE);
    APP_ERROR_CHECK(err_code);
#endif  //GAP_USE_APPEARANCE

    /*
     * Peripheral Preferred Connection Parameters(PPCP)
     * ここで設定しておくと、Connection Parameter Update Reqを送信せずに済むらしい。
     */
    {
        ble_gap_conn_params_t   gap_conn_params = {0};

        gap_conn_params.min_conn_interval = MSEC_TO_UNITS(CONN_MIN_INTERVAL, UNIT_1_25_MS);
        gap_conn_params.max_conn_interval = MSEC_TO_UNITS(CONN_MAX_INTERVAL, UNIT_1_25_MS);
        gap_conn_params.slave_latency     = CONN_SLAVE_LATENCY;
        gap_conn_params.conn_sup_timeout  = MSEC_TO_UNITS(CONN_SUP_TIMEOUT, UNIT_10_MS);

        err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
        APP_ERROR_CHECK(err_code);
    }

    /*
     * Service初期化
     */
    {
        ble_fps_init_t fps_init;

        fps_init.evt_handler_ndef = svc_fps_handler_ndef;
        err_code = ble_fps_init(&m_fps, &fps_init);
        APP_ERROR_CHECK(err_code);
    }

    /*
     * Advertising初期化
     */
    {
        ble_uuid_t adv_uuids[] = { { FPS_UUID_SERVICE, m_fps.uuid_type } };
        ble_advdata_t advdata;
        ble_advdata_t scanrsp;
        //Vol 3,Part C : Generic Access Profile "9.2 Discovery Modes and Procedures"
        uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;    //探索時間に制限あり
        //uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

        memset(&advdata, 0, sizeof(advdata));
        memset(&scanrsp, 0, sizeof(scanrsp));

        /*
         * ble_advdata_name_type_t (ble_advdata.h)
         *
         * BLE_ADVDATA_NO_NAME    : デバイス名無し
         * BLE_ADVDATA_SHORT_NAME : デバイス名あり «Shortened Local Name»
         * BLE_ADVDATA_FULL_NAME  : デバイス名あり «Complete Local Name»
         *
         * https://www.bluetooth.org/en-us/specification/assigned-numbers/generic-access-profile
         * https://developer.nordicsemi.com/nRF51_SDK/nRF51_SDK_v7.x.x/doc/7.2.0/s110/html/a01015.html#ga03c5ccf232779001be9786021b1a563b
         */
        advdata.name_type = BLE_ADVDATA_FULL_NAME;

        /*
         * Appearanceが含まれるかどうか
         */
#ifdef GAP_USE_APPEARANCE
        advdata.include_appearance = true;
#else   //GAP_USE_APPEARANCE
        advdata.include_appearance = false;
#endif  //GAP_USE_APPEARANCE
        /*
         * Advertisingフラグの設定
         * CSS_v4 : Part A  1.3 FLAGS
         * https://developer.nordicsemi.com/nRF51_SDK/nRF51_SDK_v7.x.x/doc/7.2.0/s110/html/a00802.html
         *
         * BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE = BLE_GAP_ADV_FLAG_LE_LIMITED_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED
         *      BLE_GAP_ADV_FLAG_LE_LIMITED_DISC_MODE : LE Limited Discoverable Mode
         *      BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED : BR/EDR not supported
         */
        advdata.flags.size   = sizeof(flags);
        advdata.flags.p_data = &flags;

        /* SCAN_RSPデータ設定 */
        scanrsp.uuids_complete.uuid_cnt = ARRAY_SIZE(adv_uuids);
        scanrsp.uuids_complete.p_uuids  = adv_uuids;

        err_code = ble_advdata_set(&advdata, &scanrsp);
        APP_ERROR_CHECK(err_code);
    }

    /*
     * Connection初期化
     */
    {
        ble_conn_params_init_t cp_init = {0};

        cp_init.p_conn_params                  = NULL;
        cp_init.first_conn_params_update_delay = APP_TIMER_TICKS(CONN_FIRST_PARAMS_UPDATE_DELAY, APP_TIMER_PRESCALER);
        cp_init.next_conn_params_update_delay  = APP_TIMER_TICKS(CONN_NEXT_PARAMS_UPDATE_DELAY, APP_TIMER_PRESCALER);
        cp_init.max_conn_params_update_count   = CONN_MAX_PARAMS_UPDATE_COUNT;
        cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
        cp_init.disconnect_on_fail             = false;
        cp_init.evt_handler                    = conn_params_evt_handler;
        cp_init.error_handler                  = conn_params_error_handler;

        err_code = ble_conn_params_init(&cp_init);
        APP_ERROR_CHECK(err_code);
    }

#ifdef BLE_DFU_APP_SUPPORT
    /** @snippet [DFU BLE Service initialization] */
    {
        ble_dfu_init_t   dfus_init;

        // Initialize the Device Firmware Update Service.
        memset(&dfus_init, 0, sizeof(dfus_init));

        dfus_init.evt_handler    = dfu_app_on_dfu_evt;
        dfus_init.error_handler  = NULL; //service_error_handler - Not used as only the switch from app to DFU mode is required and not full dfu service.
        dfus_init.evt_handler    = dfu_app_on_dfu_evt;
        dfus_init.revision       = DFU_REVISION;

        err_code = ble_dfu_init(&m_dfus, &dfus_init);
        APP_ERROR_CHECK(err_code);

        dfu_app_reset_prepare_set(dfu_reset_prepare);
    }
    /** @snippet [DFU BLE Service initialization] */
#endif // BLE_DFU_APP_SUPPORT
}


/** @snippet [DFU BLE Reset prepare] */
#ifdef BLE_DFU_APP_SUPPORT
static void dfu_reset_prepare(void)
{
    uint32_t err_code;

    if (m_conn_handle != BLE_CONN_HANDLE_INVALID) {
        // Disconnect from peer.
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        err_code = bsp_indication_set(BSP_INDICATE_IDLE);
        APP_ERROR_CHECK(err_code);
    }
    else {
        // If not connected, then the device will be advertising. Hence stop the advertising.
        ble_advertising_stop();
    }

    err_code = ble_conn_params_stop();
    APP_ERROR_CHECK(err_code);
}
/** @snippet [DFU BLE Reset prepare] */
#endif // BLE_DFU_APP_SUPPORT


#ifdef BLE_DFU_APP_SUPPORT
/**@brief Function for the Device Manager initialization.
 */
static void device_manager_init(void)
{
    uint32_t               err_code;
    dm_init_param_t        init_data;
    dm_application_param_t register_param;

    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    // Clear all bonded centrals if the Bonds Delete button is pushed.
    err_code = bsp_button_is_pressed(BOND_DELETE_ALL_BUTTON_ID,&(init_data.clear_persistent_data));
    APP_ERROR_CHECK(err_code);

    err_code = dm_init(&init_data);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));

    register_param.sec_param.timeout      = SEC_PARAM_TIMEOUT;
    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.mitm         = SEC_PARAM_MITM;
    register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);
}
#endif // BLE_DFU_APP_SUPPORT


/**********************************************
 * BLE : Connection
 **********************************************/

/**
 * @brief Connectionパラメータモジュールイベントハンドラ
 *
 * @details Connectionパラメータモジュールでアプリに通知するイベントが発生した場合に呼ばれる。
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in]   p_evt   Connectionパラメータモジュールから受信したイベント
 */
static void conn_params_evt_handler(ble_conn_params_evt_t *p_evt)
{
    uint32_t err_code;

    if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**
 * @brief Connectionパラメータエラーハンドラ
 *
 * @param[in]   nrf_error   エラーコード
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**********************************************
 * BLE : Services
 **********************************************/

/**
 * @brief FeliCa Plugサービスイベントハンドラ
 *
 * @param[in]   p_fps   FPサービス構造体
 * @param[in]   p_value 受信バッファ
 * @param[in]   length  受信データ長
 */
static void svc_fps_handler_ndef(ble_fps_t *p_fps, const uint8_t *p_value, uint16_t length)
{
    app_trace_log("svc_fps_handler_ndef\r\n");
}

/**********************************************
 * BLE stack
 **********************************************/

/**
 * @brief BLEスタックイベントハンドラ
 *
 * @param[in]   p_ble_evt   BLEスタックイベント
 */
static void ble_evt_handler(ble_evt_t *p_ble_evt)
{
    uint32_t                         err_code;
    static ble_gap_evt_auth_status_t m_auth_status;
    ble_gap_enc_info_t               *p_enc_info;

    switch (p_ble_evt->header.evt_id) {
    /*************
     * GAP event
     *************/

    //接続が成立したとき
    case BLE_GAP_EVT_CONNECTED:
        app_trace_log("BLE_GAP_EVT_CONNECTED\r\n");
        ST7032I_clear();
        ST7032I_writeString("connect");
        led_on(LED_PIN_NO_CONNECTED);
        led_off(LED_PIN_NO_ADVERTISING);
        m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        break;

    //相手から切断されたとき
    //必要があればsd_ble_gatts_sys_attr_get()でSystem Attributeを取得し、保持しておく。
    //保持したSystem Attributeは、EVT_SYS_ATTR_MISSINGで返すことになる。
    case BLE_GAP_EVT_DISCONNECTED:
        app_trace_log("BLE_GAP_EVT_DISCONNECTED\r\n");
        ST7032I_clear();
        ST7032I_writeString("disconnect");
        led_off(LED_PIN_NO_CONNECTED);
        m_conn_handle = BLE_CONN_HANDLE_INVALID;

        ble_advertising_start();
        break;

    //SMP Paring要求を受信したとき
    //sd_ble_gap_sec_params_reply()で値を返したあと、SMP Paring Phase 2に状態遷移する
    case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
        app_trace_log("BLE_GAP_EVT_SEC_PARAMS_REQUEST\r\n");
        {
            ble_gap_sec_params_t sec_param;
            sec_param.timeout      = SEC_PARAM_TIMEOUT;
            sec_param.bond         = SEC_PARAM_BOND;
            sec_param.mitm         = SEC_PARAM_MITM;
            sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
            sec_param.oob          = SEC_PARAM_OOB;
            sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
            sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
                                               BLE_GAP_SEC_STATUS_SUCCESS,
                                               &sec_param);
            APP_ERROR_CHECK(err_code);
        }
        break;

    //Just Works(Bonding有り)の場合、SMP Paring Phase 3のあとでPeripheral Keyが渡される。
    //ここではPeripheral Keyを保存だけしておき、次のBLE_GAP_EVT_SEC_INFO_REQUESTで処理する。
    case BLE_GAP_EVT_AUTH_STATUS:
        app_trace_log("BLE_GAP_EVT_AUTH_STATUS\r\n");
        m_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
        break;

    //SMP Paringが終わったとき？
    case BLE_GAP_EVT_SEC_INFO_REQUEST:
        app_trace_log("BLE_GAP_EVT_SEC_INFO_REQUEST\r\n");
        p_enc_info = &m_auth_status.periph_keys.enc_info;
        if (p_enc_info->div == p_ble_evt->evt.gap_evt.params.sec_info_request.div) {
            //Peripheral Keyが有る
            err_code = sd_ble_gap_sec_info_reply(m_conn_handle, p_enc_info, NULL);
        }
        else {
            //Peripheral Keyが無い
            err_code = sd_ble_gap_sec_info_reply(m_conn_handle, NULL, NULL);
        }
        APP_ERROR_CHECK(err_code);
        break;

    //Advertisingか認証のタイムアウト発生
    case BLE_GAP_EVT_TIMEOUT:
        app_trace_log("BLE_GAP_EVT_TIMEOUT\r\n");
        switch (p_ble_evt->evt.gap_evt.params.timeout.src) {
        case BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT: //Advertisingのタイムアウト
            /* Advertising LEDを消灯 */
            led_off(LED_PIN_NO_ADVERTISING);

            /* System-OFFにする(もう戻ってこない) */
            err_code = sd_power_system_off();
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_TIMEOUT_SRC_SECURITY_REQUEST:  //Security requestのタイムアウト
            break;

        default:
            break;
        }
        break;

    /*********************
     * GATT Server event
     *********************/

    //接続後、Bondingした相手からSystem Attribute要求を受信したとき
    //System Attributeは、EVT_DISCONNECTEDで保持するが、今回は保持しないのでNULLを返す。
    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        app_trace_log("BLE_GATTS_EVT_SYS_ATTR_MISSING\r\n");
        err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0);
        APP_ERROR_CHECK(err_code);
        break;

    default:
        // No implementation needed.
        break;
    }
}


/**
 * @brief BLEイベントハンドラ
 *
 * @details BLEスタックイベント受信後、メインループのスケジューラから呼ばれる。
 *
 * @param[in]   p_ble_evt   BLEスタックイベント
 */
static void ble_evt_dispatch(ble_evt_t *p_ble_evt)
{
    ble_evt_handler(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);

    //I/O Service
    ble_fps_on_ble_evt(&m_fps, p_ble_evt);

#ifdef BLE_DFU_APP_SUPPORT
    /** @snippet [Propagating BLE Stack events to DFU Service] */
    ble_dfu_on_ble_evt(&m_dfus, p_ble_evt);
    /** @snippet [Propagating BLE Stack events to DFU Service] */
#endif // BLE_DFU_APP_SUPPORT
}

/**@brief システムイベント発生
 *
 * SoCでイベントが発生した場合にコールバックされる。
 *
 *
 * @param[in]   sys_evt   enum NRF_SOC_EVTS型(NRF_EVT_xxx). nrf_soc.hに定義がある.
 *      - NRF_EVT_HFCLKSTARTED
 *      - NRF_EVT_POWER_FAILURE_WARNING
 *      - NRF_EVT_FLASH_OPERATION_SUCCESS
 *      - NRF_EVT_FLASH_OPERATION_ERROR
 *      - NRF_EVT_RADIO_BLOCKED
 *      - NRF_EVT_RADIO_CANCELED
 *      - NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN
 *      - NRF_EVT_RADIO_SESSION_IDLE
 *      - NRF_EVT_RADIO_SESSION_CLOSED
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    //ここでは異常発生のように扱っているが、必ずそうではないようなので、
    //実装する際には注意しよう。
    app_error_handler(DEAD_BEEF, 0, NULL);
}

