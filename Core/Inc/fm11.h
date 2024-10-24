#ifndef __FM11_H__
#define __FM11_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l0xx_hal.h"

#define FM11_DEVICE_ADDRESS 0xAE  // shifted address required by ST HAL
#define FM11_RESET_SILENCE_REG 0xFFE6
#define FM11_RF_TXEN_REG 0xFFF4

#define FM11_USER_DATA_START_EEPROM 0x0010
#define FM11_SAK2_EEPROM 0x03BF
#define FM11_USER_CFG_EEPROM 0x0390

void FM11_init();

void FM11_silence();

void FM11_unsilence();

void FM11_soft_reset();

void FM11_write_reg(uint16_t addr, uint8_t value);

uint8_t FM11_read_reg(uint16_t addr);

// data has to be on the same page
void FM11_write_eeprom_page(uint16_t addr, uint8_t* data, uint8_t len);

void FM11_read_eeprom_page(uint16_t addr, uint8_t* data, uint8_t len);

void FM11_enable();

void FM11_disable();

#ifdef __cplusplus
}
#endif
#endif /*__FM11_H__ */