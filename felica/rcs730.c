/** FeliCa Link(RC-S730) Library
 *
 * @file    rcs730.c
 * @author  hiro99ma
 * @version 1.00
 */

#include <string.h>
#include "rcs730.h"
#include "twi_master.h"


#define I2C_SLV_ADDR    (0x80)      //Default Slave Address(8bit)
#define RETRY_NUM       (10)         //max I2C Retry count


static uint8_t                  _slvAddr;
static RCS730_callbacktable_t   _cbTable;


__STATIC_INLINE int set_tag_rf_send_enable(void)
{
    uint32_t val = 0x00000001;
    return RCS730_pageWrite(RCS730_REG_TAG_TX_CTRL, (const uint8_t*)&val, sizeof(val));
}

static int read_rf_buf(uint8_t *pData)
{
    const int LEN_FIRST = 16;
    int len = 0;
    int ret;

    //read from LEN
    ret = RCS730_sequentialRead(RCS730_BUF_RF_COMM, pData, LEN_FIRST);
    if (ret == 0) {
        len = pData[0];
    }
    if ((ret == 0) && (pData[0] > LEN_FIRST)) {
        ret = RCS730_sequentialRead(RCS730_BUF_RF_COMM + LEN_FIRST, pData + LEN_FIRST, pData[0] - LEN_FIRST);
        if (ret != 0) {
            len = 0;
        }
    }

    return len;
}


void RCS730_init(void)
{
    _slvAddr = I2C_SLV_ADDR;
    _cbTable.pUserData = 0;
    _cbTable.pCbRxHTRDone = 0;
    _cbTable.pCbRxHTWDone = 0;
}


__INLINE void RCS730_setCallbackTable(const RCS730_callbacktable_t *pInitTable)
{
    _cbTable = *pInitTable;
}


#if 0
int RCS730_byteWrite(uint16_t MemAddr, uint8_t Data)
{
    int ret;
    int retry = RETRY_NUM;
    uint8_t buf[3];

    buf[0] = (char)(MemAddr >> 8);
    buf[1] = (char)(MemAddr & 0xff);
    buf[2] = (char)Data;

    do {
        ret = i2c_write(_slvAddr, buf, (int)sizeof(buf), false);
    } while ((ret != 0) && (retry--));

    return ret;
}
#endif


int RCS730_pageWrite(uint16_t MemAddr, const uint8_t *pData, uint8_t Length)
{
    bool ret;
    int retry = RETRY_NUM;
    uint8_t buf[256];

    if (Length > 254) {
        return NRF_ERROR_INVALID_PARAM;
    }

    buf[0] = (char)(MemAddr >> 8);
    buf[1] = (char)(MemAddr & 0xff);
    memcpy(&buf[2], pData, Length);

    do {
        ret = twi_master_transfer(_slvAddr, buf, (uint8_t)(2 + Length), TWI_ISSUE_STOP);
    } while (!ret && (retry--));

    return (int)((ret) ? NRF_SUCCESS : NRF_ERROR_INTERNAL);
}


#if 0
__INLINE int RCS730_randomRead(uint16_t MemAddr, uint8_t *pData)
{
    return RCS730_sequentialRead(MemAddr, pData, 1);
}
#endif


int RCS730_sequentialRead(uint16_t MemAddr, uint8_t *pData, uint8_t Length)
{
    bool ret;
    int retry = RETRY_NUM;
    uint8_t buf[256];

    if (Length > 255) {
        return NRF_ERROR_INVALID_PARAM;
    }

    buf[0] = (char)(MemAddr >> 8);
    buf[1] = (char)(MemAddr & 0xff);

    do {
        ret = twi_master_transfer(_slvAddr, buf, 2, TWI_DONT_ISSUE_STOP);
    } while (!ret && (retry--));
    if (ret) {
        ret = twi_master_transfer((uint8_t)(_slvAddr | TWI_READ_BIT), pData, Length, TWI_ISSUE_STOP);
    }

    return (int)((ret) ? NRF_SUCCESS : NRF_ERROR_INTERNAL);
}


#if 0
int RCS730_currentAddrRead(uint8_t *pData)
{
    int ret;
    int retry = RETRY_NUM;

    do {
        ret = i2c_read((int)(_slvAddr | 1), pData, 1);
    } while ((ret != 0) && (retry--));

    return ret;
}
#endif


__INLINE int RCS730_readRegister(uint16_t Reg, uint32_t* pData)
{
    return RCS730_sequentialRead(Reg, (uint8_t*)pData, sizeof(uint32_t));
}


__INLINE int RCS730_writeRegisterForce(uint16_t Reg, uint32_t Data)
{
    return RCS730_pageWrite(Reg, (const uint8_t*)&Data, sizeof(Data));
}


int RCS730_writeRegister(uint16_t Reg, uint32_t Data, uint32_t Mask)
{
    int ret;
    uint32_t cur;   //current register value

    ret = RCS730_readRegister(Reg, &cur);
    if (ret == 0) {
        if ((cur & Mask) != Data) {
            // change value
            Data |= cur & ~Mask;
            ret = RCS730_writeRegisterForce(Reg, Data);
        }
    }

    return ret;
}


__INLINE int RCS730_setRegOpMode(RCS730_OpMode Mode)
{
    return RCS730_writeRegister(RCS730_REG_OPMODE, (uint32_t)Mode, RCS730_REG_MASK_VAL);
}


int RCS730_setRegSlaveAddr(int SAddr)
{
    int ret;

    ret = RCS730_writeRegister(RCS730_REG_I2C_SLAVE_ADDR, (uint32_t)SAddr, RCS730_REG_MASK_VAL);

    if (ret == 0) {
        _slvAddr = SAddr << 1;
    }

    return ret;
}


__INLINE int RCS730_setRegInterruptMask(uint32_t Mask, uint32_t Value)
{
    return RCS730_writeRegister(RCS730_REG_INT_MASK, Value, Mask);
}


__INLINE int RCS730_setRegPlugSysCode(RCS730_PlugSysCode SysCode)
{
    return RCS730_writeRegister(RCS730_REG_PLUG_CONF1, (uint32_t)SysCode, 0x00000002);
}


__INLINE int RCS730_goToInitializeStatus(void)
{
    return RCS730_writeRegisterForce(RCS730_REG_INIT_CTRL, 0x0000004a);
}


int RCS730_initFTMode(RCS730_OpMode Mode)
{
    int ret;

    if (RCS730_OPMODE_PLUG < Mode) {
        return -1;
    }

    ret = RCS730_setRegOpMode(Mode);
    if (ret == 0) {
        ret = RCS730_setRegInterruptMask(RCS730_MSK_INT_TAG_RW_RX_DONE2, 0);
    }

    return ret;
}


#if 0
int RCS730_initNfcDepMode(void)
{
    int ret;

    ret = RCS730_setRegOpMode(RCS730_OPMODE_NFCDEP);
    if (ret == 0) {
        ret = RCS730_setRegInterruptMask(RCS730_MSK_INT_TAG_NFC_DEP_RX_DONE, 0);
    }

    return ret;
}
#endif


void RCS730_isrIrq(void)
{
    int ret;
    bool b_send = false;
    uint32_t intstat;
    uint8_t rf_buf[256];

    ret = RCS730_readRegister(RCS730_REG_INT_STATUS, &intstat);
    if (ret == 0) {

        if (intstat & RCS730_MSK_INT_TAG_RW_RX_DONE2) {
            //Read or Write w/o Enc Rx done for HT block
            int len = read_rf_buf(rf_buf);
            if (len > 0) {
                switch (rf_buf[1]) {
                    case 0x06:  //Read w/o Enc
                        if (_cbTable.pCbRxHTRDone) {
                            b_send = (*_cbTable.pCbRxHTRDone)(_cbTable.pUserData, rf_buf, len);
                        }
                        break;
                    case 0x08:  //Write w/o Enc;
                        if (_cbTable.pCbRxHTWDone) {
                            b_send = (*_cbTable.pCbRxHTWDone)(_cbTable.pUserData, rf_buf, len);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
#if 0
        if (_cbTable.pCbRxDepDone && (intstat & RCS730_MSK_INT_TAG_NFC_DEP_RX_DONE)) {
            //DEP command Rx done
            int len = read_rf_buf(this, rf_buf);
            (*_cbTable.pCbRxDepDone)(_cbTable.pUserData, rf_buf, len);
        }
        if (_cbTable.pCbTxDone && (intstat & MSK_INT_TAG_TX_DONE)) {
            //Tx Done
            int len = read_rf_buf(this, rf_buf);
            (*_cbTable.pCbTxDone)(_cbTable.pUserData, rf_buf, len);
        }

        uint32_t intother = intstat & ~(RCS730_MSK_INT_TAG_TX_DONE | RCS730_MSK_INT_TAG_NFC_DEP_RX_DONE | RCS730_MSK_INT_TAG_RW_RX_DONE2);
        if (_cbTable.pCbOther && intother) {
            (*_cbTable.mCbOther)(_cbTable.pUserData, 0, intother);
        }
#endif

        //response
        if (b_send) {
            ret = RCS730_pageWrite(RCS730_BUF_RF_COMM, rf_buf, rf_buf[0]);
            if (ret == 0) {
                set_tag_rf_send_enable();
            }
        }

        RCS730_writeRegisterForce(RCS730_REG_INT_CLEAR, intstat);
    }
}
