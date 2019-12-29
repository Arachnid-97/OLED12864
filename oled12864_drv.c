#include "oled12864_drv.h"
#include "oled_font.h"
//#define NDEBUG              //取消断言
#include <assert.h>         // 断言库

//#define _FONT_LIBRARY		// 是否使用外带字库

#ifdef _FONT_LIBRARY
	#include "gb2312_font.h"

#endif /* _FONT_LIBRARY */

/*
    硬件接口说明:
    ----------------------------------------------------------------
    GND  电源地
    VCC  接5V或3.3v电源
    D0   SPI -> PC1 / IIC -> PC1
    D1   SPI -> PC0 / IIC -> PC0
    RES  PB7
    DC   PB6
    CS   PB5
    ----------------------------------------------------------------
*/

//#define _OLED_IIC_MODE      // 默认 spi接口，如需 iic接口则去掉注释

/*
    OLED的显存
    存放格式如下：
    [0]0 1 2 3 ... 127
    [1]0 1 2 3 ... 127
    [2]0 1 2 3 ... 127
    [3]0 1 2 3 ... 127
    [4]0 1 2 3 ... 127
    [5]0 1 2 3 ... 127
    [6]0 1 2 3 ... 127
    [7]0 1 2 3 ... 127
*/

uint8_t g_OLED_Gram[OLED_GRAM_MAX][16] = {0};    // oled滚动显存（最大显示OLED_GRAM_MAX个字符 / （OLED_GRAM_MAX / 2）个汉字）
uint8_t g_OLED_Roll_Page = 0;                    // 第 n行滚动（对应bit置位）

/************************************************
函数名称 ： OLED_Delay_us
功    能 ： 软件毫秒延时
参    数 ： Count ---- 次数
返 回 值 ： 无
*************************************************/
static void OLED_Delay_us( uint32_t Count )
{
    while(Count)
    {
        Count--;
    }
}

/************************************************
函数名称 ： OLED_Delay_ms
功    能 ： 软件毫秒延时
参    数 ： Count ---- 次数
返 回 值 ： 无
*************************************************/
static void OLED_Delay_ms( uint32_t Count )
{
    uint16_t i;

    while(Count--)
    {
        for(i = 1240;i > 0;i--);        // 根据震荡时间为 i取值
    }
}

/************************************************
函数名称 ： IIC_Start
功    能 ： IIC写启动
参    数 ： 无
返 回 值 ： 0 / 1
*************************************************/

#ifdef _OLED_IIC_MODE
static _Bool IIC_Start(void)
{
	OLED_SCL(HIGH);
	OLED_SDA(HIGH);
    OLED_Delay_us(1);

    if(!OLED_SDA_READ)              // 回检一次电平状态
    {
        return 0;
    }
	OLED_SDA(LOW);
    OLED_Delay_us(1);

    if(OLED_SDA_READ)               // 回检一次电平状态
    {
        return 0;
    }
	OLED_SCL(LOW);

    return 1;
}

#endif /* _OLED_IIC_MODE */

/************************************************
函数名称 ： IIC_Stop
功    能 ： IIC写停止
参    数 ： 无
返 回 值 ： 无
*************************************************/

#ifdef _OLED_IIC_MODE
static void IIC_Stop(void)
{
	OLED_SCL(LOW);
	OLED_SDA(LOW);
    OLED_Delay_us(1);

    OLED_SCL(HIGH);
    OLED_Delay_us(1);

	OLED_SDA(HIGH);
}

#endif /* _OLED_IIC_MODE */

/************************************************
函数名称 ： IIC_Wait_Ack
功    能 ： ACK等待
参    数 ： 无
返 回 值 ： 0 / 1
*************************************************/

#ifdef _OLED_IIC_MODE
static _Bool IIC_Wait_Ack(void)
{
    uint8_t time = 80;

    OLED_SCL(LOW);
    OLED_Delay_us(1);

    OLED_SDA(HIGH);
    OLED_Delay_us(1);

	OLED_SCL(HIGH);
    OLED_Delay_us(1);

    while(OLED_SDA_READ)
    {
        time--;
        if(!time)
        {
            IIC_Stop();
            return 0;
        }
    }
	OLED_SCL(LOW);

    return 1;
}

#endif /* _OLED_IIC_MODE */

/************************************************
函数名称 ： Write_IIC_Byte
功    能 ： IIC写一个字节
参    数 ： Byte ---- 数据
返 回 值 ： 无
*************************************************/

#ifdef _OLED_IIC_MODE
static void Write_IIC_Byte( uint8_t Byte )
{
	uint8_t i;

	OLED_SCL(LOW);
    OLED_Delay_us(1);

	for(i = 0;i < 8;i++)
	{
        OLED_SDA((BitAction)((Byte & 0x80) >> 7));
        Byte <<= 1;
        OLED_Delay_us(1);

		OLED_SCL(HIGH);
        OLED_Delay_us(1);

		OLED_SCL(LOW);
        OLED_Delay_us(1);

    }
}

#endif /* _OLED_IIC_MODE */

/************************************************
函数名称 ： OLED_Write_Cmd
功    能 ： OLED写命令
参    数 ： Cmd ---- 命令
返 回 值 ： 无
*************************************************/
void OLED_Write_Cmd( uint8_t Cmd )
{

#ifdef _OLED_IIC_MODE
	if(!IIC_Start())
    {
        return ;
    }
	Write_IIC_Byte(0x78);            // Slave address,SA0 = 0
    IIC_Wait_Ack();
	Write_IIC_Byte(0x00);			 // write command
    IIC_Wait_Ack();
	Write_IIC_Byte(Cmd);
    IIC_Wait_Ack();
	IIC_Stop();

#else
    uint8_t i;

    OLED_CS(LOW);
    OLED_DC(LOW);

    for(i = 0;i < 8;i++)
    {
        OLED_SCK(LOW);
        OLED_Delay_us(1);            // 空等待
//        OLED_SDO((Cmd & 0x80) >> 7);
        if(Cmd & 0x80)
        {
            OLED_SDO(HIGH);
        }
        else
        {
            OLED_SDO(LOW);
        }
        OLED_SCK(HIGH);
		Cmd <<= 1;

    }
    OLED_CS(HIGH);

#endif /* _OLED_IIC_MODE */
}

/************************************************
函数名称 ： OLED_Write_Data
功    能 ： OLED写数据
参    数 ： Data ---- 数据
           Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_Write_Data( uint8_t Data, uint8_t Inverse )
{

#ifdef _OLED_IIC_MODE
    uint8_t temp = 0;

    if(!Inverse)
    {
        temp = Data;
    }
    else
    {
        temp = ~Data;
    }

	if(!IIC_Start())
    {
        return ;
    }
	Write_IIC_Byte(0x78);           // Slave address,SA0 = 0
    IIC_Wait_Ack();
	Write_IIC_Byte(0x40);			// write data
    IIC_Wait_Ack();
	Write_IIC_Byte(temp);
    IIC_Wait_Ack();
	IIC_Stop();

#else
    uint8_t i;

    OLED_CS(LOW);
    OLED_DC(HIGH);

    if(!Inverse)
    {
        for(i = 0;i < 8;i++)
        {
            OLED_SCK(LOW);
            OLED_Delay_us(1);       // 空等待
//            OLED_SDO((Data & 0x80) >> 7);
            if(Data & 0x80)
            {
                OLED_SDO(HIGH);
            }
            else
            {
                OLED_SDO(LOW);
            }
            OLED_SCK(HIGH);
            Data <<= 1;
        }
    }
    else
    {
        for(i = 0;i < 8;i++)
        {
            OLED_SCK(LOW);
            OLED_Delay_us(1);       // 空等待
//            OLED_SDO(~((Data & 0x80) >> 7));
            if(Data & 0x80)
            {
                OLED_SDO(LOW);
            }
            else
            {
                OLED_SDO(HIGH);
            }
            OLED_SCK(HIGH);
            Data <<= 1;
        }
    }
    OLED_CS(HIGH);

#endif /* _OLED_IIC_MODE */
}

/************************************************
函数名称 ： OLED_Fill
功    能 ： OLED亮屏 / 清屏
参    数 ： Mode ---- 清除/显示(OLED_CLS or OLED_SHOW)
返 回 值 ： 无
*************************************************/
void OLED_Fill( uint8_t Mode )
{
    uint8_t y,x;

    for(y = 0;y < 8;y++)
    {
        OLED_Write_Cmd(0xB0 + y);               // 第 n页开始（0~7）
        OLED_Write_Cmd(0x10);                   // 设置显示位置 - 列高位第一个
        OLED_Write_Cmd(0x00);                   // 设置显示位置 - 列低位第一个
        for(x = 0;x < OLED_MAX_COLUMN;x++)
        {
            OLED_Write_Data(Mode, DISABLE); // Data = 0x00全屏灭，Data = 0xff全屏亮
        }
    }
}

/************************************************
函数名称 ： OLED_Row_Clear
功    能 ： OLED单行清除 / 显示
参    数 ： Row ---- 首行
            Amount ---- 行数
			Mode ---- 清除/显示(OLED_CLS or OLED_SHOW)
返 回 值 ： 无
*************************************************/
void OLED_Row_Clear( uint8_t Row, uint8_t Amount ,uint8_t Mode )
{
    uint8_t y,x;

    if(Row < 8)
    {
        for(y = 0;y < Amount;y++)
        {
            OLED_Write_Cmd(0xB0 + Row + y);         // 第 n页开始（0~7）
            OLED_Write_Cmd(0x10);                   // 设置显示位置 - 列高位第一个
            OLED_Write_Cmd(0x00);                   // 设置显示位置 - 列低位第一个
            for(x = 0;x < OLED_MAX_COLUMN;x++)
            {
                OLED_Write_Data(Mode, DISABLE);
            }
        }
    }
}

/************************************************
函数名称 ： OLED_Coord
功    能 ： OLED坐标显示
参    数 ： X ---- X轴
			Y ---- Y轴
返 回 值 ： 无
*************************************************/
void OLED_Coord( uint8_t X, uint8_t Y )
{
//    assert(X < X_WIDTH);
//    assert(Y < (Y_WIDTH >> 3));

    /* B0~B7:此命令仅适用于页面寻址模式 */
    OLED_Write_Cmd(0xB0 + Y);                   // 设置GDDRAM页面开始地址,(Page 0~ Page 7)为页面寻址模式

    /* 10~1F:此命令仅适用于页面寻址模式 */
    OLED_Write_Cmd(((X & 0xF0) >> 4) | 0x10);   // 页面寻址模式下设置列开始地址寄存器的高位

    /* 00~0F:此命令仅适用于页面寻址模式 */
    OLED_Write_Cmd((X & 0x0F) | 0x00);          // 页面寻址模式下设置列开始地址寄存器的低位
}

/************************************************
函数名称 ： OLED_ShowRoll
功    能 ： OLED内置滚动显示
参    数 ： Y ---- 起始行
            Line ---- 滚动行数 (0 ---> 取消滚动)
            Mode ---- 滚动模式（OLED_LEFT_ROLL or OLED_RIGHT_ROLL）
返 回 值 ： 无
*************************************************/
void OLED_ShowRoll( uint8_t Y, uint8_t Line, uint8_t Mode )
{
    if(Line > 0)
    {
        if(Mode == OLED_LEFT_ROLL)
        {
            OLED_Write_Cmd(OLED_LEFT_ROLL);     // 左滚动显示
        }
        else if(Mode == OLED_RIGHT_ROLL)
        {
            OLED_Write_Cmd(OLED_RIGHT_ROLL);    // 右滚动显示
        }
        else
        {
            return ;
        }
        OLED_Write_Cmd(0x00);                   // 虚拟字节设置，默认为0x00
        OLED_Write_Cmd(Y);                      // 定义开始页面地址
        OLED_Write_Cmd(0x07);                   // 帧频设置
        OLED_Write_Cmd(Y + (Line - 1));  		// 定义结束页面地址
        OLED_Write_Cmd(0x00);                   // 虚拟字节设置，默认为0x00
        OLED_Write_Cmd(0xFF);                   // 虚拟字节设置，默认为0xFF
        OLED_Write_Cmd(0x2F);                   // 激活滚动
    }
    else
    {
        OLED_Write_Cmd(0x2E);                   // 失能滚动
    }
}

/************************************************
函数名称 ： OLED_ShowChar
功    能 ： OLED字符显示
参    数 ： X ---- X轴
			Y ---- Y轴
            Char ---- 字符
            Size ---- 字体大小
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_ShowChar( uint8_t X, uint8_t Y, uint8_t Char, uint8_t Size, uint8_t Inverse )
{
    uint8_t i;
	uint8_t value = 0;

    if(X <= OLED_MAX_COLUMN - 8 && (Y < (OLED_MAX_ROW >> 3)))
    {
		if(X >= 0 && Y >= 0)
		{
			value = Char - 32;                                          // 得到偏移值(偏移量32)

			if(Size == OLED_FONT_SIXTEEN)                               // 8x16
			{
				OLED_Coord(X, Y);
				for(i = 0;i < 8;i++)
				{
					OLED_Write_Data(F8x16[value][i], Inverse);      // 前八个数据
				}

				OLED_Coord(X, Y+1);
				for(i = 0;i < 8;i++)
				{
					OLED_Write_Data(F8x16[value][i + 8], Inverse);  // 后八个数据
				}
			}
			else if(Size == OLED_FONT_EIGHT)                           // 6x8
			{
				OLED_Coord(X, Y);
				for(i = 0;i < 6;i++)
				{
					OLED_Write_Data(F6x8[value][i], Inverse);
				}
				/* 清除剩下的两个 */
				OLED_Write_Data(0x00, Inverse);
				OLED_Write_Data(0x00, Inverse);
			}
		}
    }
}

/************************************************
函数名称 ： OLED_ShowString
功    能 ： OLED字符串显示（超区域自动滚动）
参    数 ： X ---- X轴
			Y ---- Y轴
            Len ---- 长度
            pChar ---- 数据
            Size ---- 字体大小
            Inverse ---- 反白（全行）显示使能
返 回 值 ： 无
*************************************************/
void OLED_ShowString( uint8_t X, uint8_t Y, const uint8_t *pChar, uint16_t Len, uint8_t Size, uint8_t Inverse )
{
    uint8_t i,j;
    uint8_t temp = 0;

    if(Inverse)
    {
        for(j = 0;j < (Size / 8);j++)
        {
            OLED_Coord(0, Y + j);
            for(i = 0;i < OLED_MAX_COLUMN;i++)
            {
                OLED_Write_Data(OLED_SHOW, DISABLE);    // OLED_CLS = 0x00全屏灭，OLED_SHOW = 0xff全屏亮
            }
        }
    }

    if(Len <= 16)
    {
        while(Len--)
        {
            if(X >= OLED_MAX_COLUMN - 8)
            {
                break;
            }
            OLED_ShowChar(X, Y, *pChar, Size, Inverse);
            X += 8;
            pChar++;
        }
    }
    else if(Len <= OLED_GRAM_MAX)
    {
        g_OLED_Roll_Page |= (1 << Y);

        for(j = 0;j < Len;j++)
        {
            temp = *(pChar + j) - 32;                                   // 得到偏移值(偏移量32)

            if(Size == OLED_FONT_SIXTEEN)                               // 8x16
            {
                for(i = 0;i < 16;i++)
                {
                    g_OLED_Gram[j][i] = F8x16[temp][i];                   // 存入显存区
                }
            }
//            else if(Size == OLED_FONT_EIGHT)                           // 6x8
//            {
//                OLED_Coord(X, Y);
//                for(i = 0;i < 6;i++)
//                {
//                    OLED_Gram[j][i] = F6x8[temp][i];
//                }
//            }
        }
    }
}

/************************************************
函数名称 ： OLED_ShowPrintf
功    能 ： OLED字符串输出（自动顶部对齐换行）
参    数 ： X ---- X轴
			Y ---- Y轴
            pChar ---- 数据
            Size ---- 字体大小
            Align ---- 对齐显示使能
            Inverse ---- 反白（单字体）显示使能
返 回 值 ： 无
*************************************************/
void OLED_ShowPrintf( uint8_t X, uint8_t Y, const uint8_t *pChar, uint8_t Size, _Bool Align, uint8_t Inverse )
{
    uint8_t addr = 0;

    if(Align)
    {
        addr = X;		// 记录顶部对齐位置 
    }

	while(*pChar != '\0')
	{
		if(X >= OLED_MAX_COLUMN - 8)             // 列溢出
        {
            X = addr;                           // 对齐列地址
            Y += (Size >> 3);                   // 转到下一行
            if(Y >= (OLED_MAX_ROW >> 3))
            {
                break;                         // Y轴越界退出
            }
        }
        OLED_ShowChar(X, Y, *pChar, Size, Inverse);
		X += 8;
        pChar++;
	}
}

/************************************************
函数名称 ： OLED_Power
功    能 ： OLED幂计算
参    数 ： M ---- X轴
			N ---- Y轴
返 回 值 ： power ---- 幂
*************************************************/
uint32_t OLED_Power( uint8_t M, uint8_t N )
{
	uint32_t power = 1;

	while(N--)
    {
        power *= M;
    }

	return power;
}

/************************************************
函数名称 ： OLED_ShowNum
功    能 ： OLED数字显示（限正整数）
参    数 ： X ---- X轴
			Y ---- Y轴
            Num ---- 数字
            Len ---- 位数
            Size ---- 字体大小
			Prefix ---- 补零显示使能
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_ShowNum( uint8_t X, uint8_t Y, uint32_t Num, uint8_t Len, uint8_t Size, uint8_t Prefix, uint8_t Inverse )
{
	char temp = 0;
	uint8_t t;
	uint8_t show = 0;

	for(t = 0;t < Len;t++)
	{
		temp = (Num / OLED_Power(10, Len - t - 1)) % 10;          // 提取每位数字

		if(!show && t < (Len - 1))
		{
			if(0 == temp)
			{
				if(Prefix)
					OLED_ShowChar(X + 8 *t, Y , '0', Size, Inverse);
				else
					OLED_ShowChar(X + 8 *t, Y , ' ', Size, Inverse);
				continue;
			}
            else
            {
                show = 1;
            }
		}
	 	OLED_ShowChar(X + 8 *t, Y, temp + '0', Size, Inverse);
	}
}

/************************************************
函数名称 ： OLED_ShowHex
功    能 ： OLED十六进制显示
参    数 ： X ---- X轴
			Y ---- Y轴
            Num ---- 数字（10进制）
            Size ---- 字体大小（固定宽度为 8像素）
            Prefix ---- 补零显示使能
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_ShowHex( uint8_t X, uint8_t Y, uint32_t Num, uint8_t Size, uint8_t Prefix, uint8_t Inverse )
{
	uint8_t t,temp;
    uint8_t i = 0;
	uint8_t show = 0;

	for(t = 0;t < 4;t++)
	{
		temp = (uint8_t)(Num >> (12 - 4*t)) & 0x000F;          // 提取每位数字

		if(!show && t < 3)
		{
			if(0 == temp)
			{
                if(Prefix)
                {
                    OLED_ShowChar(X + 8 *i, Y , '0', Size, Inverse);
                    i++;
                }
				continue;
			}
            else
            {
                show = 1;
            }
		}
		switch(temp)
		{
			case  0: temp = '0'; break;
			case  1: temp = '1'; break;
			case  2: temp = '2'; break;
			case  3: temp = '3'; break;
			case  4: temp = '4'; break;
			case  5: temp = '5'; break;
			case  6: temp = '6'; break;
			case  7: temp = '7'; break;
			case  8: temp = '8'; break;
			case  9: temp = '9'; break;
			case 10: temp = 'A'; break;
			case 11: temp = 'B'; break;
			case 12: temp = 'C'; break;
			case 13: temp = 'D'; break;
			case 14: temp = 'E'; break;
			case 15: temp = 'F'; break;
		}

	 	OLED_ShowChar(X + 8 *i, Y, temp, Size, Inverse);
        i++;
	}
}

/************************************************
函数名称 ： OLED_ShowFloat
功    能 ： OLED可变精度小数显示
参    数 ： X ---- X轴
			Y ---- Y轴
            Num ---- 数字
            Accuracy ---- 精确位数
            Size ---- 字体大小
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/
void OLED_ShowFloat( uint8_t X, uint8_t Y, float Num, uint8_t Accuracy, uint8_t Size, uint8_t Inverse )
{
	uint8_t i = 0;
    uint8_t j = 0;
	uint8_t t = 0;
    uint8_t temp = 0;
    uint16_t numel = 0;
    uint32_t integer = 0;
    float decimals = 0;


    /* 判断是否为负数 */
    if(Num < 0)
    {
        OLED_ShowChar(X, Y , '-', Size, Inverse);
        Num = 0 - Num;
        i++;
    }

    integer = (uint32_t)Num;
    decimals = Num - integer;

    /* 整数部分 */
    if(integer)
    {
        numel = integer;

		while(numel)                                                        // 获取整数位数
		{
			numel /= 10;
            j++;
		}
        i += (j - 1);
        for(temp = 0;temp < j;temp++)
        {
            OLED_ShowChar(X + 8 *(i - temp), Y, integer % 10 + '0', Size, Inverse);  // 显示整数部分
            integer /= 10;
        }
	}
    else
    {
		OLED_ShowChar(X + 8 *i, Y, temp + '0', Size, Inverse);
	}
    i++;

    /* 小数部分 */
    if(Accuracy)
    {
        OLED_ShowChar(X + 8 *i, Y , '.', Size, Inverse);                    // 显示小数点

        i++;
        for(t = 0;t < Accuracy;t++)
        {
            decimals *= 10;
            temp = (uint8_t)decimals;                                       // 提取每位小数
            OLED_ShowChar(X + 8 *(i + t), Y, temp + '0', Size, Inverse);
            decimals -= temp;
        }
    }

}

/************************************************
函数名称 ： OLED_ShowLanguage
功    能 ： OLED汉字显示(16x16) 不带字库的 
参    数 ： X ---- X轴
			Y ---- Y轴
            pArray ---- 数据
            Len ---- 字数长度
            Inverse ---- 反白显示使能
返 回 值 ： 无
*************************************************/

/*
    mode 0:使用 1级指针来访问二维数组
    mode 1:使用指向数组的指针来访问二维数组
*/
#define _OLED_ShowLanguage_MODE     0       // (0 or 1)

#if (0 == _OLED_ShowLanguage_MODE)
void OLED_ShowLanguage( uint8_t X, uint8_t Y, const uint8_t *pArray, uint16_t Len, uint8_t Inverse )

#elif (1 == _OLED_ShowLanguage_MODE)
void OLED_ShowLanguage( uint8_t X, uint8_t Y, const uint8_t (*pArray)[16], uint16_t Len, uint8_t Inverse )

#endif /* _OLED_ShowLanguage_MODE */
{
    uint8_t i,j;
    uint8_t q = 0;
    uint8_t temp = 0;
	uint8_t distribute_flag = 0;									// 文字平均分布标志 

    if(Len <= 8)
    {
        if(!X)		// 首位显示，自动平均分布 
        {
            switch(Len)
            {
                case 1:
                        temp = 56;          // OLED_MAX_COLUMN / 2 - 8 = 56
                        break;
                case 2:
                        temp = 32;          // (OLED_MAX_COLUMN - Len*16) / (Len + 1) = 32  Len = 2
                        break;
                case 3:
                        temp = 20;          // (OLED_MAX_COLUMN / 2 - 8) / 2 - 8 = 20
                        break;
                case 4:
                        temp = 13;          // (OLED_MAX_COLUMN - Len*16) / (Len + 1) = 12 .... 4   Len = 4
                        break;
                case 5:
                        temp = 8;           // ((OLED_MAX_COLUMN / 2 - 8) - ((Len - 1) / 2)*16) / ((Len - 1) / 2 + 1) = 8   Len = 5
                        break;
                case 6:
                        temp = 5;           // (OLED_MAX_COLUMN - Len*16) / (Len + 1) = 4 .... 4    Len = 6
                        break;
                case 7:
                        temp = 2;           // ((OLED_MAX_COLUMN / 2 - 8) / 2 - 8) / 2 - 8 = 2
                        break;
                case 8:
                        temp = 0;
                        break;
                default:
                        break;
            }
            X += temp;
            distribute_flag = 1; 
        }

        for(j = 0;j < Len;j++)
        {
            OLED_Coord(X, Y);
            for(i = 0;i < 16;i++)
            {

#if (0 == _OLED_ShowLanguage_MODE)
                OLED_Write_Data(*(pArray + 2*j*16 + i), Inverse);      // 前十六个数据

#elif (1 == _OLED_ShowLanguage_MODE)
                OLED_Write_Data(pArray[2*j][i], Inverse);       // 前十六个数据

#endif /* _OLED_ShowLanguage_MODE */
            }

            OLED_Coord(X, Y+1);
            for(i = 0;i < 16;i++)
            {

#if (0 == _OLED_ShowLanguage_MODE)
                OLED_Write_Data(*(pArray + 2*j*16 + 16 + i), Inverse);           // 后十六个数据

#elif (1 == _OLED_ShowLanguage_MODE)
                OLED_Write_Data(pArray[2*j + 1][i], Inverse);   // 后十六个数据

#endif /* _OLED_ShowLanguage_MODE */
            }
            X += (16 + temp);

            if(distribute_flag)
            {
            	q++;

	            /* 特殊处理 */
	            if((4 == Len && 2 == q) || (6 == Len && 0 == (q + 1) % 2))
	            {
	                X -= 1;
	            }
	        }
		}
    }
    else if(Len < (OLED_GRAM_MAX >> 1))
    {
        g_OLED_Roll_Page |= (1 << Y);

        for(j = 0;j < Len;j++)
        {
            for(i = 0;i < 16;i++)
            {

#if (0 == _OLED_ShowLanguage_MODE)
                g_OLED_Gram[j][i] = *(pArray + j*16 + i);                  // 存入显存区

#elif (1 == _OLED_ShowLanguage_MODE)
                g_OLED_Gram[j][i] = pArray[j][i];                 // 存入显存区

#endif /* _OLED_ShowLanguage_MODE */
            }
        }

    }
}

/************************************************
函数名称 ： OLED_Display_On
功    能 ： 开启OLED显示
参    数 ： 无
返 回 值 ： 无
*************************************************/
void OLED_Display_On(void)
{
	OLED_Write_Cmd(0x8D);       //SET DCDC命令
	OLED_Write_Cmd(0x14);       //DCDC ON
	OLED_Write_Cmd(0xAF);       //DISPLAY ON
}

/************************************************
函数名称 ： OLED_Display_Off
功    能 ： 关闭OLED显示
参    数 ： 无
返 回 值 ： 无
*************************************************/
void OLED_Display_Off(void)
{
	OLED_Write_Cmd(0x8D);       // SET DCDC命令
	OLED_Write_Cmd(0x10);       // DCDC OFF
	OLED_Write_Cmd(0xAE);       // DISPLAY OFF
}

/************************************************
函数名称 ： OLED_Config
功    能 ： OLED配置
参    数 ： 无
返 回 值 ： 无
*************************************************/
static void OLED_Config(void)
{
    /* OLED_DC */
    GPIO_Init(OLED_DC_PORT, OLED_DC_PINS, GPIO_Mode_Out_PP_Low_Slow);

#ifdef _OLED_IIC_MODE
    /* OLED_SCL */
    GPIO_Init(OLED_SCL_PORT, OLED_SCL_PINS, GPIO_Mode_Out_OD_HiZ_Fast);

    /* OLED_SDA */
    GPIO_Init(OLED_SDA_PORT, OLED_SDA_PINS, GPIO_Mode_Out_OD_HiZ_Fast);

#else
    /* OLED_CS */
    GPIO_Init(OLED_CS_PORT, OLED_CS_PINS, GPIO_Mode_Out_PP_High_Slow);

    /* OLED_SCK */
    GPIO_Init(OLED_SCK_PORT, OLED_SCK_PINS, GPIO_Mode_Out_PP_Low_Fast);

    /* OLED_RST */
    GPIO_Init(OLED_RST_PORT, OLED_RST_PINS, GPIO_Mode_Out_PP_High_Slow);

    /* OLED_SDO */
    GPIO_Init(OLED_SDO_PORT, OLED_SDO_PINS, GPIO_Mode_Out_PP_Low_Fast);

#endif /* _OLED_IIC_MODE */

}

/************************************************
函数名称 ： OLED_Init
功    能 ： OLED初始化
参    数 ： 无
返 回 值 ： 无
*************************************************/
void OLED_Init(void)
{
    OLED_Config();

#ifndef _OLED_IIC_MODE
	OLED_SCK(HIGH);          // 空闲态时，SCLK处于高电平
	OLED_CS(HIGH);	         // 关闭选择输入

    /* 从上电到下面开始初始化要有足够的时间，即等待 RC复位完毕 */
	OLED_RST(LOW);           // 复位（低电平有效）
    OLED_Delay_ms(100);
	OLED_RST(HIGH);
    OLED_Delay_ms(30);

#endif /* _OLED_IIC_MODE */

    OLED_Write_Cmd(0xAE);    // 关闭OLED -- turn off oled panel
    OLED_Write_Cmd(0xD5);    // 设置显示时钟分频因子/振荡器频率 -- set display clock divide ratio/oscillator frequency
    OLED_Write_Cmd(0x80);    // \ set divide ratio, Set Clock as 100 Frames/Sec
    OLED_Write_Cmd(0x20);    // 设置内存寻址模式 -- Set Memory Addressing Mode (0x00 / 0x01 / 0x02)
    OLED_Write_Cmd(0x02);    // \ Page Addressing
    OLED_Write_Cmd(0xA8);    // 设置多路传输比率 -- set multiplex ratio (16 to 63)
    OLED_Write_Cmd(0x3F);    // \ 1 / 64 duty
    OLED_Write_Cmd(0xDA);    // 设置列引脚硬件配置 -- set com pins hardware configuration
    OLED_Write_Cmd(0x12);    // \ Sequential COM pin configuration，Enable COM Left/Right remap

    /* ----- 方向显示配置 ----- */
    OLED_Write_Cmd(0xA1);    // 设置段重映射 -- Set SEG / Column Mapping     0xA0左右反置（复位值） 0xA1正常（重映射值）
    OLED_Write_Cmd(0xC8);    // 设置列输出扫描方向 -- Set COM / Row Scan Direction   0xc0上下反置（复位值） 0xC8正常（重映射值）
    /* ----- END ----- */

    OLED_Write_Cmd(0x40);    // 设置设置屏幕（GDDRAM）起始行 -- Set Display Start Line (0x40~0x7F)
    OLED_Write_Cmd(0xD3);    // 设置显示偏移 -- set display offset (0x00~0x3F)
    OLED_Write_Cmd(0x00);    // \ not offset
    OLED_Write_Cmd(0x81);    // 设置对比度 -- set contrast control register (0x00~0x100)
    OLED_Write_Cmd(0xCF);    // \ Set SEG Output Current Brightness
    OLED_Write_Cmd(0xD9);    // 设置预充电期间的持续时间 -- set pre-charge period
    OLED_Write_Cmd(0xF1);    // \ Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    OLED_Write_Cmd(0xDB);    // 调整VCOMH调节器的输出 -- set vcomh (0x00 / 0x20 / 0x30)
    OLED_Write_Cmd(0x20);    // \ Set VCOM Deselect Level
    OLED_Write_Cmd(0x8D);    // 电荷泵设置 -- set Charge Pump enable / disable (0x14 / 0x10)
    OLED_Write_Cmd(0x14);    // \ Enable charge pump during display on
    OLED_Write_Cmd(0xA4);    //  全局显示开启(黑屏/亮屏) -- Entire Display On (0xA4 / 0xA5)
    OLED_Write_Cmd(0xA6);    // 设置显示方式(正常/反显) -- set normal display (0xA6 / 0xA7)

    OLED_Write_Cmd(0xAF);    // 打开OLED -- turn on oled panel
    OLED_Fill(0x00);         // 初始清屏
    OLED_Coord(0,0);         // 设置原点坐标（0, 0）
}

/************************* 	需要提供字库相关文件 *************************/

#ifdef _FONT_LIBRARY

/************************************************
函数名称 ： OLED_ShowChinese
功    能 ： OLED单汉字显示 
参    数 ： X ---- X轴
			Y ---- Y轴
            pArray ---- 数据
            Inverse ---- 反白显示使能
返 回 值 ： 无 
*************************************************/
void OLED_ShowChinese( uint8_t X, uint8_t Y, const uint8_t *pArray, uint8_t Inverse )
{
	uint8_t i;
	uint8_t buffer[GB2312_1616_BYTE_SIZE] = {0};

    if(X <= OLED_MAX_COLUMN - 16 && (Y < (OLED_MAX_ROW >> 3)))
	{
		Get_GB2312_Code(buffer, pArray);
		
		OLED_Coord(X, Y);
		for(i = 0;i < 16;i++)
		{
			OLED_Write_Data(buffer[i], Inverse);      // 前十六个数据
		}
		OLED_Coord(X, Y+1);
		for(i = 0;i < 16;i++)
		{
			OLED_Write_Data(buffer[i + 16], Inverse); // 后十六个数据
		}
	}
}

/************************************************
函数名称 ： OLED_Draw_Font
功    能 ： OLED中英输出显示（支持混合显示）
参    数 ： X ---- X轴
			Y ---- Y轴
            pArray ---- 数据
            Inverse ---- 反白显示使能
返 回 值 ： 无 
*************************************************/
void OLED_Draw_Font( uint8_t X, uint8_t Y, const uint8_t *pArray, uint8_t Inverse )
{
	uint8_t i = 0;
	uint16_t j = 0;
	uint8_t buffer[GB2312_1616_BYTE_SIZE] = {0};
	
	while(*pArray != '\0')
	{
		if(*pArray < 127)			// ASCII码
		{
			if(X + 8 > OLED_MAX_COLUMN)
			{
				X = 0;
				Y += 2;			// 换行
			}
			if(Y > (OLED_MAX_ROW >> 3))
			{
				X = 0;
				Y = 0;			// 跳到原点
			}
			
			OLED_ShowChar(X, Y, *pArray, OLED_FONT_SIXTEEN, Inverse);
			X += 8;
			pArray++;			// 一个 ASCII码1个字节
		}
		else						// GB2312码
		{
			if(X + 16 > OLED_MAX_COLUMN)
			{
				X = 0;
				Y += 2;			// 换行
			}
			if(Y > (OLED_MAX_ROW >> 3))
			{
				X = 0;
				Y = 0;			// 跳到原点
			}

//			Get_GB2312_Code(buffer, pArray);
//			
//			OLED_Coord(X, Y);
//			for(i = 0;i < 16;i++)
//			{
//				OLED_Write_Data(*(buffer + 2*j*16 + i), Inverse);      // 前十六个数据
//			}
//			OLED_Coord(X, Y+1);
//			for(i = 0;i < 16;i++)
//			{
//				OLED_Write_Data(*(buffer + 2*j*16 + 16 + i), Inverse); // 后十六个数据
//			}
//			j++;
			
			OLED_ShowChinese(X, Y, pArray, Inverse);
			X += 16;
			pArray += 2;		// 一个 GB2312文字两个字节
		}
	}
}

#endif /* _FONT_LIBRARY */


/************************* 	需要提供字库相关文件 *************************/


/*---------------------------- END OF FILE ----------------------------*/


