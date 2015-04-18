/** FeliCa Link(RC-S730) Library
 *
 * @file    RCS730.h
 * @author  hiro99ma
 * @version 1.00
 */

#ifndef RCS730_H
#define RCS730_H

#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "nrf_error.h"

/** callback function type */
typedef bool (*RCS730_CALLBACK_T)(void *pUser, uint8_t *pData, uint8_t Len);


#define RCS730_BLK_PAD0             ((uint16_t)0x0000)  //!< [addr]PAD0
#define RCS730_BLK_PAD1             ((uint16_t)0x0001)  //!< [addr]PAD1
#define RCS730_BLK_PAD2             ((uint16_t)0x0002)  //!< [addr]PAD2
#define RCS730_BLK_PAD3             ((uint16_t)0x0003)  //!< [addr]PAD3
#define RCS730_BLK_PAD4             ((uint16_t)0x0004)  //!< [addr]PAD4
#define RCS730_BLK_PAD5             ((uint16_t)0x0005)  //!< [addr]PAD5
#define RCS730_BLK_PAD6             ((uint16_t)0x0006)  //!< [addr]PAD6
#define RCS730_BLK_PAD7             ((uint16_t)0x0007)  //!< [addr]PAD7
#define RCS730_BLK_PAD8             ((uint16_t)0x0008)  //!< [addr]PAD8
#define RCS730_BLK_PAD9             ((uint16_t)0x0009)  //!< [addr]PAD9
#define RCS730_BLK_PAD10            ((uint16_t)0x000a)  //!< [addr]PAD10
#define RCS730_BLK_PAD11            ((uint16_t)0x000b)  //!< [addr]PAD11
#define RCS730_BLK_PAD12            ((uint16_t)0x000c)  //!< [addr]PAD12
#define RCS730_BLK_PAD13            ((uint16_t)0x000d)  //!< [addr]PAD13
#define RCS730_BLK_REG              ((uint16_t)0x000e)  //!< [addr]REG
#define RCS730_BLK_RC               ((uint16_t)0x0080)  //!< [addr]RC
#define RCS730_BLK_MAC              ((uint16_t)0x0081)  //!< [addr]MAC
#define RCS730_BLK_ID               ((uint16_t)0x0082)  //!< [addr]ID
#define RCS730_BLK_D_ID             ((uint16_t)0x0083)  //!< [addr]D_ID
#define RCS730_BLK_SER_C            ((uint16_t)0x0084)  //!< [addr]SER_C
#define RCS730_BLK_SYS_C            ((uint16_t)0x0085)  //!< [addr]SYS_C
#define RCS730_BLK_CKV              ((uint16_t)0x0086)  //!< [addr]CKV
#define RCS730_BLK_CK               ((uint16_t)0x0087)  //!< [addr]CK
#define RCS730_BLK_MC               ((uint16_t)0x0088)  //!< [addr]MC
#define RCS730_BLK_WCNT             ((uint16_t)0x0090)  //!< [addr]WCNT
#define RCS730_BLK_MAC_A            ((uint16_t)0x0091)  //!< [addr]MAC_A
#define RCS730_BLK_STATE            ((uint16_t)0x0092)  //!< [addr]STATE
#define RCS730_BLK_CRC_CHECK        ((uint16_t)0x00a0)  //!< [addr]CRC_CHECK

#define RCS730_REG_OPMODE           ((uint16_t)0x0b00)  //!< [addr]Operation Mode register
#define RCS730_REG_TAG_TX_CTRL      ((uint16_t)0x0b04)  //!< [addr]Tag TX Control register
#define RCS730_REG_TAG_RX_CTRL      ((uint16_t)0x0b08)  //!< [addr]Tag RX Control register
#define RCS730_REG_RF_STATUS        ((uint16_t)0x0b0c)  //!< [addr]RF Status register
#define RCS730_REG_I2C_SLAVE_ADDR   ((uint16_t)0x0b10)  //!< [addr]I2C Slave Address register
#define RCS730_REG_I2C_BUFF_CTRL    ((uint16_t)0x0b14)  //!< [addr]I2C Buffer Control register
#define RCS730_REG_I2C_STATUS       ((uint16_t)0x0b18)  //!< [addr]I2C Status register
#define RCS730_REG_INT_MASK         ((uint16_t)0x0b20)  //!< [addr]Interrupt Mask register
#define RCS730_REG_INT_RAW_STATUS   ((uint16_t)0x0b24)  //!< [addr]Interrupt Raw Status register
#define RCS730_REG_INT_STATUS       ((uint16_t)0x0b28)  //!< [addr]Interrupt Status register
#define RCS730_REG_INT_CLEAR        ((uint16_t)0x0b2c)  //!< [addr]Interrupt Clear register
#define RCS730_REG_WRT_PROTECT      ((uint16_t)0x0b30)  //!< [addr]Write Protect register
#define RCS730_REG_STBY_CTRL        ((uint16_t)0x0b34)  //!< [addr]Standby Control register
#define RCS730_REG_INIT_CTRL        ((uint16_t)0x0b38)  //!< [addr]Initialize Control register
#define RCS730_REG_HOST_IF_SECURITY ((uint16_t)0x0b40)  //!< [addr]Host Interface Security register
#define RCS730_REG_HOST_IF_WCNT     ((uint16_t)0x0b44)  //!< [addr]Host Interface WCNT register
#define RCS730_REG_RF_PARAM         ((uint16_t)0x0b50)  //!< [addr]RF Parameter register
#define RCS730_REG_LITES_HT_CONF    ((uint16_t)0x0b60)  //!< [addr]Lite-S Host Through Configration register
#define RCS730_REG_LITES_PMM        ((uint16_t)0x0b64)  //!< [addr]Lite-S PMm register
#define RCS730_REG_PLUG_CONF1       ((uint16_t)0x0b80)  //!< [addr]Plug Configration 1 register
#define RCS730_REG_PLUG_CONF2       ((uint16_t)0x0b84)  //!< [addr]Plug Configration 2 register
#define RCS730_REG_PLUG_CONF3       ((uint16_t)0x0b88)  //!< [addr]Plug Configration 3 register
#define RCS730_REG_DEP_CONF         ((uint16_t)0x0ba0)  //!< [addr]DEP Configration register
#define RCS730_REG_DEP_PMM1         ((uint16_t)0x0ba4)  //!< [addr]DEP PMm1 register
#define RCS730_REG_DEP_PMM2         ((uint16_t)0x0ba8)  //!< [addr]DEP PMm2 register
#define RCS730_REG_RW_CONF          ((uint16_t)0x0bc0)  //!< [addr]RW Configration register
#define RCS730_REG_RW_CTRL          ((uint16_t)0x0b4c)  //!< [addr]RW Control register
#define RCS730_REG_RW_TIMEOUT       ((uint16_t)0x0bc8)  //!< [addr]RW Timeout register

#define RCS730_BUF_RF_COMM          ((uint16_t)0x0c00)  //!< [addr]RF Communication
#define RCS730_BUF_I2CFELICA_COMM   ((uint16_t)0x0d00)  //!< [addr]I2C FeliCa Communication

#define RCS730_MSK_MODE_CHANGED             ((uint32_t)0x80000000)  //!< OPMODE changed
#define RCS730_MSK_INT_RW_RX_ERROR          ((uint32_t)0x00080000)  //!< [R/W]
#define RCS730_MSK_INT_RW_RX_TIMEOUT        ((uint32_t)0x00040000)  //!< [R/W]
#define RCS730_MSK_INT_RW_RX_DONE           ((uint32_t)0x00020000)  //!< [R/W]
#define RCS730_MSK_INT_RW_TX_DONE           ((uint32_t)0x00010000)  //!< [R/W]
#define RCS730_MSK_INT_I2C_FELICA_CMD_ERROR ((uint32_t)0x00000200)  //!< [I2C]
#define RCS730_MSK_INT_I2C_FELICA_CMD_DONE  ((uint32_t)0x00000100)  //!< [I2C]
#define RCS730_MSK_INT_TAG_TX_DONE          ((uint32_t)0x00000040)  //!< [tag/DEP]Tx done
#define RCS730_MSK_INT_TAG_NFC_DEP_RX_DONE  ((uint32_t)0x00000020)  //!< [DEP]D4 command Rx done
#define RCS730_MSK_INT_TAG_RW_RX_DONE3      ((uint32_t)0x00000010)  //!< [tag]Write w/o Enc Rx done for HT block
#define RCS730_MSK_INT_TAG_RW_RX_DONE2      ((uint32_t)0x00000008)  //!< [tag]Read or Write w/o Enc Rx done for HT block
#define RCS730_MSK_INT_TAG_RW_RX_DONE1      ((uint32_t)0x00000004)  //!< [tag]Write w/o Enc Rx done for User block
#define RCS730_MSK_INT_TAG_PL_RX_DONE       ((uint32_t)0x00000002)  //!< [tag]Polling Rx done
#define RCS730_MSK_INT_TAG_RX_DONE          ((uint32_t)0x00000001)  //!< [tag]Read or Write w/o Enc Rx done
#define RCS730_MSK_ALL                      ((uint32_t)0x800f037f)

#define RCS730_REG_MASK_VAL                 ((uint32_t)0xffffffff)


/** Operation Mode
 *
 * @enum    OpMode
 */
typedef enum RCS730_OpMode {
    RCS730_OPMODE_LITES_HT = 0x00,        //!< Lite-S HT mode
    RCS730_OPMODE_PLUG = 0x01,            //!< Plug mode
    RCS730_OPMODE_NFCDEP = 0x02,          //!< NFC-DEP mode
    RCS730_OPMODE_LITES = 0x03            //!< Lite-S mode
} RCS730_OpMode;


/** System Code in Plug mode
 *
 * @enum    PlugSysCode
 */
typedef enum RCS730_PlugSysCode {
    RCS730_PLUG_SYS_CODE_FEEL = 0,     //!< 0xFEE1
    RCS730_PLUG_SYS_CODE_NDEF = 2,     //!< 0x12FC
} RCS730_PlugSysCode;


/** Callback Table
 *
 * @struct  callbacktable_t
 */
typedef struct RCS730_callbacktable_t {
    void                    *pUserData;         //!< User Data pointer
    RCS730_CALLBACK_T       pCbRxHTRDone;       //!< Rx Done(Read w/o Enc[HT mode])
    RCS730_CALLBACK_T       pCbRxHTWDone;       //!< Rx Done(Write w/o Enc[HT mode])
#if 0
    RCS730_CALLBACK_T       pCbTxDone;          //!< Tx Done
    RCS730_CALLBACK_T       pCbRxDepDone;       //!< Rx Done(DEP mode)
    RCS730_CALLBACK_T       pCbOther;           //!< Other IRQ interrupt
#endif
} RCS730_callbacktable_t;


/** constructor
 *
 */
void RCS730_init(void);


/** Set Callback Table
 *
 * @param   [in]        pInitTable      callback table
 */
void RCS730_setCallbackTable(const RCS730_callbacktable_t *pInitTable);


#if 0
/** Byte Write(1byte)
 *
 * @param   [in]    MemAddr     memory address to write
 * @param   [in]    pData       data to write
 * @retval  0       success
 */
int RCS730_byteWrite(uint16_t MemAddr, uint8_t Data);
#endif


/** Page Write
 *
 * @param   [in]    MemAddr     memory address to write
 * @param   [in]    pData       data to write
 * @param   [in]    Length      pData Length
 * @retval  0       success
 */
int RCS730_pageWrite(uint16_t MemAddr, const uint8_t *pData, uint8_t Length);


#if 0
/** Random Read(1byte)
 *
 * @param   [in]    MemAddr     memory address to read
 * @param   [out]   pData       data buffer to read
 * @retval  0       success
 */
int RCS730_randomRead(uint16_t MemAddr, uint8_t *pData);
#endif


/** Sequential Read
 *
 * @param   [in]    MemAddr     memory address to read
 * @param   [out]   pData       data buffer to read
 * @param   [in]    Length      pData Length
 * @retval  0       success
 */
int RCS730_sequentialRead(uint16_t MemAddr, uint8_t *pData, uint8_t Length);


#if 0
/** Current Address Read(1byte)
 *
 * @param   [out]   pData       data buffer to read
 * @retval  0       success
 */
int RCS730_currentAddrRead(uint8_t *pData);
#endif


/** Read Register
 *
 * @param   [in]    Reg         FeliCa Link Register
 * @param   [out]   pData       data buffer to read
 * @retval  0       success
 */
int RCS730_readRegister(uint16_t Reg, uint32_t *pData);


/** Write Register Force
 *
 * @param   [in]    Reg         FeliCa Link Register
 * @param   [in]    Data        data buffer to write
 * @retval  0       success
 */
int RCS730_writeRegisterForce(uint16_t Reg, uint32_t Data);


/** Write Register
 *
 * Write Register if not same value.
 *
 * @param   [in]    Reg         FeliCa Link Register
 * @param   [in]    Data        data buffer to write
 * @param   [in]    Mask        write mask
 * @retval  0       success
 *
 * @note
 *      - this API like below:
 *          @code
 *              uint32_t val_old = REG[Reg];
 *              uint32_t val_new = (val_old & ~Mask) | Data;
 *              if (val_old != val_new) {
 *                  REG[Reg] = val_new;
 *              }
 *          @endcode
 */
int RCS730_writeRegister(uint16_t Reg, uint32_t Data, uint32_t Mask);


/** Set operation mode
 *
 * @param   [in]    Mode        Operation Mode
 * @retval  0       success
 *
 * @note
 *      - This value is written to non-volatile memory in FeliCa Link.
 */
int RCS730_setRegOpMode(RCS730_OpMode Mode);


/** Set I2C Slave Address
 *
 * @param   [in]    SAddr       Slave Address(7bit address)
 * @retval  0       success
 *
 * @attention
 *      - SAddr is "7bit" address(not 8bit address).
 *
 * @note
 *      - This value is written to non-volatile memory in FeliCa Link.
 *      - Default slave address is 0x40.
 */
int RCS730_setRegSlaveAddr(int SAddr);


/** Set interrupt mask
 *
 * @param   [in]    Mask        Bit Mask
 * @param   [in]    Value       Set value to Mask
 * @retval  0       success
 *
 * @note
 *      - This value is written to non-volatile memory in FeliCa Link.
 */
int RCS730_setRegInterruptMask(uint32_t Mask, uint32_t Value);


/** Set System Code in Plug mode
 *
 * @param   [in]    SysCode     System Code
 * @retval  0       success
 *
 * @note
 *      - This value is written to non-volatile memory in FeliCa Link.
 */
int RCS730_setRegPlugSysCode(RCS730_PlugSysCode SysCode);


/** go to initialize status
 *
 * @retval  0       success
 */
int RCS730_goToInitializeStatus(void);


/** initialize to FeliCa Through(FT) mode
 *
 * @param   [in]    Mode    Operation Mode(OPMODE_LITES_HT or OPMODE_PLUG)
 * @retval  0       success
 */
int RCS730_initFTMode(RCS730_OpMode Mode);


#if 0
/** initialize to NFC-DEP mode
 *
 * @retval  0       success
 */
int RCS730_initNfcDepMode(void);
#endif


/** Interrupt Service Routine(IRQ pin)
 *
 */
void RCS730_isrIrq(void);

#endif /* RCS730_H */
