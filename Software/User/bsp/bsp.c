/*
*********************************************************************************************************
*
*	ģ������ : BSPģ��(For STM32F1XX)
*	�ļ����� : bsp.c
*	��    �� : V1.0
*	˵    �� : ����Ӳ���ײ���������ģ������ļ�����Ҫ�ṩ bsp_Init()��������������á��������ÿ��c�ļ������ڿ�
*			  ͷ	���� #include "bsp.h" ���������е���������ģ�顣
*
*	�޸ļ�¼ :
*		�汾��  ����        ����     ˵��
*		V1.0    2013-03-01 armfly   ��ʽ����
*		V1.1    2015-08-02 Eric2013 ����uCOS-II���躯��
*
*	Copyright (C), 2015-2020, ���������� www.armfly.com
*
*********************************************************************************************************
*/

#include "includes.h"


/*
*********************************************************************************************************
*	�� �� ��: bsp_Init
*	����˵��: ��ʼ��Ӳ���豸��ֻ��Ҫ����һ�Ρ��ú�������CPU�Ĵ���������ļĴ�������ʼ��һЩȫ�ֱ�����
*			 ȫ�ֱ�����
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void bsp_Init(void)
{
	/*
		����ST�̼���������ļ��Ѿ�ִ����CPUϵͳʱ�ӵĳ�ʼ�������Բ����ٴ��ظ�����ϵͳʱ�ӡ�
		�����ļ�������CPU��ʱ��Ƶ�ʡ��ڲ�Flash�����ٶȺͿ�ѡ���ⲿSRAM FSMC��ʼ����

		ϵͳʱ��ȱʡ����Ϊ72MHz�������Ҫ���ģ������޸� system_stm32f10x.c �ļ�
	*/

	/* ���ȼ���������Ϊ4 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	
	bsp_InitUart(); 	/* ��ʼ������ */
	bsp_InitLed(); 		/* ��ʼLEDָʾ�ƶ˿� */
	bsp_InitTm1638();   /* TM1638 ����ܡ����� */
	bsp_Adc1Init();     /* ADC �ɼ���ѹ */
	bsp_InitSPIBus();   /* SPI for W25Q64 */
	bsp_InitSFlash();	/* SPI for W25Q64 */
	bsp_RelayInit();    /* �̵�����ʼ�� */
	bsp_InputStateInit(); /* ����IO������ */
	bsp_SensorInit(720, 6000); // cunt 10us,  max 600ms
	
	
//	bsp_InitKey();		/* ��ʼ������ */

}

/*
*********************************************************************************************************
*	�� �� ��: BSP_CPU_ClkFreq
*	����˵��: ��ȡϵͳʱ�ӣ�uCOS-II��Ҫʹ��
*	��    ��: ��
*	�� �� ֵ: ϵͳʱ��
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
*	�� �� ��: BSP_Tick_Init
*	����˵��: ��ʼ��ϵͳ�δ�ʱ����ΪuCOS-II��ϵͳʱ�ӽ��ģ�1msһ��
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void BSP_Tick_Init (void)
{
    CPU_INT32U  cpu_clk_freq;
    CPU_INT32U  cnts;
    
    cpu_clk_freq = BSP_CPU_ClkFreq();                           /* ��ȡϵͳʱ��  */
    
#if (OS_VERSION >= 30000u)
    cnts  = cpu_clk_freq / (CPU_INT32U)OSCfg_TickRate_Hz;     
#else
    cnts  = cpu_clk_freq / (CPU_INT32U)OS_TICKS_PER_SEC;        /* ��õδ�ʱ���Ĳ���  */
#endif
    
	OS_CPU_SysTickInit(cnts);                                   /* ����Ĭ�ϵ���������ȼ�            */
	//SysTick_Config(SystemCoreClock / 1000);
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/