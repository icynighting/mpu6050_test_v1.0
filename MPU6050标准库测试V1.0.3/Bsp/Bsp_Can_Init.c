#include "Bsp_Can_Init.h"


//全局变量
CanRxMsg RxMessage1;  
CanRxMsg RxMessage2; 


//CAN1初始化
//tsjw:重新同步跳跃时间单元.范围:CAN_SJW_1tq~ CAN_SJW_4tq
//tbs2:时间段2的时间单元.   范围:CAN_BS2_1tq~CAN_BS2_8tq;
//tbs1:时间段1的时间单元.   范围:CAN_BS1_1tq ~CAN_BS1_16tq
//brp :波特率分频器.范围:1~1024; tq=(brp)*tpclk1
//波特率=Fpclk1/((tbs1+1+tbs2+1+1)*brp);
//mode:CAN_Mode_Normal,普通模式;CAN_Mode_LoopBack,回环模式;
//Fpclk1的时钟在初始化的时候设置为42M,如果设置CAN1_Mode_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,CAN_Mode_LoopBack);
//则波特率为:42M/((6+7+1)*6)=500Kbps
//返回值:0,初始化OK;
//    其他,初始化失败; 

u8 Bsp_Can1_Init(u8 tsjw,u8 tbs2,u8 tbs1,u8 brp,u8 mode)
{
	
   GPIO_InitTypeDef GPIO_InitStructure;
   CAN_InitTypeDef CAN_InitStructrue;
   CAN_FilterInitTypeDef CAN_FilterInitStructure;
#if CAN1_RX0_INT_ENABLE
   NVIC_InitTypeDef NVIC_InitStructure;
#endif
	
   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1,ENABLE);
   
	
	
   GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;//复用功能
   GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;//推挽输出
   GPIO_InitStructure.GPIO_Pin=GPIO_Pin_11|GPIO_Pin_12;
   GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;//上拉
   GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
   GPIO_Init(GPIOA,&GPIO_InitStructure);

   //引脚复用设置
   GPIO_PinAFConfig(GPIOA,GPIO_PinSource11,GPIO_AF_CAN1);
   GPIO_PinAFConfig(GPIOA,GPIO_PinSource12,GPIO_AF_CAN1);
	
   //CAN单元设置	
   CAN_InitStructrue.CAN_ABOM=ENABLE;//软件自动离线管理
   CAN_InitStructrue.CAN_AWUM=DISABLE;//睡眠模式通过软件唤醒
   CAN_InitStructrue.CAN_BS1=tbs1;//范围CAN_BS1_1tq ~CAN_BS1_16tq
   CAN_InitStructrue.CAN_BS2=tbs2;//范围:CAN_BS2_1tq~CAN_BS2_8tq;
   CAN_InitStructrue.CAN_Mode=mode;
   CAN_InitStructrue.CAN_NART=ENABLE;//禁止报文自动传送
   CAN_InitStructrue.CAN_Prescaler=brp;//分频系数(Fidv)为brp+1
   CAN_InitStructrue.CAN_RFLM=DISABLE;//报文不锁定，新的覆盖旧的
   CAN_InitStructrue.CAN_SJW=tsjw;//重新同步跳跃宽度(Tsjw)为tsjw+1个时间单位，tsjw范围:CAN_SJW_1tq~ CAN_SJW_4tq
   CAN_InitStructrue.CAN_TTCM=DISABLE;//非时间触发通信模式
   CAN_InitStructrue.CAN_TXFP=DISABLE;//优先级由报文标识符决定
   CAN_Init(CAN1,&CAN_InitStructrue);
   
   //配置过滤器
   CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;//使能过滤器
   CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_FilterFIFO0;//过滤器0关联到FIFO0
   CAN_FilterInitStructure.CAN_FilterIdHigh=0x201<<5;//32位ID
   CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
   CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x7FF<<5;//只匹配前面11位ID
   CAN_FilterInitStructure.CAN_FilterMaskIdLow=0|0x02;  //只接受数据帧
   CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;//标识符屏蔽位模式(另一个是标识符列表模式)
   CAN_FilterInitStructure.CAN_FilterNumber=0;//使能的是过滤器0
   CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;//32位mask
   CAN_FilterInit(&CAN_FilterInitStructure);
   
#if CAN1_RX0_INT_ENABLE
   CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);//FIFO0消息挂号中断允许
  
   NVIC_InitStructure.NVIC_IRQChannel=CAN1_RX0_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
   NVIC_Init(&NVIC_InitStructure);
#endif
  
   return 0;
}

#if CAN1_RX0_INT_ENABLE
void CAN1_RX0_IRQHandler(void)
{

 
  CAN_Receive(CAN1,CAN_FilterFIFO0,&RxMessage1);
  //for(i=0;i<8;i++)
  //printf("rxbuf[%d]:%d\r\n",i,RxMessage.Data[i]);
	
}
#endif

//can发送一组数据(固定格式:ID为0x12,标准帧，数据帧)
//len:数据长度(最大为8)
//msg:数据指针，最大8个字节
//返回值:0,成功;  其他,失败.
u8 CAN1_Send_Msg(u8*msg, u8 len)
{
   u8 mbox; //发送邮箱
   u16 i=0;
   CanTxMsg TxMessage;

	TxMessage.DLC=len;//要发送的数据帧长度(取值范围:0-0x8)
 //  TxMessage.ExtId=0x200;//设置拓展位标识符(29位)
   TxMessage.IDE=CAN_Id_Standard;     //标准帧 (拓展帧:CAN_Id_Extended)
   TxMessage.RTR=CAN_RTR_Data;     //数据帧 (远程帧:CAN_RTR_Remote)
   TxMessage.StdId=0x200;//标准标识符
   for(i=0;i<len;i++)
	TxMessage.Data[i]=msg[i];
   mbox=CAN_Transmit(CAN1,&TxMessage);//把TxMessage的信息通过CAN1发送出去,邮箱可通过函数返回值查询到
   i=0;
   while((CAN_TransmitStatus(CAN1,mbox)==CAN_TxStatus_Failed)&&(i<0xFFFF))
	   i++;
   if(i>=0xFFFF) return 1;
   return 0;
}


//can口接收数据查询
//buf:数据缓存区
//返回值:0,无数据被收到;   其他,接收到的数据长度
u8 CAN1_Receive_Msg(u8 *buf)
{
 	u32 i;
extern  CanRxMsg RxMessage1; 
    if (!RxMessage1.DLC)   return 0;		//没有接收到数据,直接退出 
    
    for(i=0;i<RxMessage1.DLC;i++)
    buf[i]=RxMessage1.Data[i];  
	return RxMessage1.DLC;
}







//CAN2初始化
//tsjw:重新同步跳跃时间单元.范围:CAN_SJW_1tq~ CAN_SJW_4tq
//tbs2:时间段2的时间单元.   范围:CAN_BS2_1tq~CAN_BS2_8tq;
//tbs1:时间段1的时间单元.   范围:CAN_BS1_1tq ~CAN_BS1_16tq
//brp :波特率分频器.范围:1~1024; tq=(brp)*tpclk1
//波特率=Fpclk1/((tbs1+1+tbs2+1+1)*brp);
//mode:CAN_Mode_Normal,普通模式;CAN_Mode_LoopBack,回环模式;
//Fpclk1的时钟在初始化的时候设置为42M,如果设置CAN1_Mode_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,CAN_Mode_LoopBack);
//则波特率为:42M/((6+7+1)*6)=500Kbps
//返回值:0,初始化OK;
//    其他,初始化失败; 

u8 Bsp_Can2_Init(u8 tsjw,u8 tbs2,u8 tbs1,u8 brp,u8 mode)
{
	
   GPIO_InitTypeDef GPIO_InitStructure;
   CAN_InitTypeDef CAN_InitStructrue;
   CAN_FilterInitTypeDef CAN_FilterInitStructure;
#if CAN2_RX1_INT_ENABLE
   NVIC_InitTypeDef NVIC_InitStructure;
#endif
	
   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2,ENABLE);
   
	
	
   GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;//复用功能
   GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;//推挽输出
   GPIO_InitStructure.GPIO_Pin=GPIO_Pin_12|GPIO_Pin_13;
   GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;//上拉
   GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
   GPIO_Init(GPIOB,&GPIO_InitStructure);

   //引脚复用设置
   GPIO_PinAFConfig(GPIOB,GPIO_PinSource12,GPIO_AF_CAN2);
   GPIO_PinAFConfig(GPIOB,GPIO_PinSource13,GPIO_AF_CAN2);
	
   //CAN单元设置	
   CAN_InitStructrue.CAN_ABOM=DISABLE;//软件自动离线管理
   CAN_InitStructrue.CAN_AWUM=DISABLE;//睡眠模式通过软件唤醒
   CAN_InitStructrue.CAN_BS1=tbs1;//范围CAN_BS1_1tq ~CAN_BS1_16tq
   CAN_InitStructrue.CAN_BS2=tbs2;//范围:CAN_BS2_1tq~CAN_BS2_8tq;
   CAN_InitStructrue.CAN_Mode=mode;
   CAN_InitStructrue.CAN_NART=ENABLE;//禁止报文自动传送
   CAN_InitStructrue.CAN_Prescaler=brp;//分频系数(Fidv)为brp+1
   CAN_InitStructrue.CAN_RFLM=DISABLE;//报文不锁定，新的覆盖旧的
   CAN_InitStructrue.CAN_SJW=tsjw;//重新同步跳跃宽度(Tsjw)为tsjw+1个时间单位，tsjw范围:CAN_SJW_1tq~ CAN_SJW_4tq
   CAN_InitStructrue.CAN_TTCM=DISABLE;//非时间触发通信模式
   CAN_InitStructrue.CAN_TXFP=DISABLE;//优先级由报文标识符决定
   CAN_Init(CAN2,&CAN_InitStructrue);
   
   //配置过滤器
   CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;//使能过滤器
   CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_FilterFIFO0;//过滤器14关联到FIFO1
   CAN_FilterInitStructure.CAN_FilterIdHigh=0x200<<5;      //接收ID为0x201的数据
   CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
   CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x7FF<<5;  //检验前11位ID
   CAN_FilterInitStructure.CAN_FilterMaskIdLow=0|0x02;     //只接收数据帧
   CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;//标识符屏蔽位模式(另一个是标识符列表模式)
   CAN_FilterInitStructure.CAN_FilterNumber=14;//使能的是过滤器14
   CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;//32位mask
   CAN_FilterInit(&CAN_FilterInitStructure);
   
#if CAN2_RX1_INT_ENABLE
   CAN_ITConfig(CAN2,CAN_IT_FMP0,ENABLE);//FIFO1消息挂号中断允许
  
   NVIC_InitStructure.NVIC_IRQChannel=CAN2_RX0_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
   NVIC_Init(&NVIC_InitStructure);
#endif
  
   return 0;
}

#if CAN2_RX1_INT_ENABLE
void CAN2_RX0_IRQHandler(void)
{

  //int i=0;
 CAN_Receive(CAN2,CAN_FilterFIFO0,&RxMessage2);
  //for(i=0;i<8;i++)
  //printf("rxbuf[%d]:%d\r\n",i,RxMessage.Data[i]);
	
}
#endif

//can发送一组数据(固定格式:ID为0x12,标准帧，数据帧)
//len:数据长度(最大为8)
//msg:数据指针，最大8个字节
//返回值:0,成功;  其他,失败.
u8 CAN2_Send_Msg(u8*msg, u8 len)
{
   u8 mbox; //发送邮箱
   u16 i=0;
   CanTxMsg TxMessage;

   TxMessage.DLC=len;//要发送的数据长度
//   TxMessage.ExtId=0x12;//设置拓展位标识符(29位)
   TxMessage.IDE=CAN_Id_Standard;     //标准帧
   TxMessage.RTR=CAN_RTR_Data;       //数据帧
   TxMessage.StdId=0x200;//标准标识符
   for( i=0;i<len;i++)
	TxMessage.Data[i]=msg[i];
   mbox=CAN_Transmit(CAN2,&TxMessage);//把TxMessage的信息通过CAN1发送出去,邮箱可通过函数返回值查询到
   i=0;
   while((CAN_TransmitStatus(CAN2,mbox)==CAN_TxStatus_Failed)&&(i<0xFFFF))
	   i++;
   if(i>=0xFFFF) return 1;
   return 0;
}


//can口接收数据查询
//buf:数据缓存区
//返回值:0,无数据被收到;   其他,接收到的数据长度
u8 CAN2_Receive_Msg(u8 *buf)
{
 	u32 i;
 extern  CanRxMsg RxMessage2; 
    if (!RxMessage2.DLC) return 0;		//没有接收到数据,直接退出 

    for(i=0;i<RxMessage2.DLC;i++)
    buf[i]=RxMessage2.Data[i];  
	return RxMessage2.DLC;
}

