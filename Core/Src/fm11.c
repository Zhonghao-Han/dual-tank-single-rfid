#include "fm11.h"

#include "i2c.h"
#include "main.h"

#define FM11_DEVICE_ADDRESS 0xAE  // shifted address required by ST HAL
#define FM11_RESET_SILENCE_REG 0xFFE6
#define FM11_RF_TXEN_REG 0xFFF4

#define FM11_SAK2_EEPROM 0x03BF
#define FM11_USER_CFG_EEPROM 0x0390

uint8_t sak2[1] = {0x00};
uint8_t user_cfg[4] = {0x90, 0x84, 0x21, 0xCA};

void FM11_write_eeprom_page(uint16_t addr, uint8_t* value, uint8_t len) {
    HAL_I2C_Mem_Write(&hi2c1, FM11_DEVICE_ADDRESS, addr, I2C_MEMADD_SIZE_16BIT, value, len, 100);
    HAL_Delay(10);  // waiting for EEPROM operation
}

void FM11_write_reg(uint16_t addr, uint8_t value) {
    HAL_I2C_Mem_Write(&hi2c1, FM11_DEVICE_ADDRESS, addr, I2C_MEMADD_SIZE_16BIT, &value, 1, 100);
}

uint8_t FM11_read_reg(uint16_t addr) {
    uint8_t data = 0;
    HAL_I2C_Mem_Read(&hi2c1, FM11_DEVICE_ADDRESS, addr, I2C_MEMADD_SIZE_16BIT, &data, 1, 100);
    return data;
}

void FM11_init() {
    FM11_enable();
    FM11_silence();
    FM11_write_eeprom_page(FM11_SAK2_EEPROM, sak2, 1);
    FM11_write_eeprom_page(FM11_USER_CFG_EEPROM, user_cfg, 4);
    FM11_unsilence();
    FM11_write_reg(FM11_RF_TXEN_REG, 0x77);
    FM11_soft_reset();
    FM11_disable();
}

void FM11_silence() {
    FM11_write_reg(FM11_RESET_SILENCE_REG, 0x33);
}

void FM11_unsilence() {
    FM11_write_reg(FM11_RESET_SILENCE_REG, 0xCC);
}

void FM11_soft_reset() {
    FM11_write_reg(FM11_RESET_SILENCE_REG, 0x55);
}

void FM11_write_eeprom(uint16_t addr, const uint8_t* value, uint8_t length) {
}

void FM11_enable() {
    HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
}

void FM11_disable() {
    HAL_Delay(1);
    HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_SET);
}
