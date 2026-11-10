#include "oled.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

static uint8_t oledBuffer[8][128];
static uint8_t oledAddr = OLED_I2C_ADDR;

static const uint8_t fontDigit[10][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E}
};

static const uint8_t fontUpper[26][5] = {
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    {0x3E, 0x41, 0x51, 0x21, 0x5E},
    {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    {0x3F, 0x40, 0x38, 0x40, 0x3F},
    {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07},
    {0x61, 0x51, 0x49, 0x45, 0x43}
};

static const uint8_t fontSpace[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t fontColon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
static const uint8_t fontPercent[5] = {0x23, 0x13, 0x08, 0x64, 0x62};
static const uint8_t fontLowerS[5] = {0x48, 0x54, 0x54, 0x54, 0x20};
static const uint8_t fontDash[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
static const uint8_t fontDot[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
static const uint8_t fontPlus[5] = {0x08, 0x08, 0x3E, 0x08, 0x08};

static const uint8_t *OLED_GetGlyph(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
    {
        return fontDigit[ch - '0'];
    }
    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return fontUpper[ch - 'A'];
    }
    if (ch == 's')
    {
        return fontLowerS;
    }
    if ((ch >= 'a') && (ch <= 'z'))
    {
        return fontUpper[ch - 'a'];
    }

    switch (ch)
    {
        case ':': return fontColon;
        case '%': return fontPercent;
        case '-': return fontDash;
        case '.': return fontDot;
        case '+': return fontPlus;
        default:  return fontSpace;
    }
}

static void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(&hi2c1, oledAddr, data, 2, 100);
}

static void OLED_WriteData(const uint8_t *data, uint16_t len)
{
    uint8_t buffer[129];

    if (len > 128U)
    {
        len = 128U;
    }
    buffer[0] = 0x40;
    memcpy(&buffer[1], data, len);
    HAL_I2C_Master_Transmit(&hi2c1, oledAddr, buffer, (uint16_t)(len + 1U), 200);
}

static void OLED_DrawPixel(uint8_t x, uint8_t y)
{
    if ((x < 128U) && (y < 64U))
    {
        oledBuffer[y / 8U][x] |= (uint8_t)(1U << (y % 8U));
    }
}

void OLED_Init(void)
{
    HAL_Delay(100);

    oledAddr = OLED_I2C_ADDR;
    if (HAL_I2C_IsDeviceReady(&hi2c1, 0x78, 3, 50) != HAL_OK)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1, 0x7A, 3, 50) == HAL_OK)
        {
            oledAddr = 0x7A;
        }
    }

    OLED_WriteCmd(0xAE);
    OLED_WriteCmd(0xD5);
    OLED_WriteCmd(0x80);
    OLED_WriteCmd(0xA8);
    OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xD3);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x8D);
    OLED_WriteCmd(0x14);
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xA1);
    OLED_WriteCmd(0xC8);
    OLED_WriteCmd(0xDA);
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0x81);
    OLED_WriteCmd(0xCF);
    OLED_WriteCmd(0xD9);
    OLED_WriteCmd(0xF1);
    OLED_WriteCmd(0xDB);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0xA4);
    OLED_WriteCmd(0xA6);
    OLED_WriteCmd(0xAF);
    OLED_Clear();
    OLED_Refresh();
}

void OLED_Clear(void)
{
    memset(oledBuffer, 0, sizeof(oledBuffer));
}

void OLED_Refresh(void)
{
    for (uint8_t page = 0; page < 8U; page++)
    {
        OLED_WriteCmd((uint8_t)(0xB0U + page));
        OLED_WriteCmd(0x00);
        OLED_WriteCmd(0x10);
        OLED_WriteData(oledBuffer[page], 128);
    }
}

void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t scale)
{
    const uint8_t *glyph = OLED_GetGlyph(ch);

    if (scale == 0U)
    {
        scale = 1U;
    }

    for (uint8_t col = 0; col < 5U; col++)
    {
        for (uint8_t row = 0; row < 7U; row++)
        {
            if ((glyph[col] & (uint8_t)(1U << row)) != 0U)
            {
                for (uint8_t dx = 0; dx < scale; dx++)
                {
                    for (uint8_t dy = 0; dy < scale; dy++)
                    {
                        OLED_DrawPixel((uint8_t)(x + col * scale + dx),
                                       (uint8_t)(y + row * scale + dy));
                    }
                }
            }
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t scale)
{
    if (scale == 0U)
    {
        scale = 1U;
    }

    while (*str != '\0')
    {
        OLED_ShowChar(x, y, *str, scale);
        x = (uint8_t)(x + 6U * scale);
        str++;
        if (x > (uint8_t)(128U - 6U * scale))
        {
            break;
        }
    }
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t scale)
{
    uint32_t div = 1;

    if (scale == 0U)
    {
        scale = 1U;
    }
    for (uint8_t i = 1; i < len; i++)
    {
        div *= 10U;
    }
    for (uint8_t i = 0; i < len; i++)
    {
        char ch = (char)('0' + (num / div) % 10U);
        OLED_ShowChar((uint8_t)(x + i * 6U * scale), y, ch, scale);
        if (div >= 10U)
        {
            div /= 10U;
        }
    }
}
