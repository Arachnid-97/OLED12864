#include "oled_extend.h"
#include "font_array.h"
#include "oled12864_drv.h"
#include "bsp_time.h"
#include "bsp_uart.h"


bit g_OLED_Updata_flag = 0;

//uint8_t OLED_GRAM[128][OLED_BUFF_ROW_SIZE] = {0};
uint8_t OLED_GRAM[OLED_BUFF_ROW_SIZE][128] = {0};

extern code unsigned char F6x8[][6];

/************************************************
函数名称 ： OLED_String_Move
功    能 ： OLED字符串平移显示，超出不显
参    数 ： X ---- X轴
			Y ---- Y轴
            pChar ---- 数据
            Size ---- 字体大小
			Buffer ---- 缓冲区设定使能
            Inverse ---- 反白（单字体）显示使能
返 回 值 ： 无
*************************************************/
void OLED_String_Move( uint8_t X, uint8_t Y, const uint8_t *pChar, uint8_t Size, uint8_t Buffer, uint8_t Inverse )
{
	uint8_t i;
    uint8_t addr = 0;

	while(*pChar != '\0')
	{
		if(!Buffer)
		{
			if(X < 0)             // 列反向溢出
			{
//				OLED_Coord(0, Y);
//				OLED_Write_Data(OLED_CLS, DISABLE);

//				OLED_ShowChar(0, Y, ' ',OLED_FONT_EIGHT, DISABLE);
				break;
			}
			else if(X > OLED_MAX_COLUMN - 8)	// 列正向溢出
			{
//				OLED_Coord(127, Y);
//				OLED_Write_Data(OLED_CLS, DISABLE);

//				OLED_ShowChar(127, Y, ' ',OLED_FONT_EIGHT, DISABLE);
				break;
			}
			OLED_ShowChar(X, Y, *pChar, Size, Inverse);
			X += 6;
		}
		else
		{
			addr = *pChar - 32;
			
//			if(Size == OLED_FONT_SIXTEEN)                               // 8x16
//			{
//				for(i = 0;i < 8;i++)
//				{
//					OLED_GRAM[Y - OLED_BUFFER_ROW][X + i] = F8x16[addr][i];
//				}
//				for(i = 0;i < 8;i++)
//				{
//					OLED_GRAM[Y - OLED_BUFFER_ROW + 1][X + i] = F8x16[addr][i + 8];
//				}
//				X += 8;
//			}
//			else if(Size == OLED_FONT_EIGHT)                           // 6x8
				
			if(Size == OLED_FONT_EIGHT)
			{
				for(i = 0;i < 6;i++)
				{
					OLED_GRAM[Y - OLED_BUFFER_ROW][X + i] = F6x8[addr][i];
				}
				X += 6;				// 存入缓存区宽度为 6
			}
		}
		pChar++;
	}
}

/************************************************
函数名称 ： OLED_Num_Move
功    能 ： OLED数字平移显示，超出不显
参    数 ： X ---- X轴
			Y ---- Y轴
            Num ---- 数字
            Len ---- 位数
            Size ---- 字体大小
			Prefix ---- 补零显示使能
			Buffer ---- 缓冲区设定使能
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_Num_Move( uint8_t X, uint8_t Y, uint32_t Num, uint8_t Len, uint8_t Size, uint8_t Prefix, uint8_t Buffer, uint8_t Inverse )
{
	char temp = 0;
	uint8_t t,i;
	uint8_t show = 0;

	for(t = 0;t < Len;t++)
	{
		temp = (Num / OLED_Power(10, Len - t - 1)) % 10;          // 提取每位数字

		if(!show && t < (Len - 1))
		{
			if(0 == temp)
			{
				if(Prefix)
					temp = 0;
				else
					temp = -16;
			}
            else
            {
                show = 1;
            }
		}
		if(!Buffer)
		{
			if(X < 0)             // 列反向溢出
			{
//				OLED_Coord(0, Y);
//				OLED_Write_Data(OLED_CLS, DISABLE);

				break;
			}
			else if(X > OLED_MAX_COLUMN - 8)	// 列正向溢出
			{
//				OLED_Coord(127, Y);
//				OLED_Write_Data(OLED_CLS, DISABLE);
				
				break;
			}
			OLED_ShowChar(X + 8 *t, Y, temp + '0', Size, Inverse);
			X += 6;
		}
		else
		{
//			if(Size == OLED_FONT_SIXTEEN)                               // 8x16
//			{
//				for(i = 0;i < 8;i++)
//				{
//					OLED_GRAM[Y - OLED_BUFFER_ROW][X + i] = F8x16[temp + 16][i];
//				}
//				for(i = 0;i < 8;i++)
//				{
//					OLED_GRAM[Y - OLED_BUFFER_ROW + 1][X + i] = F8x16[temp + 16][i + 8];
//				}
//				X += 8;
//			}
//			else if(Size == OLED_FONT_EIGHT)                           // 6x8

			if(Size == OLED_FONT_EIGHT)
			{
				for(i = 0;i < 6;i++)
				{
					OLED_GRAM[Y - OLED_BUFFER_ROW][X + i] = F6x8[temp + 16][i];
				}
				X += 6;
			}
		}
	}
}

/************************************************
函数名称 ： OLED_Refresh
功    能 ： 更新显存到 OLED
参    数 ： Row ---- 首行(最好通过输入宏 OLED_BUFFER_ROW修改)
返 回 值 ： 无
*************************************************/
void OLED_Refresh( uint8_t Row )
{
	uint8_t n = 0,i = 0;
    uint8_t y,x = 0;

    for(y = 0;y < OLED_BUFF_ROW_SIZE;y++)
    {
        OLED_Write_Cmd(0xB0 + Row + y);         // 第 n页开始（0~7）
        OLED_Write_Cmd(0x10);                   // 设置显示位置 - 列高位第一个
        OLED_Write_Cmd(0x00);                   // 设置显示位置 - 列低位第一个
        for(x = 0;x < OLED_MAX_COLUMN;x++)
        {
//            OLED_Write_Data(OLED_GRAM[x][y], DISABLE);
            OLED_Write_Data(OLED_GRAM[y][x], DISABLE);
        }
    }
}

/************************************************
函数名称 ： OLED_P78x24Ch
功    能 ： OLED 78*24进度框显示
参    数 ： X ---- X轴
			Y ---- Y轴
返 回 值 ： 无
*************************************************/
void OLED_P78x24Ch( uint8_t X, uint8_t Y )
{
    uint8_t wm = 0;
    uint8_t adder = 0;

    OLED_Coord(X , Y);
    for(wm = 0; wm < 78; wm++)
    {
        OLED_Write_Data(P78x24[adder], 0);
        adder += 1;
    }
    OLED_Coord(X , Y + 1);
    for(wm = 0; wm < 78; wm++)
    {
        OLED_Write_Data(P78x24[adder], 0);
        adder += 1;
    }
    OLED_Coord(X , Y + 2);
    for(wm = 0; wm < 78; wm++)
    {
        OLED_Write_Data(P78x24[adder], 0);
        adder += 1;
    }
}

/************************************************
函数名称 ： OLED_P8x8
功    能 ： OLED8*8图像显示
参    数 ： X ---- X轴
			Y ---- Y轴
            Queue ---- 队列
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_P8x8( uint8_t X, uint8_t Y, uint16_t Queue, uint8_t Buffer, uint16_t Inverse )
{
    uint8_t wm,i;
    uint32_t addr = 8 * Queue;

	if((X >= 0) && (X <= OLED_MAX_COLUMN - 8))             // 列溢出
	{
		if(!Buffer)
		{
			OLED_Coord(X , Y);
			for(wm = 0; wm < 8; wm++)
			{
				OLED_Write_Data(P8x8[addr], Inverse);
				addr += 1;
			}
		}
		else
		{
			for(i = 0;i < 8;i++)
			{
//				OLED_GRAM[X + i][Y - OLED_BUFFER_ROW] = P8x8[addr];
				OLED_GRAM[Y - OLED_BUFFER_ROW][X + i] = P8x8[addr];
				addr++;
			}
		}
	}
}

/************************************************
函数名称 ： OLED_P16x16
功    能 ： OLED16*16图像显示
参    数 ： X ---- X轴
			Y ---- Y轴
            Queue ---- 队列
			Buffer ---- 缓冲区设定使能
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_P16x16( uint8_t X, uint8_t Y, uint16_t Queue, uint8_t Buffer, uint16_t Inverse )
{
	uint8_t i;
	uint32_t addr = Queue * 32;

	if((X >= 0) && (X <= OLED_MAX_COLUMN - 16))     // 列溢出
	{	
		if(!Buffer)
		{
			OLED_Coord(X, Y);
			for(i = 0;i < 16;i++)
			{
				OLED_Write_Data(P16x16[addr], Inverse);	// 前十六个数据
				addr++;
			}

			OLED_Coord(X, Y+1);
			for(i = 0;i < 16;i++)
			{
				OLED_Write_Data(P16x16[addr], Inverse); // 后十六个数据
				addr++;
			}
		}
		else
		{
			for(i = 0;i < 16;i++)
			{
//				OLED_GRAM[X + i][Y - OLED_BUFFER_ROW] = P16x16[addr];
				OLED_GRAM[Y - OLED_BUFFER_ROW][X + i] = P16x16[addr];
				addr++;
			}
			for(i = 0;i < 16;i++)
			{
//				OLED_GRAM[X + i][Y - OLED_BUFFER_ROW + 1] = P16x16[addr];
				OLED_GRAM[Y - OLED_BUFFER_ROW + 1][X + i] = P16x16[addr];
				addr++;
			}
		}
	}
}

/************************************************
函数名称 ： OLED_P32x32
功    能 ： OLED32*32图像显示
参    数 ： X ---- X轴
			Y ---- Y轴
            Queue ---- 队列
			Buffer ---- 缓冲区设定使能
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_P32x32( uint8_t X, uint8_t Y, uint16_t Queue, uint8_t Buffer, uint16_t Inverse )
{
	uint8_t i,j;
	uint32_t addr = Queue * 128;

	if((X >= 0) && (X <= OLED_MAX_COLUMN - 32))             // 列溢出
	{
		if(!Buffer)
		{
			for(j = 0;j < 4;j++)
			{
				OLED_Coord(X, Y + j);
				for(i = 0;i < 32;i++)
				{
					OLED_Write_Data(P32x32[addr], Inverse);
					addr++;
				}
			}
		}
		else
		{
			for(j = 0;j < 4;j++)
			{
				for(i = 0;i < 32;i++)
				{
//					OLED_GRAM[X + i][Y - OLED_BUFFER_ROW + j] = P32x32[addr];
					OLED_GRAM[Y - OLED_BUFFER_ROW + j][X + i] = P32x32[addr];
					addr++;
				}
			}
		}
	}
}

/************************************************
函数名称 ： OLED_P16x32
功    能 ： OLED16*32图像显示
参    数 ： X ---- X轴
			Y ---- Y轴
            Queue ---- 队列
			Buffer ---- 缓冲区设定使能
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_P16x32( uint8_t X, uint8_t Y, uint16_t Queue, uint8_t Buffer, uint16_t Inverse )
{
	uint8_t i,j;
	uint32_t addr = Queue * 64;

	if((X >= 0) && (X <= OLED_MAX_COLUMN - 16))             // 列溢出
	{
		if(!Buffer)
		{
			for(j = 0;j < 4;j++)
			{
				OLED_Coord(X, Y + j);
				for(i = 0;i < 16;i++)
				{
					OLED_Write_Data(P16x32[addr], Inverse);
					addr++;
				}
			}
		}
		else
		{
			for(j = 0;j < 4;j++)
			{
				for(i = 0;i < 16;i++)
				{
//					OLED_GRAM[X + i][Y - OLED_BUFFER_ROW + j] = P16x32[addr];
					OLED_GRAM[Y - OLED_BUFFER_ROW + j][X + i] = P16x32[addr];
					addr++;
				}
			}
		}
	}
}

/************************************************
函数名称 ： OLED_Period_Show
功    能 ： 星期显示
参    数 ： X ---- X轴
			Y ---- Y轴
            Week ---- 星期
			Size ---- 字体大小
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_Period_Show( uint8_t X, uint8_t Y, uint8_t Week, uint8_t Size, uint16_t Inverse )
{
	if(Week >= 8)
	{
		Week -= 7;
	}
	
	switch(Week)
	{
		case 1:
			OLED_String_Move(X, Y,"Mon. ", Size, ENABLE, Inverse);
			break;
		
		case 2:
			OLED_String_Move(X, Y,"Tues.", Size, ENABLE, Inverse);
			break;

		case 3:
			OLED_String_Move(X, Y,"Wed. ", Size, ENABLE, Inverse);
			break;

		case 4:
			OLED_String_Move(X, Y,"Thur.", Size, ENABLE, Inverse);
			break;

		case 5:
			OLED_String_Move(X, Y,"Fri. ", Size, ENABLE, Inverse);
			break;

		case 6:
			OLED_String_Move(X, Y,"Sat. ", Size, ENABLE, Inverse);
			break;

		case 7:
			OLED_String_Move(X, Y,"Sun. ", Size, ENABLE, Inverse);
			break;
		
		default:
			break;
	}
}

/************************************************
函数名称 ： OLED_Starting_Up
功    能 ： 开机图像显示
参    数 ： 无
返 回 值 ： 无
*************************************************/
void OLED_Starting_Up(void)
{
	uint8_t count = 0;
	
    OLED_ShowPrintf(15, 4, "Loading", OLED_FONT_EIGHT, DISABLE, DISABLE);
    OLED_ShowNum(79, 4, 0, 3, OLED_FONT_EIGHT, DISABLE, DISABLE);
    OLED_ShowPrintf(105, 4, "%", OLED_FONT_EIGHT, DISABLE, DISABLE);

    OLED_P78x24Ch(25, 5);		// 设置进度条外框
    SoftwareDelay_ms(100);

	/* 进度条读取 */
    OLED_P8x8(29, 6, 0, DISABLE, DISABLE);
	OLED_ShowNum(79, 4, 38, 3, OLED_FONT_EIGHT, DISABLE, DISABLE);
	SoftwareDelay_ms(200);
	OLED_P8x8(29 + 1, 6, 1, DISABLE, DISABLE);
	OLED_ShowNum(79, 4, 39, 3, OLED_FONT_EIGHT, DISABLE, DISABLE);
    for(count = 0; count < 61;count++)
    {
        SoftwareDelay_ms(230);
        OLED_P8x8(29 + 2 + count, 6, 2, DISABLE, DISABLE);
		OLED_ShowNum(79, 4, 40 + count, 3, OLED_FONT_EIGHT, DISABLE, DISABLE);
    }	
}


/*---------------------------- END OF FILE ----------------------------*/


