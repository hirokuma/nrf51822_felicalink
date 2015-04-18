/**
 * @file    st7032i.h
 * @brief   ST7032i Library
 *
 * http://strawberry-linux.com/catalog/items?code=27001
 *
 */
#ifndef ST7032I_H
#define ST7032I_H

typedef enum ST7032I_Line {
    ST7032I_LINE1   = 0x00,
    ST7032I_LINE2   = 0x40
} ST7032I_Line;

void ST7032I_init(void);
void ST7032I_clear(void);
void ST7032I_movePos(int x, ST7032I_Line y);
void ST7032I_writeString(const char *pStr);

#endif /* ST7032I_H */
