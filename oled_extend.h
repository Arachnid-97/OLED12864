#ifndef __OLED_EXTEND_H
#define __OLED_EXTEND_H


#include "N76E003.h"
#include "SFR_Macro.h"
#include "Function_define.h"

//enum week
//{
//	MON=1, TUE, WED, THU, FRI, SAT, SUN
//}day;

#define MON			1
#define TUE			2
#define WED			3
#define THU			4
#define FRI			5
#define SAT			6
#define SUN			7

#define OLED_LEFT_CLEAR			0
#define OLED_RIGH_CLEAR			1

#define OLED_BUFFER_ROW			2		// 显存显示首行
#define OLED_BUFF_ROW_SIZE		5		// 显存显示行数

extern bit g_OLED_Updata_flag;

//extern uint8_t OLED_GRAM[128][OLED_BUFF_ROW_SIZE];
extern uint8_t OLED_GRAM[OLED_BUFF_ROW_SIZE][128];
void OLED_Refresh( uint8_t Row );

void OLED_String_Move( uint8_t X, uint8_t Y, const uint8_t *pChar, uint8_t Size, uint8_t Buffer, uint8_t Inverse );
void OLED_Num_Move( uint8_t X, uint8_t Y, uint32_t Num, uint8_t Len, uint8_t Size, uint8_t Prefix, uint8_t Buffer, uint8_t Inverse );
void OLED_Refresh( uint8_t Row );
void OLED_P8x8( uint8_t X, uint8_t Y, uint16_t Queue, uint8_t Buffer, uint16_t Inverse );
void OLED_P16x16( uint8_t X, uint8_t Y, uint16_t Queue, uint8_t Buffer, uint16_t Inverse );
void OLED_P32x32( uint8_t X, uint8_t Y, uint16_t Queue, uint8_t Buffer, uint16_t Inverse );
void OLED_P16x32( uint8_t X, uint8_t Y, uint16_t Queue, uint8_t Buffer, uint16_t Inverse );
void OLED_Period_Show( uint8_t X, uint8_t Y, uint8_t Week, uint8_t Size, uint16_t Inverse );
void OLED_Starting_Up(void);


#endif /* __OLED_EXTEND_H */


/*---------------------------- END OF FILE ----------------------------*/


