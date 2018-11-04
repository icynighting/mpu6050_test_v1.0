#include "Bsp_usart_Init.h"

void Bsp_usart_Init()
{

  //配置USART2	
   GPIO_InitTypeDef   GPIO_InitStructure;  
  USART_InitTypeDef  USART_InitStructure;
  NVIC_InitTypeDef NVIC_Initstructure;
  
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
  

  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9|GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;
  GPIO_Init(GPIOA,&GPIO_InitStructure);

  
	
  USART_InitStructure.USART_BaudRate=115200;
  USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;
  USART_InitStructure.USART_Parity=USART_Parity_No;
  USART_InitStructure.USART_StopBits=USART_StopBits_1;
  USART_InitStructure.USART_WordLength=USART_WordLength_8b;
  USART_Init(USART1,&USART_InitStructure);
  
     //引脚复用设置
   GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
   GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);
  
  USART_Cmd(USART1,ENABLE);
  
  USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);//打开接收中断，接收到数据就会执行中断服务函数
 
  NVIC_Initstructure.NVIC_IRQChannel=USART1_IRQn;//在stm32f10x.h中的通道
  NVIC_Initstructure.NVIC_IRQChannelCmd=ENABLE;
  NVIC_Initstructure.NVIC_IRQChannelPreemptionPriority=1;
  NVIC_Initstructure.NVIC_IRQChannelSubPriority=1;
  NVIC_Init(&NVIC_Initstructure);
  
}


void USART1_IRQHandler(void)
{
  extern   u8 res;
  extern   u8 res1[8];
  extern   u8 j;
  extern   u8 k;
  if(USART_GetITStatus(USART1,USART_IT_RXNE))
  {  
     res=USART_ReceiveData(USART1);                   
      USART_SendData(USART1,res);
      if ((res ==0x0D)||(res ==0x0A))    j=0 ; 	 
	 else 
	{
	  res1[j]=res-0x30;
		j++;
		k=j;
	}
  }
 
}


