#ifndef __OLED_H
#define __OLED_H

#include "main.h"

#define OLED_I2C_ADDR 0x78

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t scale);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t scale);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t scale);

#endif
