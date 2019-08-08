/*
*********************************************************************************************************
*
*	模块名称 : BSP模块(For STM32F1XX)
*	文件名称 : bsp.c
*	版    本 : V1.0
*	说    明 : 这是硬件底层驱动程序模块的主文件。主要提供 bsp_Init()函数供主程序调用。主程序的每个c文件可以在开
*			  头	添加 #include "bsp.h" 来包含所有的外设驱动模块。
*
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2013-03-01 armfly   正式发布
*		V1.1    2015-08-02 Eric2013 增加uCOS-II所需函数
*
*	Copyright (C), 2015-2020, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "includes.h"


/*
*********************************************************************************************************
*	函 数 名: bsp_Init
*	功能说明: 初始化硬件设备。只需要调用一次。该函数配置CPU寄存器和外设的寄存器并初始化一些全局变量。
*			 全局变量。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_Init(void)
{
	/*
		由于ST固件库的启动文件已经执行了CPU系统时钟的初始化，所以不必再次重复配置系统时钟。
		启动文件配置了CPU主时钟频率、内部Flash访问速度和可选的外部SRAM FSMC初始化。

		系统时钟缺省配置为72MHz，如果需要更改，可以修改 system_stm32f10x.c 文件
	*/

	/* 优先级分组设置为4 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	
	bsp_InitUart(); 	/* 初始化串口 */
	bsp_InitLed(); 		/* 初始LED指示灯端口 */
	bsp_InitTm1638();   /* TM1638 数码管、按键 */
	bsp_Adc1Init();     /* ADC 采集电压 */
	bsp_InitSPIBus();   /* SPI for W25Q64 */
	bsp_InitSFlash();	/* SPI for W25Q64 */
	bsp_RelayInit();    /* 继电器初始化 */
	bsp_InputStateInit(); /* 输入IO传感器 */
	bsp_SensorInit(720, 60000); // cunt 10us,  max 600ms
	bsp_InitRTC();        /* 初始化RTC */
	
//	bsp_InitKey();		/* 初始化按键 */

}

/*
*********************************************************************************************************
*	函 数 名: BSP_CPU_ClkFreq
*	功能说明: 获取系统时钟，uCOS-II需要使用
*	形    参: 无
*	返 回 值: 系统时钟
*********************************************************************************************************
*/

CPU_INT32U  BSP_CPU_ClkFreq (void)
{
    RCC_ClocksTypeDef  rcc_clocks;

    RCC_GetClocksFreq(&rcc_clocks);
    return ((CPU_INT32U)rcc_clocks.HCLK_Frequency);
}


/*
*********************************************************************************************************
*	函 数 名: BSP_Tick_Init
*	功能说明: 初始化系统滴答时钟做为uCOS-II的系统时钟节拍，1ms一次
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void BSP_Tick_Init (void)
{
    CPU_INT32U  cpu_clk_freq;
    CPU_INT32U  cnts;
    
    cpu_clk_freq = BSP_CPU_ClkFreq();                           /* 获取系统时钟  */
    
#if (OS_VERSION >= 30000u)
    cnts  = cpu_clk_freq / (CPU_INT32U)OSCfg_TickRate_Hz;     
#else
    cnts  = cpu_clk_freq / (CPU_INT32U)OS_TICKS_PER_SEC;        /* 获得滴答定时器的参数  */
#endif
    
	OS_CPU_SysTickInit(cnts);                                   /* 这里默认的是最低优先级            */
	//SysTick_Config(SystemCoreClock / 1000);
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
