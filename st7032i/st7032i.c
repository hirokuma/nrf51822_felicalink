/**
 * @file    st7032i.c
 * @brief   ST7032i Library
 *
 * http://strawberry-linux.com/catalog/items?code=27001
 *
 */
#include "st7032i.h"
#include "twi_master.h"
#include "nrf_delay.h"


#define I2C_SLV_ADDR            (0x7c)      //Slave Address(8bit)
#define VAL_CONTRAST            (0x28)      //LCD contrast
#define VAL_CONTRAST_C54        ((VAL_CONTRAST >> 4) & 0x03)
#define VAL_CONTRAST_C3210      (VAL_CONTRAST & 0x0f)

#define REG(addr)               (*((volatile uint32_t*)addr))

#define CONTROL_BYTE(rs)        ((uint8_t)((rs)<<6))
#define CTRL_BYTE_CMD           CONTROL_BYTE(0)
#define CTRL_BYTE_DATA          CONTROL_BYTE(1)

#define CMD_CLEARDISPLAY            ((uint8_t)0x01)
#define CMD_RETHOME                 ((uint8_t)0x02)
#define CMD_ENTRYMODESET(id,s)      ((uint8_t)(0x04|((id)<<1)|(s)))
#define CMD_DISPONOFF(d,c,b)        ((uint8_t)(0x08|((d)<<2)|((c)<<1)|(b)))
#define CMD_FUNCSET(is)             ((uint8_t)(0x38|((0)<<2)|(is)))
#define CMD_SETDDRAMADDR(addr)      ((uint8_t)(0x80|(addr)))
#define CMD_INOSCFREQ(bs,f)         ((uint8_t)(0x10|((bs)<<3)|(f)))             //Internal OSC frequency
#define CMD_SETICONADDR(ac)         ((uint8_t)(0x40|(ac)))                      //Set ICON address
#define CMD_PICTRL_CSET(i,b,c54)    ((uint8_t)(0x50|(i)<<3|(b)<<2|(c54)))       //Power/Icon control/Contrast set
#define CMD_FLW_CTRL(f,rab)         ((uint8_t)(0x60|(f)<<3|(rab)))              //Follower control
#define CMD_CNTRSET(c3210)          ((uint8_t)(0x70|(c3210)))                   //Contrast set

#define CMD_ENTRYMODESET_NORMAL CMD_ENTRYMODESET(1,0)       //cursor move:right
                                                            //shift:off
#define CMD_DISPLAY_ON          CMD_DISPONOFF(1,0,0)
#define CMD_DISPLAY_OFF         CMD_DISPONOFF(0,0,0)
#define CMD_FUNCSET_IS0         CMD_FUNCSET(0)
#define CMD_FUNCSET_IS1         CMD_FUNCSET(1)


static int write_lcd(uint8_t cmd, uint8_t data, uint32_t usec);



/**
 * 初期化
 *
 * タスク起動前に呼び出される想定
 */
void ST7032I_init(void)
{
    //リセット解除から40ms必要
    nrf_delay_ms(40);

    //Function Set(IS=0)
    write_lcd(CTRL_BYTE_CMD, CMD_FUNCSET_IS0, 27);

    //Function Set(IS=1)
    write_lcd(CTRL_BYTE_CMD, CMD_FUNCSET_IS1, 27);

    /*
     * (Instruction table 1)internal OSC frequency
     *      BS=1(1/4bias)
     *      F2-0=0
     *      wait:26.3us
     */
    write_lcd(CTRL_BYTE_CMD, CMD_INOSCFREQ(1,0), 27);

    /*
     * (Instruction table 1)コントラスト調整など
     *      Power/ICON/Contrast Set
     *          Ion=1
     *          Bon=1
     *          C5-4=VAL_CONTRASTのb54
     *          wait:26.3us
     *
     *      Constrast Set:
     *          C3-0=VAL_CONTRASTのb3210
     *          wait:26.3us
     */
    write_lcd(CTRL_BYTE_CMD, CMD_PICTRL_CSET(1,1,VAL_CONTRAST_C54), 27);
    write_lcd(CTRL_BYTE_CMD, CMD_CNTRSET(VAL_CONTRAST_C3210), 27);

    /*
     * (Instruction table 1)Follower control
     *      Fon=1
     *      Rab=4
     *      wait:26.3us --> 電力安定のため200ms
     */
    write_lcd(CTRL_BYTE_CMD, CMD_FLW_CTRL(1,4), 200000);

    //Function Set(IS=0)
    //write_lcd(CTRL_BYTE_CMD, CMD_FUNCSET_IS0, 27);

    //display on
    write_lcd(CTRL_BYTE_CMD, CMD_DISPLAY_ON, 27);
    //clear display
    ST7032I_clear();
    //entry mode set
    write_lcd(CTRL_BYTE_CMD, CMD_ENTRYMODESET_NORMAL, 27);
}


/**
 * 画面クリア
 */
void ST7032I_clear(void)
{
    write_lcd(CTRL_BYTE_CMD, CMD_CLEARDISPLAY, 1080);
}


/**
 * カーソル移動
 *
 * @param[in]   x   X座標(0～15)
 * @param[in]   y   Y座標
 */
void ST7032I_movePos(int x, ST7032I_Line y)
{
    write_lcd(CTRL_BYTE_CMD, CMD_SETDDRAMADDR(y | x), 27);
}


/**
 * 現在のカーソル位置から文字列出力
 *
 * @param[in]   pStr    文字列
 */
void ST7032I_writeString(const char *pStr)
{
    while(*pStr) {
        write_lcd(CTRL_BYTE_DATA, *pStr++, 27);
    }
}


/**
 * ST7032iへの出力
 *
 * @param[in]   ctrl    コントロールバイト
 * @param[in]   data    データ
 * @param[in]   usec    待ち時間[usec]
 */
static int write_lcd(uint8_t ctrl, uint8_t data, uint32_t usec)
{
    bool ret;
    uint8_t buf[2];

    buf[0] = ctrl;
    buf[1] = data;
    ret = twi_master_transfer(I2C_SLV_ADDR, buf, (uint8_t)sizeof(buf), true);
    nrf_delay_us(usec);

    return (ret) ? 0 : -1;
}
