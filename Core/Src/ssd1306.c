#include "ssd1306.h"
#include <string.h>

static I2C_HandleTypeDef *oled_i2c;
static uint8_t oled_buffer[SSD1306_WIDTH * SSD1306_PAGES];

static uint8_t oled_x = 0;
static uint8_t oled_page = 0;

static void OLED_Command(uint8_t cmd)
{
    uint8_t data[2];

    data[0] = 0x00;   // command
    data[1] = cmd;

    HAL_I2C_Master_Transmit(oled_i2c, SSD1306_ADDR, data, 2, 100);
}

static void OLED_GetChar(char c, uint8_t font[5])
{
    font[0] = 0x00;
    font[1] = 0x00;
    font[2] = 0x00;
    font[3] = 0x00;
    font[4] = 0x00;

    switch (c)
    {
        case '0': font[0]=0x3E; font[1]=0x51; font[2]=0x49; font[3]=0x45; font[4]=0x3E; break;
        case '1': font[0]=0x00; font[1]=0x42; font[2]=0x7F; font[3]=0x40; font[4]=0x00; break;
        case '2': font[0]=0x42; font[1]=0x61; font[2]=0x51; font[3]=0x49; font[4]=0x46; break;
        case '3': font[0]=0x21; font[1]=0x41; font[2]=0x45; font[3]=0x4B; font[4]=0x31; break;
        case '4': font[0]=0x18; font[1]=0x14; font[2]=0x12; font[3]=0x7F; font[4]=0x10; break;
        case '5': font[0]=0x27; font[1]=0x45; font[2]=0x45; font[3]=0x45; font[4]=0x39; break;
        case '6': font[0]=0x3C; font[1]=0x4A; font[2]=0x49; font[3]=0x49; font[4]=0x30; break;
        case '7': font[0]=0x01; font[1]=0x71; font[2]=0x09; font[3]=0x05; font[4]=0x03; break;
        case '8': font[0]=0x36; font[1]=0x49; font[2]=0x49; font[3]=0x49; font[4]=0x36; break;
        case '9': font[0]=0x06; font[1]=0x49; font[2]=0x49; font[3]=0x29; font[4]=0x1E; break;

        case 'A': font[0]=0x7E; font[1]=0x11; font[2]=0x11; font[3]=0x11; font[4]=0x7E; break;
        case 'C': font[0]=0x3E; font[1]=0x41; font[2]=0x41; font[3]=0x41; font[4]=0x22; break;
        case 'E': font[0]=0x7F; font[1]=0x49; font[2]=0x49; font[3]=0x49; font[4]=0x41; break;
        case 'G': font[0]=0x3E; font[1]=0x41; font[2]=0x49; font[3]=0x49; font[4]=0x7A; break;
        case 'H': font[0]=0x7F; font[1]=0x08; font[2]=0x08; font[3]=0x08; font[4]=0x7F; break;
        case 'I': font[0]=0x00; font[1]=0x41; font[2]=0x7F; font[3]=0x41; font[4]=0x00; break;
        case 'L': font[0]=0x7F; font[1]=0x40; font[2]=0x40; font[3]=0x40; font[4]=0x40; break;
        case 'M': font[0]=0x7F; font[1]=0x02; font[2]=0x0C; font[3]=0x02; font[4]=0x7F; break;
        case 'N': font[0]=0x7F; font[1]=0x04; font[2]=0x08; font[3]=0x10; font[4]=0x7F; break;
        case 'O': font[0]=0x3E; font[1]=0x41; font[2]=0x41; font[3]=0x41; font[4]=0x3E; break;
        case 'P': font[0]=0x7F; font[1]=0x09; font[2]=0x09; font[3]=0x09; font[4]=0x06; break;
        case 'R': font[0]=0x7F; font[1]=0x09; font[2]=0x19; font[3]=0x29; font[4]=0x46; break;
        case 'S': font[0]=0x46; font[1]=0x49; font[2]=0x49; font[3]=0x49; font[4]=0x31; break;
        case 'T': font[0]=0x01; font[1]=0x01; font[2]=0x7F; font[3]=0x01; font[4]=0x01; break;
        case 'U': font[0]=0x3F; font[1]=0x40; font[2]=0x40; font[3]=0x40; font[4]=0x3F; break;
        case 'W': font[0]=0x7F; font[1]=0x20; font[2]=0x18; font[3]=0x20; font[4]=0x7F; break;
        case 'Y': font[0]=0x07; font[1]=0x08; font[2]=0x70; font[3]=0x08; font[4]=0x07; break;
case 'J': font[0]=0x20; font[1]=0x40; font[2]=0x41; font[3]=0x3F; font[4]=0x01; break;
case 'K': font[0]=0x7F; font[1]=0x08; font[2]=0x14; font[3]=0x22; font[4]=0x41; break;
case 'V': font[0]=0x1F; font[1]=0x20; font[2]=0x40; font[3]=0x20; font[4]=0x1F; break;
case 'Z': font[0]=0x61; font[1]=0x51; font[2]=0x49; font[3]=0x45; font[4]=0x43; break;
				case 'a': font[0]=0x20; font[1]=0x54; font[2]=0x54; font[3]=0x54; font[4]=0x78; break;
				case 'i': font[0]=0x00; font[1]=0x44; font[2]=0x7D; font[3]=0x40; font[4]=0x00; break;
				case 'p': font[0]=0x7C; font[1]=0x14; font[2]=0x14; font[3]=0x14; font[4]=0x08; break;
				case 'z': font[0]=0x44; font[1]=0x64; font[2]=0x54; font[3]=0x4C; font[4]=0x44; break;
				case 'o': font[0]=0x38; font[1]=0x44; font[2]=0x44; font[3]=0x44; font[4]=0x38; break;
				case 'n': font[0]=0x7C; font[1]=0x08; font[2]=0x04; font[3]=0x04; font[4]=0x78; break;
				case 's': font[0]=0x48; font[1]=0x54; font[2]=0x54; font[3]=0x54; font[4]=0x20; break;
				case 'D': font[0]=0x7F; font[1]=0x41; font[2]=0x41; font[3]=0x22; font[4]=0x1C; break;

        case ' ': font[0]=0x00; font[1]=0x00; font[2]=0x00; font[3]=0x00; font[4]=0x00; break;
        case '.': font[0]=0x00; font[1]=0x60; font[2]=0x60; font[3]=0x00; font[4]=0x00; break;
        case ':': font[0]=0x00; font[1]=0x36; font[2]=0x36; font[3]=0x00; font[4]=0x00; break;
        case '=': font[0]=0x14; font[1]=0x14; font[2]=0x14; font[3]=0x14; font[4]=0x14; break;
        case '-': font[0]=0x08; font[1]=0x08; font[2]=0x08; font[3]=0x08; font[4]=0x08; break;
				

        default: break;
    }
}

void OLED_Init(I2C_HandleTypeDef *hi2c)
{
    oled_i2c = hi2c;

    HAL_Delay(100);

    OLED_Command(0xAE); // display off
    OLED_Command(0x20); // memory mode
    OLED_Command(0x02); // horizontal addressing
    OLED_Command(0xB0);
    OLED_Command(0xC8);
    OLED_Command(0x00);
    OLED_Command(0x10);
    OLED_Command(0x40);
    OLED_Command(0x81);
    OLED_Command(0x7F);
    OLED_Command(0xA1);
    OLED_Command(0xA6);
    OLED_Command(0xA8);

#if SSD1306_PAGES == 8
    OLED_Command(0x3F); // 128x64
#else
    OLED_Command(0x1F); // 128x32
#endif

    OLED_Command(0xA4);
    OLED_Command(0xD3);
    OLED_Command(0x00);
    OLED_Command(0xD5);
    OLED_Command(0x80);
    OLED_Command(0xD9);
    OLED_Command(0xF1);
    OLED_Command(0xDA);

#if SSD1306_PAGES == 8
    OLED_Command(0x12); // 128x64
#else
    OLED_Command(0x02); // 128x32
#endif

    OLED_Command(0xDB);
    OLED_Command(0x40);
    OLED_Command(0x8D);
    OLED_Command(0x14);
    OLED_Command(0xAF); // display on

    OLED_Clear();
    OLED_Update();
}

void OLED_Clear(void)
{
    for (uint16_t i = 0; i < SSD1306_WIDTH * SSD1306_PAGES; i++)
    {
        oled_buffer[i] = 0x00;
    }

    oled_x = 0;
    oled_page = 0;
}

void OLED_Update(void)
{
    uint8_t data[129];

    data[0] = 0x40; // data

    for (uint8_t page = 0; page < SSD1306_PAGES; page++)
    {
        OLED_Command(0xB0 + page);
        OLED_Command(0x00);
        OLED_Command(0x10);

        for (uint8_t i = 0; i < SSD1306_WIDTH; i++)
        {
            data[i + 1] = oled_buffer[page * SSD1306_WIDTH + i];
        }

        HAL_I2C_Master_Transmit(oled_i2c, SSD1306_ADDR, data, 129, 100);
    }
}

void OLED_SetCursor(uint8_t x, uint8_t page)
{
    oled_x = x;
    oled_page = page;
}

void OLED_WriteChar(char c)
{
    uint8_t font[5];

    if (oled_page >= SSD1306_PAGES)
    {
        return;
    }

    if (oled_x > 122)
    {
        oled_x = 0;
        oled_page++;
    }

    if (oled_page >= SSD1306_PAGES)
    {
        return;
    }

    OLED_GetChar(c, font);

    for (uint8_t i = 0; i < 5; i++)
    {
        oled_buffer[oled_page * SSD1306_WIDTH + oled_x] = font[i];
        oled_x++;
    }

    oled_buffer[oled_page * SSD1306_WIDTH + oled_x] = 0x00;
    oled_x++;
}

void OLED_WriteString(const char *str)
{
    while (*str)
    {
        OLED_WriteChar(*str);
        str++;
    }
}
