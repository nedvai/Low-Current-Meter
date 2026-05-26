#ifndef SSD1306_SIMPLE_H
#define SSD1306_SIMPLE_H

#include "stm32l0xx_hal.h"
#include <stdint.h>

#define SSD1306_ADDR   (0x3C << 1)
#define SSD1306_WIDTH  128
#define SSD1306_PAGES  4   // 4 - 128x32 OLED 8 - 128x64

void OLED_Init(I2C_HandleTypeDef *hi2c);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_SetCursor(uint8_t x, uint8_t page);
void OLED_WriteChar(char c);
void OLED_WriteString(const char *str);


#endif