/*
*********************************************************************************************************
*
*	模块名称 : modbus底层驱动程序
*	文件名称 : bsp_modbus.c
*	版    本 : V1.0
*	说    明 : Modbus驱动程序，提供收发的函数。
*
*	Copyright (C), 2014-2015, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"

static void MODBUS_RxTimeOut(void);
static uint8_t g_rtu_timeout = 0;

MODBUS_T g_tModbus;

extern void MODBUS_AnalyzeApp(void);
/*
*********************************************************************************************************
*	函 数 名: MODBUS_InitVar
*	功能说明: 初始化Modbus结构变量
*	形    参: _Baud 通信波特率，改参数决定了RTU协议包间的超时时间。3.5个字符。
*			  _WorkMode 接收中断处理模式1. RXM_NO_CRC   RXM_MODBUS_HOST   RXM_MODBUS_DEVICE
*
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_InitVar(uint32_t _Baud, uint8_t _WorkMode)
{
	g_rtu_timeout = 0;
	g_tModbus.RxCount = 0;

	g_tModbus.Baud = _Baud;

	g_tModbus.WorkMode = WKM_NO_CRC;	/* 接收数据帧不进行CRC校验 */

	bsp_Set485Baud(_Baud);
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_Poll
*	功能说明: 解析数据包. 在主程序中轮流调用。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_Poll(void)
{
	uint16_t crc1;

	if (g_rtu_timeout == 0)
	{
		/* 没有超时，继续接收。不要清零 g_tModbus.RxCount */
		return;
	}

	/* 收到命令
		05 06 00 88 04 57 3B70 (8 字节)
			05    :  数码管屏的号站，
			06    :  指令
			00 88 :  数码管屏的显示寄存器
			04 57 :  数据,,,转换成 10 进制是 1111.高位在前,
			3B70  :  二个字节 CRC 码	从05到 57的校验
	*/
	g_rtu_timeout = 0;

	switch (g_tModbus.WorkMode)
	{
		case WKM_NO_CRC:	/* 接收数据帧不进行CRC校验. 用于ASCII协议 */
			{
				/* 将接收的数据复制到另外一个缓冲区，等待APP程序读取 */
				memcpy(g_tModbus.AppRxBuf, g_tModbus.RxBuf, g_tModbus.RxCount);
				g_tModbus.AppRxCount = g_tModbus.RxCount;
				bsp_PutKey(MSG_485_RX);		/* 借用按键FIFO，发送一个收到485数据帧的消息 */
			}
			break;

		case WKM_MODBUS_HOST:			/* Modbus 主机模式 */
			if (g_tModbus.RxCount < 4)
			{
				goto err_ret;
			}

			/* 计算CRC校验和 */
			crc1 = CRC16_Modbus(g_tModbus.RxBuf, g_tModbus.RxCount);
			if (crc1 != 0)
			{
				goto err_ret;
			}

			/* 站地址 (1字节） */
			g_tModbus.AppRxAddr = g_tModbus.RxBuf[0];	/* 第1字节 站号 */

			/* 将接收的数据复制到另外一个缓冲区，等待APP程序读取 */
			memcpy(g_tModbus.AppRxBuf, g_tModbus.RxBuf, g_tModbus.RxCount);
			g_tModbus.AppRxCount = g_tModbus.RxCount;
			bsp_PutKey(MSG_485_RX);		/* 借用按键FIFO，发送一个收到485数据帧的消息 */
			break;

		case WKM_MODBUS_DEVICE:			/* Modbus 从机模式 */
			if (g_tModbus.RxCount < 4)
			{
				goto err_ret;
			}

			/* 计算CRC校验和 */
			crc1 = CRC16_Modbus(g_tModbus.RxBuf, g_tModbus.RxCount);
			if (crc1 != 0)
			{
				goto err_ret;
			}

			/* 站地址 (1字节） */
			g_tModbus.AppRxAddr = g_tModbus.RxBuf[0];	/* 第1字节 站号 */

			/* 分析应用层协议 */
			MODBUS_AnalyzeApp();
			break;

		default:
			break;
	}

err_ret:
	g_tModbus.RxCount = 0;	/* 必须清零计数器，方便下次帧同步 */
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_ReciveNew
*	功能说明: 串口接收中断服务程序会调用本函数。当收到一个字节时，执行一次本函数。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_ReciveNew(uint8_t _byte)
{
	/*
		3.5个字符的时间间隔，只是用在RTU模式下面，因为RTU模式没有开始符和结束符，
		两个数据包之间只能靠时间间隔来区分，Modbus定义在不同的波特率下，间隔时间是不一样的，
		所以就是3.5个字符的时间，波特率高，这个时间间隔就小，波特率低，这个时间间隔相应就大

        4800  = 7.297ms
        9600  = 3.646ms
        19200  = 1.771ms
        38400  = 0.885ms
	*/
	uint32_t timeout;

	timeout = 35000000 / g_tModbus.Baud;		/* 计算超时时间，单位us */

	/* 硬件定时中断，定时精度us 定时器4用于Modbus */
	bsp_StartHardTimer(4, timeout, (void *)MODBUS_RxTimeOut);

	if (g_tModbus.RxCount < MODBUS_RX_SIZE)
	{
		g_tModbus.RxBuf[g_tModbus.RxCount++] = _byte;
	}
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_RxTimeOut
*	功能说明: 超过3.5个字符时间后执行本函数。 设置全局变量 g_rtu_timeout = 1; 通知主程序开始解码。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_RxTimeOut(void)
{
	g_rtu_timeout = 1;
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_SendWithCRC
*	功能说明: 发送一串数据, 自动追加2字节CRC
*	形    参: _pBuf 数据；
*			  _ucLen 数据长度（不带CRC）
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_SendWithCRC(uint8_t *_pBuf, uint8_t _ucLen)
{
	uint16_t crc;
	uint8_t buf[MODBUS_TX_SIZE];

	memcpy(buf, _pBuf, _ucLen);
	crc = CRC16_Modbus(_pBuf, _ucLen);
	buf[_ucLen++] = crc >> 8;
	buf[_ucLen++] = crc;
	RS485_SendBuf(buf, _ucLen);
}

/* Modbus 应用层解码示范。下面的代码请放在主程序做 */

/*
*********************************************************************************************************
*	函 数 名: MODBUS_01H
*	功能说明: 读取线圈状态（对应远程开关D01/D02/D03）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_01H(void)
{
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_02H
*	功能说明: 读取输入状态（对应T01～T18）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_02H(void)
{

}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_03H
*	功能说明: 读取保持寄存器 在一个或多个保持寄存器中取得当前的二进制值
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_03H(void)
{

}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_04H
*	功能说明: 读取输入寄存器（对应A01/A02） SMA
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_04H(void)
{

}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_05H
*	功能说明: 强制单线圈（对应D01/D02/D03）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_05H(void)
{

}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_06H
*	功能说明: 写单个寄存器
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/

static void MODBUS_06H(void)
{

}


/*
*********************************************************************************************************
*	函 数 名: MODBUS_10H
*	功能说明: 连续写多个寄存器.  进用于改写时钟
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_10H(void)
{

}


/*
*********************************************************************************************************
*	函 数 名: MODBUS_AnalyzeApp
*	功能说明: 分析应用层协议
*	形    参:
*		     _DispBuf  存储解析到的显示数据ASCII字符串，0结束
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_AnalyzeApp(void)
{
	/* Modbus从机 */
	switch (g_tModbus.RxBuf[1])			/* 第2个字节 功能码 */
	{
		case 0x01:	/* 读取线圈状态（对应远程开关D01/D02/D03） */
			MODBUS_01H();
			break;

		case 0x02:	/* 读取输入状态（对应T01～T18） */
			MODBUS_02H();
			break;

		case 0x03:	/* 读取保持寄存器 在一个或多个保持寄存器中取得当前的二进制值 */
			MODBUS_03H();
			break;

		case 0x04:	/* 读取输入寄存器（对应A01/A02） ） */
			MODBUS_04H();
			break;

		case 0x05:	/* 强制单线圈（对应D01/D02/D03） */
			MODBUS_05H();
			break;

		case 0x06:	/* 写单个寄存器 (存储在EEPROM中的参数) */
			MODBUS_06H();
			break;

		case 0x10:	/* 写多个寄存器 （改写时钟） */
			MODBUS_10H();
			break;

		default:
			g_tModbus.RspCode = RSP_ERR_CMD;
			//MODBUS_SendAckErr(g_tModbus.RspCode);	/* 告诉主机命令错误 */
			break;
	}
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
