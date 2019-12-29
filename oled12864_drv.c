#include "oled12864_drv.h"
#include "oled_font.h"
//#define NDEBUG              //ȡ������
#include <assert.h>         // ���Կ�

//#define _FONT_LIBRARY		// �Ƿ�ʹ������ֿ�

#ifdef _FONT_LIBRARY
	#include "gb2312_font.h"

#endif /* _FONT_LIBRARY */

/*
    Ӳ���ӿ�˵��:
    ----------------------------------------------------------------
    GND  ��Դ��
    VCC  ��5V��3.3v��Դ
    D0   SPI -> PC1 / IIC -> PC1
    D1   SPI -> PC0 / IIC -> PC0
    RES  PB7
    DC   PB6
    CS   PB5
    ----------------------------------------------------------------
*/

//#define _OLED_IIC_MODE      // Ĭ�� spi�ӿڣ����� iic�ӿ���ȥ��ע��

/*
    OLED���Դ�
    ��Ÿ�ʽ���£�
    [0]0 1 2 3 ... 127
    [1]0 1 2 3 ... 127
    [2]0 1 2 3 ... 127
    [3]0 1 2 3 ... 127
    [4]0 1 2 3 ... 127
    [5]0 1 2 3 ... 127
    [6]0 1 2 3 ... 127
    [7]0 1 2 3 ... 127
*/

uint8_t g_OLED_Gram[OLED_GRAM_MAX][16] = {0};    // oled�����Դ棨�����ʾOLED_GRAM_MAX���ַ� / ��OLED_GRAM_MAX / 2�������֣�
uint8_t g_OLED_Roll_Page = 0;                    // �� n�й�������Ӧbit��λ��

/************************************************
�������� �� OLED_Delay_us
��    �� �� ���������ʱ
��    �� �� Count ---- ����
�� �� ֵ �� ��
*************************************************/
static void OLED_Delay_us( uint32_t Count )
{
    while(Count)
    {
        Count--;
    }
}

/************************************************
�������� �� OLED_Delay_ms
��    �� �� ���������ʱ
��    �� �� Count ---- ����
�� �� ֵ �� ��
*************************************************/
static void OLED_Delay_ms( uint32_t Count )
{
    uint16_t i;

    while(Count--)
    {
        for(i = 1240;i > 0;i--);        // ������ʱ��Ϊ iȡֵ
    }
}

/************************************************
�������� �� IIC_Start
��    �� �� IICд����
��    �� �� ��
�� �� ֵ �� 0 / 1
*************************************************/

#ifdef _OLED_IIC_MODE
static _Bool IIC_Start(void)
{
	OLED_SCL(HIGH);
	OLED_SDA(HIGH);
    OLED_Delay_us(1);

    if(!OLED_SDA_READ)              // �ؼ�һ�ε�ƽ״̬
    {
        return 0;
    }
	OLED_SDA(LOW);
    OLED_Delay_us(1);

    if(OLED_SDA_READ)               // �ؼ�һ�ε�ƽ״̬
    {
        return 0;
    }
	OLED_SCL(LOW);

    return 1;
}

#endif /* _OLED_IIC_MODE */

/************************************************
�������� �� IIC_Stop
��    �� �� IICдֹͣ
��    �� �� ��
�� �� ֵ �� ��
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
�������� �� IIC_Wait_Ack
��    �� �� ACK�ȴ�
��    �� �� ��
�� �� ֵ �� 0 / 1
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
�������� �� Write_IIC_Byte
��    �� �� IICдһ���ֽ�
��    �� �� Byte ---- ����
�� �� ֵ �� ��
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
�������� �� OLED_Write_Cmd
��    �� �� OLEDд����
��    �� �� Cmd ---- ����
�� �� ֵ �� ��
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
        OLED_Delay_us(1);            // �յȴ�
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
�������� �� OLED_Write_Data
��    �� �� OLEDд����
��    �� �� Data ---- ����
           Inverse ---- ������ʾʹ��
�� �� ֵ �� ��
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
            OLED_Delay_us(1);       // �յȴ�
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
            OLED_Delay_us(1);       // �յȴ�
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
�������� �� OLED_Fill
��    �� �� OLED���� / ����
��    �� �� Mode ---- ���/��ʾ(OLED_CLS or OLED_SHOW)
�� �� ֵ �� ��
*************************************************/
void OLED_Fill( uint8_t Mode )
{
    uint8_t y,x;

    for(y = 0;y < 8;y++)
    {
        OLED_Write_Cmd(0xB0 + y);               // �� nҳ��ʼ��0~7��
        OLED_Write_Cmd(0x10);                   // ������ʾλ�� - �и�λ��һ��
        OLED_Write_Cmd(0x00);                   // ������ʾλ�� - �е�λ��һ��
        for(x = 0;x < OLED_MAX_COLUMN;x++)
        {
            OLED_Write_Data(Mode, DISABLE); // Data = 0x00ȫ����Data = 0xffȫ����
        }
    }
}

/************************************************
�������� �� OLED_Row_Clear
��    �� �� OLED������� / ��ʾ
��    �� �� Row ---- ����
            Amount ---- ����
			Mode ---- ���/��ʾ(OLED_CLS or OLED_SHOW)
�� �� ֵ �� ��
*************************************************/
void OLED_Row_Clear( uint8_t Row, uint8_t Amount ,uint8_t Mode )
{
    uint8_t y,x;

    if(Row < 8)
    {
        for(y = 0;y < Amount;y++)
        {
            OLED_Write_Cmd(0xB0 + Row + y);         // �� nҳ��ʼ��0~7��
            OLED_Write_Cmd(0x10);                   // ������ʾλ�� - �и�λ��һ��
            OLED_Write_Cmd(0x00);                   // ������ʾλ�� - �е�λ��һ��
            for(x = 0;x < OLED_MAX_COLUMN;x++)
            {
                OLED_Write_Data(Mode, DISABLE);
            }
        }
    }
}

/************************************************
�������� �� OLED_Coord
��    �� �� OLED������ʾ
��    �� �� X ---- X��
			Y ---- Y��
�� �� ֵ �� ��
*************************************************/
void OLED_Coord( uint8_t X, uint8_t Y )
{
//    assert(X < X_WIDTH);
//    assert(Y < (Y_WIDTH >> 3));

    /* B0~B7:�������������ҳ��Ѱַģʽ */
    OLED_Write_Cmd(0xB0 + Y);                   // ����GDDRAMҳ�濪ʼ��ַ,(Page 0~ Page 7)Ϊҳ��Ѱַģʽ

    /* 10~1F:�������������ҳ��Ѱַģʽ */
    OLED_Write_Cmd(((X & 0xF0) >> 4) | 0x10);   // ҳ��Ѱַģʽ�������п�ʼ��ַ�Ĵ����ĸ�λ

    /* 00~0F:�������������ҳ��Ѱַģʽ */
    OLED_Write_Cmd((X & 0x0F) | 0x00);          // ҳ��Ѱַģʽ�������п�ʼ��ַ�Ĵ����ĵ�λ
}

/************************************************
�������� �� OLED_ShowRoll
��    �� �� OLED���ù�����ʾ
��    �� �� Y ---- ��ʼ��
            Line ---- �������� (0 ---> ȡ������)
            Mode ---- ����ģʽ��OLED_LEFT_ROLL or OLED_RIGHT_ROLL��
�� �� ֵ �� ��
*************************************************/
void OLED_ShowRoll( uint8_t Y, uint8_t Line, uint8_t Mode )
{
    if(Line > 0)
    {
        if(Mode == OLED_LEFT_ROLL)
        {
            OLED_Write_Cmd(OLED_LEFT_ROLL);     // �������ʾ
        }
        else if(Mode == OLED_RIGHT_ROLL)
        {
            OLED_Write_Cmd(OLED_RIGHT_ROLL);    // �ҹ�����ʾ
        }
        else
        {
            return ;
        }
        OLED_Write_Cmd(0x00);                   // �����ֽ����ã�Ĭ��Ϊ0x00
        OLED_Write_Cmd(Y);                      // ���忪ʼҳ���ַ
        OLED_Write_Cmd(0x07);                   // ֡Ƶ����
        OLED_Write_Cmd(Y + (Line - 1));  		// �������ҳ���ַ
        OLED_Write_Cmd(0x00);                   // �����ֽ����ã�Ĭ��Ϊ0x00
        OLED_Write_Cmd(0xFF);                   // �����ֽ����ã�Ĭ��Ϊ0xFF
        OLED_Write_Cmd(0x2F);                   // �������
    }
    else
    {
        OLED_Write_Cmd(0x2E);                   // ʧ�ܹ���
    }
}

/************************************************
�������� �� OLED_ShowChar
��    �� �� OLED�ַ���ʾ
��    �� �� X ---- X��
			Y ---- Y��
            Char ---- �ַ�
            Size ---- �����С
            Inverse ---- ������ʾʹ��
�� �� ֵ �� ��
*************************************************/
void OLED_ShowChar( uint8_t X, uint8_t Y, uint8_t Char, uint8_t Size, uint8_t Inverse )
{
    uint8_t i;
	uint8_t value = 0;

    if(X <= OLED_MAX_COLUMN - 8 && (Y < (OLED_MAX_ROW >> 3)))
    {
		if(X >= 0 && Y >= 0)
		{
			value = Char - 32;                                          // �õ�ƫ��ֵ(ƫ����32)

			if(Size == OLED_FONT_SIXTEEN)                               // 8x16
			{
				OLED_Coord(X, Y);
				for(i = 0;i < 8;i++)
				{
					OLED_Write_Data(F8x16[value][i], Inverse);      // ǰ�˸�����
				}

				OLED_Coord(X, Y+1);
				for(i = 0;i < 8;i++)
				{
					OLED_Write_Data(F8x16[value][i + 8], Inverse);  // ��˸�����
				}
			}
			else if(Size == OLED_FONT_EIGHT)                           // 6x8
			{
				OLED_Coord(X, Y);
				for(i = 0;i < 6;i++)
				{
					OLED_Write_Data(F6x8[value][i], Inverse);
				}
				/* ���ʣ�µ����� */
				OLED_Write_Data(0x00, Inverse);
				OLED_Write_Data(0x00, Inverse);
			}
		}
    }
}

/************************************************
�������� �� OLED_ShowString
��    �� �� OLED�ַ�����ʾ���������Զ�������
��    �� �� X ---- X��
			Y ---- Y��
            Len ---- ����
            pChar ---- ����
            Size ---- �����С
            Inverse ---- ���ף�ȫ�У���ʾʹ��
�� �� ֵ �� ��
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
                OLED_Write_Data(OLED_SHOW, DISABLE);    // OLED_CLS = 0x00ȫ����OLED_SHOW = 0xffȫ����
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
            temp = *(pChar + j) - 32;                                   // �õ�ƫ��ֵ(ƫ����32)

            if(Size == OLED_FONT_SIXTEEN)                               // 8x16
            {
                for(i = 0;i < 16;i++)
                {
                    g_OLED_Gram[j][i] = F8x16[temp][i];                   // �����Դ���
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
�������� �� OLED_ShowPrintf
��    �� �� OLED�ַ���������Զ��������뻻�У�
��    �� �� X ---- X��
			Y ---- Y��
            pChar ---- ����
            Size ---- �����С
            Align ---- ������ʾʹ��
            Inverse ---- ���ף������壩��ʾʹ��
�� �� ֵ �� ��
*************************************************/
void OLED_ShowPrintf( uint8_t X, uint8_t Y, const uint8_t *pChar, uint8_t Size, _Bool Align, uint8_t Inverse )
{
    uint8_t addr = 0;

    if(Align)
    {
        addr = X;		// ��¼��������λ�� 
    }

	while(*pChar != '\0')
	{
		if(X >= OLED_MAX_COLUMN - 8)             // �����
        {
            X = addr;                           // �����е�ַ
            Y += (Size >> 3);                   // ת����һ��
            if(Y >= (OLED_MAX_ROW >> 3))
            {
                break;                         // Y��Խ���˳�
            }
        }
        OLED_ShowChar(X, Y, *pChar, Size, Inverse);
		X += 8;
        pChar++;
	}
}

/************************************************
�������� �� OLED_Power
��    �� �� OLED�ݼ���
��    �� �� M ---- X��
			N ---- Y��
�� �� ֵ �� power ---- ��
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
�������� �� OLED_ShowNum
��    �� �� OLED������ʾ������������
��    �� �� X ---- X��
			Y ---- Y��
            Num ---- ����
            Len ---- λ��
            Size ---- �����С
			Prefix ---- ������ʾʹ��
            Inverse ---- ������ʾʹ��
�� �� ֵ �� ��
*************************************************/
void OLED_ShowNum( uint8_t X, uint8_t Y, uint32_t Num, uint8_t Len, uint8_t Size, uint8_t Prefix, uint8_t Inverse )
{
	char temp = 0;
	uint8_t t;
	uint8_t show = 0;

	for(t = 0;t < Len;t++)
	{
		temp = (Num / OLED_Power(10, Len - t - 1)) % 10;          // ��ȡÿλ����

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
�������� �� OLED_ShowHex
��    �� �� OLEDʮ��������ʾ
��    �� �� X ---- X��
			Y ---- Y��
            Num ---- ���֣�10���ƣ�
            Size ---- �����С���̶����Ϊ 8���أ�
            Prefix ---- ������ʾʹ��
            Inverse ---- ������ʾʹ��
�� �� ֵ �� ��
*************************************************/
void OLED_ShowHex( uint8_t X, uint8_t Y, uint32_t Num, uint8_t Size, uint8_t Prefix, uint8_t Inverse )
{
	uint8_t t,temp;
    uint8_t i = 0;
	uint8_t show = 0;

	for(t = 0;t < 4;t++)
	{
		temp = (uint8_t)(Num >> (12 - 4*t)) & 0x000F;          // ��ȡÿλ����

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
�������� �� OLED_ShowFloat
��    �� �� OLED�ɱ侫��С����ʾ
��    �� �� X ---- X��
			Y ---- Y��
            Num ---- ����
            Accuracy ---- ��ȷλ��
            Size ---- �����С
            Inverse ---- ������ʾʹ��
�� �� ֵ �� ��
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


    /* �ж��Ƿ�Ϊ���� */
    if(Num < 0)
    {
        OLED_ShowChar(X, Y , '-', Size, Inverse);
        Num = 0 - Num;
        i++;
    }

    integer = (uint32_t)Num;
    decimals = Num - integer;

    /* �������� */
    if(integer)
    {
        numel = integer;

		while(numel)                                                        // ��ȡ����λ��
		{
			numel /= 10;
            j++;
		}
        i += (j - 1);
        for(temp = 0;temp < j;temp++)
        {
            OLED_ShowChar(X + 8 *(i - temp), Y, integer % 10 + '0', Size, Inverse);  // ��ʾ��������
            integer /= 10;
        }
	}
    else
    {
		OLED_ShowChar(X + 8 *i, Y, temp + '0', Size, Inverse);
	}
    i++;

    /* С������ */
    if(Accuracy)
    {
        OLED_ShowChar(X + 8 *i, Y , '.', Size, Inverse);                    // ��ʾС����

        i++;
        for(t = 0;t < Accuracy;t++)
        {
            decimals *= 10;
            temp = (uint8_t)decimals;                                       // ��ȡÿλС��
            OLED_ShowChar(X + 8 *(i + t), Y, temp + '0', Size, Inverse);
            decimals -= temp;
        }
    }

}

/************************************************
�������� �� OLED_ShowLanguage
��    �� �� OLED������ʾ(16x16) �����ֿ�� 
��    �� �� X ---- X��
			Y ---- Y��
            pArray ---- ����
            Len ---- ��������
            Inverse ---- ������ʾʹ��
�� �� ֵ �� ��
*************************************************/

/*
    mode 0:ʹ�� 1��ָ�������ʶ�ά����
    mode 1:ʹ��ָ�������ָ�������ʶ�ά����
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
	uint8_t distribute_flag = 0;									// ����ƽ���ֲ���־ 

    if(Len <= 8)
    {
        if(!X)		// ��λ��ʾ���Զ�ƽ���ֲ� 
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
                OLED_Write_Data(*(pArray + 2*j*16 + i), Inverse);      // ǰʮ��������

#elif (1 == _OLED_ShowLanguage_MODE)
                OLED_Write_Data(pArray[2*j][i], Inverse);       // ǰʮ��������

#endif /* _OLED_ShowLanguage_MODE */
            }

            OLED_Coord(X, Y+1);
            for(i = 0;i < 16;i++)
            {

#if (0 == _OLED_ShowLanguage_MODE)
                OLED_Write_Data(*(pArray + 2*j*16 + 16 + i), Inverse);           // ��ʮ��������

#elif (1 == _OLED_ShowLanguage_MODE)
                OLED_Write_Data(pArray[2*j + 1][i], Inverse);   // ��ʮ��������

#endif /* _OLED_ShowLanguage_MODE */
            }
            X += (16 + temp);

            if(distribute_flag)
            {
            	q++;

	            /* ���⴦�� */
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
                g_OLED_Gram[j][i] = *(pArray + j*16 + i);                  // �����Դ���

#elif (1 == _OLED_ShowLanguage_MODE)
                g_OLED_Gram[j][i] = pArray[j][i];                 // �����Դ���

#endif /* _OLED_ShowLanguage_MODE */
            }
        }

    }
}

/************************************************
�������� �� OLED_Display_On
��    �� �� ����OLED��ʾ
��    �� �� ��
�� �� ֵ �� ��
*************************************************/
void OLED_Display_On(void)
{
	OLED_Write_Cmd(0x8D);       //SET DCDC����
	OLED_Write_Cmd(0x14);       //DCDC ON
	OLED_Write_Cmd(0xAF);       //DISPLAY ON
}

/************************************************
�������� �� OLED_Display_Off
��    �� �� �ر�OLED��ʾ
��    �� �� ��
�� �� ֵ �� ��
*************************************************/
void OLED_Display_Off(void)
{
	OLED_Write_Cmd(0x8D);       // SET DCDC����
	OLED_Write_Cmd(0x10);       // DCDC OFF
	OLED_Write_Cmd(0xAE);       // DISPLAY OFF
}

/************************************************
�������� �� OLED_Config
��    �� �� OLED����
��    �� �� ��
�� �� ֵ �� ��
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
�������� �� OLED_Init
��    �� �� OLED��ʼ��
��    �� �� ��
�� �� ֵ �� ��
*************************************************/
void OLED_Init(void)
{
    OLED_Config();

#ifndef _OLED_IIC_MODE
	OLED_SCK(HIGH);          // ����̬ʱ��SCLK���ڸߵ�ƽ
	OLED_CS(HIGH);	         // �ر�ѡ������

    /* ���ϵ絽���濪ʼ��ʼ��Ҫ���㹻��ʱ�䣬���ȴ� RC��λ��� */
	OLED_RST(LOW);           // ��λ���͵�ƽ��Ч��
    OLED_Delay_ms(100);
	OLED_RST(HIGH);
    OLED_Delay_ms(30);

#endif /* _OLED_IIC_MODE */

    OLED_Write_Cmd(0xAE);    // �ر�OLED -- turn off oled panel
    OLED_Write_Cmd(0xD5);    // ������ʾʱ�ӷ�Ƶ����/����Ƶ�� -- set display clock divide ratio/oscillator frequency
    OLED_Write_Cmd(0x80);    // \ set divide ratio, Set Clock as 100 Frames/Sec
    OLED_Write_Cmd(0x20);    // �����ڴ�Ѱַģʽ -- Set Memory Addressing Mode (0x00 / 0x01 / 0x02)
    OLED_Write_Cmd(0x02);    // \ Page Addressing
    OLED_Write_Cmd(0xA8);    // ���ö�·������� -- set multiplex ratio (16 to 63)
    OLED_Write_Cmd(0x3F);    // \ 1 / 64 duty
    OLED_Write_Cmd(0xDA);    // ����������Ӳ������ -- set com pins hardware configuration
    OLED_Write_Cmd(0x12);    // \ Sequential COM pin configuration��Enable COM Left/Right remap

    /* ----- ������ʾ���� ----- */
    OLED_Write_Cmd(0xA1);    // ���ö���ӳ�� -- Set SEG / Column Mapping     0xA0���ҷ��ã���λֵ�� 0xA1��������ӳ��ֵ��
    OLED_Write_Cmd(0xC8);    // ���������ɨ�跽�� -- Set COM / Row Scan Direction   0xc0���·��ã���λֵ�� 0xC8��������ӳ��ֵ��
    /* ----- END ----- */

    OLED_Write_Cmd(0x40);    // ����������Ļ��GDDRAM����ʼ�� -- Set Display Start Line (0x40~0x7F)
    OLED_Write_Cmd(0xD3);    // ������ʾƫ�� -- set display offset (0x00~0x3F)
    OLED_Write_Cmd(0x00);    // \ not offset
    OLED_Write_Cmd(0x81);    // ���öԱȶ� -- set contrast control register (0x00~0x100)
    OLED_Write_Cmd(0xCF);    // \ Set SEG Output Current Brightness
    OLED_Write_Cmd(0xD9);    // ����Ԥ����ڼ�ĳ���ʱ�� -- set pre-charge period
    OLED_Write_Cmd(0xF1);    // \ Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    OLED_Write_Cmd(0xDB);    // ����VCOMH����������� -- set vcomh (0x00 / 0x20 / 0x30)
    OLED_Write_Cmd(0x20);    // \ Set VCOM Deselect Level
    OLED_Write_Cmd(0x8D);    // ��ɱ����� -- set Charge Pump enable / disable (0x14 / 0x10)
    OLED_Write_Cmd(0x14);    // \ Enable charge pump during display on
    OLED_Write_Cmd(0xA4);    //  ȫ����ʾ����(����/����) -- Entire Display On (0xA4 / 0xA5)
    OLED_Write_Cmd(0xA6);    // ������ʾ��ʽ(����/����) -- set normal display (0xA6 / 0xA7)

    OLED_Write_Cmd(0xAF);    // ��OLED -- turn on oled panel
    OLED_Fill(0x00);         // ��ʼ����
    OLED_Coord(0,0);         // ����ԭ�����꣨0, 0��
}

/************************* 	��Ҫ�ṩ�ֿ�����ļ� *************************/

#ifdef _FONT_LIBRARY

/************************************************
�������� �� OLED_ShowChinese
��    �� �� OLED��������ʾ 
��    �� �� X ---- X��
			Y ---- Y��
            pArray ---- ����
            Inverse ---- ������ʾʹ��
�� �� ֵ �� �� 
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
			OLED_Write_Data(buffer[i], Inverse);      // ǰʮ��������
		}
		OLED_Coord(X, Y+1);
		for(i = 0;i < 16;i++)
		{
			OLED_Write_Data(buffer[i + 16], Inverse); // ��ʮ��������
		}
	}
}

/************************************************
�������� �� OLED_Draw_Font
��    �� �� OLED��Ӣ�����ʾ��֧�ֻ����ʾ��
��    �� �� X ---- X��
			Y ---- Y��
            pArray ---- ����
            Inverse ---- ������ʾʹ��
�� �� ֵ �� �� 
*************************************************/
void OLED_Draw_Font( uint8_t X, uint8_t Y, const uint8_t *pArray, uint8_t Inverse )
{
	uint8_t i = 0;
	uint16_t j = 0;
	uint8_t buffer[GB2312_1616_BYTE_SIZE] = {0};
	
	while(*pArray != '\0')
	{
		if(*pArray < 127)			// ASCII��
		{
			if(X + 8 > OLED_MAX_COLUMN)
			{
				X = 0;
				Y += 2;			// ����
			}
			if(Y > (OLED_MAX_ROW >> 3))
			{
				X = 0;
				Y = 0;			// ����ԭ��
			}
			
			OLED_ShowChar(X, Y, *pArray, OLED_FONT_SIXTEEN, Inverse);
			X += 8;
			pArray++;			// һ�� ASCII��1���ֽ�
		}
		else						// GB2312��
		{
			if(X + 16 > OLED_MAX_COLUMN)
			{
				X = 0;
				Y += 2;			// ����
			}
			if(Y > (OLED_MAX_ROW >> 3))
			{
				X = 0;
				Y = 0;			// ����ԭ��
			}

//			Get_GB2312_Code(buffer, pArray);
//			
//			OLED_Coord(X, Y);
//			for(i = 0;i < 16;i++)
//			{
//				OLED_Write_Data(*(buffer + 2*j*16 + i), Inverse);      // ǰʮ��������
//			}
//			OLED_Coord(X, Y+1);
//			for(i = 0;i < 16;i++)
//			{
//				OLED_Write_Data(*(buffer + 2*j*16 + 16 + i), Inverse); // ��ʮ��������
//			}
//			j++;
			
			OLED_ShowChinese(X, Y, pArray, Inverse);
			X += 16;
			pArray += 2;		// һ�� GB2312���������ֽ�
		}
	}
}

#endif /* _FONT_LIBRARY */


/************************* 	��Ҫ�ṩ�ֿ�����ļ� *************************/


/*---------------------------- END OF FILE ----------------------------*/


