/*
 * usart.c
 * 
 * 
 */

#include "usart.h"
#include "tle5012b.h"

/*
#define H_ENABLE_STATUS   "Enable =%d \r\n"
#define L_ENABLE_STATUS   "!Enable =%d \r\n"
#define CCW_DIR           "Dir =%d, CCW\r\n"
#define CW_DIR            "Dir =%d, CW\r\n"
*/

#define BUFFER_TX_SIZE      64              // Transmit buffer size
#define BUFFER_RX_SIZE      64              // Receive buffer size


volatile uint8_t rxBuffer[BUFFER_RX_SIZE];           // Software RX buffer
volatile uint8_t rxbIndexIn;                         // Index in rxBuffer where the next byte will be stored
volatile uint8_t rxbIndexOut;                        // Index in rxBuffer from where the next byte will be transmitted
volatile bool rxbFull;                               // True if receive buffer is full
volatile bool rxbEmpty;                              // True if receive buffer is empty
volatile uint8_t txBuffer[BUFFER_TX_SIZE];           // Software TX buffer
volatile uint8_t txbIndexIn;                         // Index in txBuffer where the next byte will be stored
volatile uint8_t txbIndexOut;                        // Index in txBuffer from where the next byte will be transmitted
volatile bool txbFull;                               // True if transmit buffer is full
volatile bool txbEmpty;                              // True if transmit buffer is empty


//char Charbuff[16];
//                
//struct __FILE 
//{ 
//	int handle; 
	/* Whatever you require here. If the only file you are using is */ 
	/* standard output using printf() for debugging, no file handling */ 
	/* is required. */ 
//}; 

/* FILE is typedef’ d in stdio.h. */ 
/*
FILE __stdout;       
//  
void _sys_exit(int x) 
{ 
	x = x; 
} 
*/
//STDIO.H
/*
int fputc(int c,FILE *stream) 
{	   
  LL_USART_TransmitData8(USART1,c);
  while(!LL_USART_IsActiveFlag_TXE(USART1)) ;//__NOP();

//  USART1->TDR=(uint8_t)c;
//  LL_USART_ClearFlag_TC(USART1); 
  return c;
}
*/
////STDIO.H
//int fgetc(FILE *stream) 
//{
//  while(!LL_USART_IsActiveFlag_RXNE(USART1)) __NOP();
//  return ((char)LL_USART_ReceiveData8(USART1));
//}

void MX_USART1_UART_Init(void)
{

    rxbIndexIn = 0;
    rxbIndexOut = 0;
    rxbFull = false;
    rxbEmpty = true;

    txbIndexIn = 0;
    txbIndexOut = 0;
    txbFull = false;
    txbEmpty = true;
    
  LL_USART_InitTypeDef USART_InitStruct;
  LL_GPIO_InitTypeDef GPIO_InitStruct;
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1);
  
  /**USART1 GPIO Configuration  
  PA9   ------> USART1_TX
  PA10   ------> USART1_RX 
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  USART_InitStruct.BaudRate = 38400;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART1, &USART_InitStruct);

  LL_USART_DisableIT_CTS(USART1);
  LL_USART_DisableOverrunDetect(USART1);
  LL_USART_ConfigAsyncMode(USART1);
  
  //NVIC_SetPriority(USART1_IRQn,4);  
  
  //LL_USART_EnableIT_PE(USART1);                         // Enable Parity Error - Is dit regtig nodig
  LL_USART_EnableIT_RXNE(USART1);                       // Enable Receive register not empty interrupt
  //LL_USART_EnableIT_IDLE(USART1);                       // Enable IDLE line interrupt (end of transmission)
  //LL_USART_EnableIT_TXE(USART1);                        // Enable Transmit data register empty interrupt

  LL_USART_Enable(USART1);
  NVIC_EnableIRQ(USART1_IRQn);                          //
                                        
}


// Write one byte to the transmit buffer. Returns false if buffer is full.
bool UART1_WriteByte(uint8_t data)
{
    // Add bytes to software TX buffer for writing when the
    // hardware buffer is empty.

    if (txbFull)
        return false;

    txBuffer[txbIndexIn] = data;
    txbIndexIn++;

    if (txbIndexIn >= BUFFER_TX_SIZE)
        txbIndexIn = 0;
    
    txbEmpty = false;

    if (txbIndexIn == txbIndexOut)
        txbFull = true;

    // Enable TX data register empty interrupt, which will fire
    // almost immediatedly if no bytes are loaded in the hardware
    // register. 
    LL_USART_EnableIT_TXE(USART1);

    return true;
}


uint8_t UART1_Write(uint8_t *data, uint8_t len)
{
    if ((txbFull) | (len == 0))
        return 0;

    int i;
    for (i = 0; i < len; i++)
    {
        txBuffer[txbIndexIn] = data[i];
        txbIndexIn++;

        if (txbIndexIn >= BUFFER_TX_SIZE)
            txbIndexIn = 0;

        if (txbIndexIn == txbIndexOut)
        {
            txbFull = true;
            break;
        }
    }

    txbEmpty = false;
    LL_USART_EnableIT_TXE(USART1);

    return i;
}


// Returns the number of bytes in the receive buffer
uint8_t UART1_BytesToRead()
{
    if (rxbFull)
		return BUFFER_RX_SIZE;
		
	if (rxbIndexIn >= rxbIndexOut)
		return rxbIndexIn - rxbIndexOut;
	else
		return (BUFFER_RX_SIZE - rxbIndexOut) + rxbIndexIn;
}


// Read one byte from the receive buffer
uint8_t UART1_Read()
{
    uint8_t data = 0;

    if (!rxbEmpty)
    {
        data = rxBuffer[rxbIndexOut];
        rxbIndexOut++;

        if (rxbIndexOut >= BUFFER_RX_SIZE)
            rxbIndexOut = 0;

        rxbFull = false;

        if (rxbIndexOut == rxbIndexIn)
            rxbEmpty = true;
    }

    return data;
}


// Register the callback function for received 
/*
void RegisterByteReceived(byteReceivedCallbackType callback)
{
    receivedByte = callback;
}
*/

// Word op die oomblik uit n infinite loop in main.c geroep
/*
void usart_Receive_Process(void)
{
    static uint8_t t;
    uint16_t angle;
//    uint32_t Temporary_val=0;
    uint16_t CRC_val=0;
    if(Communications_Process_flag){
        Communications_Process_flag=0;
        if(Uart_CRC_flag ){                                   //   
            Uart_CRC_flag=0;
            for(t=2;t<Rx1_temp_num-2;t++){//8-2
                CRC_val+=Rx1_buff[t];
                if(t==Rx1_temp_num-3) CRC_val=CRC_val & 0xff;//
            } 
            if(Rx1_buff[t]==CRC_val){
                Urat_CRC_Correct_flag=1;                    //
                // OK
                UART1_SendStr("OK\r\n");
            }
            else{
//                Uart_CRC_Error_flag=1;                      //
                //
                UART1_SendStr("CRC verify err\r\n");
            }
        }
        if(Urat_CRC_Correct_flag){               //
            Urat_CRC_Correct_flag=0;
            if(Rx1_buff[2] ==0x05){                 //
                value_Temp = Rx1_buff[4];
                value_Temp<<=8;
                value_Temp|= Rx1_buff[5];
                if(Rx1_buff[3]>=0xA0 && Rx1_buff[3]<=0xBF){
                    switch(Rx1_buff[3]){         //
                        case 0xA0:
                                kp = value_Temp;
                                table1[11]=kp;
                                //printf("new kp = %d\r\n",kp);
                                UART1_SendStr("new kp =");
                                Uart_Data_Conversion(kp,3);
                                UART1_SendStr(Charbuff);
                                UART1_SendStr("\r\n");    
                        break;
                        case 0xA1:
                                ki = value_Temp;
                                table1[12]=ki;
                               // printf("new ki = %d\r\n",ki);
                                UART1_SendStr("new ki =");
                                Uart_Data_Conversion(ki,2);
                                UART1_SendStr(Charbuff);
                                UART1_SendStr("\r\n");
                            break;
                        case 0xA2: kd = value_Temp;
                                table1[13]=kd;
                              //  printf("new kd = %d\r\n",kd);
                                UART1_SendStr("new kd =");
                                Uart_Data_Conversion(kd,3);
                                UART1_SendStr(Charbuff);
                                UART1_SendStr("\r\n");
                            break;
                        case 0xA3:      //set Current 
                                Currents = (value_Temp/100)<<3;
                                Menu_Num2_item = Currents>>3;
                        
                                table1[1]=Currents;
                                table1[2]=Menu_Num2_item;
                            break;                                      //
                        case 0xA4:
                                PID_Cal_value_init();                   
                                stepangle = 64/value_Temp;
                                //stepangle =Microstep_Set;              //
                                table1[3]=stepangle;                    //
                                switch(stepangle & 0x3e){
                                    case 0x02:Menu_Num3_item =4;  break;
                                    case 0x04:Menu_Num3_item =3;  break;
                                    case 0x08:Menu_Num3_item =2;  break;
                                    case 0x10:Menu_Num3_item =1;  break;
                                    case 0x20:Menu_Num3_item =0;  break;
                                    default: Menu_Num3_item =3; break;
                                }
                                table1[4]=Menu_Num3_item;
                            break;
                        case 0xA5:
                                PID_Cal_value_init();                               //
                                if(0xaa == value_Temp){
                                    Motor_ENmode_flag=0;Menu_Num4_item=1;           //
                                }else if(0x55 == value_Temp) {
                                    Motor_ENmode_flag=1;Menu_Num4_item=0;
                                }
                                table1[5]=Motor_ENmode_flag;                        //
                                table1[6]=Menu_Num4_item;
                            break; 
                        case 0xA6:                                                  //
                                if(0x11 == value_Temp){
                                    Motor_Dir=1;Menu_Num5_item=0;                   //
                                }else if(0x22 == value_Temp) {
                                    Motor_Dir=0;Menu_Num5_item=1;
                                }
                                table1[7]=Motor_Dir;                                //
                                table1[8]=Menu_Num5_item;
                            break; 
                        case 0xB0: if((Rx1_buff[4]==0xaa) && (Rx1_buff[5]== 0xaa)){                     //
                                    t=1;
                                    UART1_SendStr("P ---- kp =");
                                    Uart_Data_Conversion(kp,3);
                                    UART1_SendStr(Charbuff);
                                    UART1_SendStr("\r\n");
                                    UART1_SendStr("I ---- ki =");
                                    Uart_Data_Conversion(ki,2);
                                    UART1_SendStr(Charbuff);
                                    UART1_SendStr("\r\n");
                                    UART1_SendStr("D ---- kd =");
                                    Uart_Data_Conversion(kd,3);
                                    UART1_SendStr(Charbuff);
                                    UART1_SendStr("\r\n");
                                     //printf("p ---- kp =%d\r\n",kp);
                                    //printf("i ---- ki =%d\r\n",ki);
                                    // printf("d ---- kd =%d\r\n",kd);
                                }else{
                                    UART1_SendStr("Read PID err\r\n");
                                }
                            break;
                        case 0xB1: if((Rx1_buff[4]==0xaa) && (Rx1_buff[5]== 0xaa)){                     //
                                    t=1;
                                    //printf("Currents =%d mA\r\n",(Currents>>3)*100);
                                    UART1_SendStr("Currents =");
                                    Uart_Data_Conversion((Currents>>3)*100,4);
                                    UART1_SendStr(Charbuff);
                                    UART1_SendStr(" mA\r\n");
                                }else{
                                    UART1_SendStr("Read Current err\r\n");
                                }
                            break;
                        case 0xB2: if((Rx1_buff[4]==0xaa) && (Rx1_buff[5]== 0xaa)){                     //
                                    t=1;
                                   // printf("Microstep =%d -->%d Gear\r\n",64/stepangle,16384/stepangle);
                                    UART1_SendStr("Microstep =");
                                    Uart_Data_Conversion(64/stepangle,2);
                                    UART1_SendStr(Charbuff);
                                    UART1_SendStr("-->");
                                    Uart_Data_Conversion(16384/stepangle,4);
                                    UART1_SendStr(Charbuff);
                                    UART1_SendStr(" Gear\r\n");
                                }else{
                                    UART1_SendStr("Read Microstep err\r\n");
                                }
                            break;
                        case 0xB3: if((Rx1_buff[4]==0xaa) && (Rx1_buff[5]== 0xaa)){                     //
                                    t=1;
                                    if(1 == Motor_ENmode_flag){
                                       // printf(H_ENABLE_STATUS,Motor_ENmode_flag);
                                       UART1_SendStr("Enable =");
                                       Uart_Data_Conversion(Motor_ENmode_flag,1);
                                       UART1_SendStr(Charbuff);
                                       UART1_SendStr("\r\n");
                                    }
                                    if (0== Motor_ENmode_flag)
                                    {
                                       UART1_SendStr("!Enable =");
                                       Uart_Data_Conversion(Motor_ENmode_flag,1);
                                       UART1_SendStr(Charbuff);
                                       UART1_SendStr("\r\n");
                                        //printf(L_ENABLE_STATUS,Motor_ENmode_flag);
                                    }
                                }else{
                                    UART1_SendStr("Read enable err\r\n");
                                }
                            break;
                                
                        case 0xB4: if((Rx1_buff[4]==0xaa) && (Rx1_buff[5]== 0xaa)){                     //
                                    t=1;
                                    if(0 == Motor_Dir){
                                        UART1_SendStr("Dir =");
                                       Uart_Data_Conversion(Motor_Dir,1);
                                       UART1_SendStr(Charbuff);
                                       UART1_SendStr(", CCW");
                                       UART1_SendStr("\r\n");
                                        //printf(CCW_DIR,Motor_Dir);
                                    }
                                    if(1 == Motor_Dir){
                                        UART1_SendStr("Dir =");
                                       Uart_Data_Conversion(Motor_Dir,1);
                                       UART1_SendStr(Charbuff);
                                       UART1_SendStr(", CW");
                                       UART1_SendStr("\r\n");
                                        //printf(CW_DIR,Motor_Dir);
                                    }
                                }else{
                                    UART1_SendStr("Read Dir err\r\n");
                                }
                            break;
                        
                        case 0xBA:
                            // Custom instruction (Read Sensor Angle)
                            angle = ReadAngle();
                            angle = (uint16_t)((360.0 / 16384.0) * (float)angle);   // Eindlik deel met 2^15 maar ReadAngle deel reeds met 2
                            UART1_SendStr("Angle =");
                            Uart_Data_Conversion(angle,5);
                            UART1_SendStr(Charbuff);
                            UART1_SendStr("\r\n");
                            break;

                        default: UART1_SendStr("Function Code Undefined\r\n");break;
                    }
                }else{
                    UART1_SendStr("Function Code err\r\n");
                }
                //
                if(t!=1){
                    flash_store_flag=1;
                    //ART1_SendStr(Charbuff);
                    //UART1_SendStr("\r\n");
                }
            }
            else{
                ////
                UART1_SendStr("Data Length err\r\n");
            }
        }
        NVIC_EnableIRQ(USART1_IRQn); //
    }
}
*/
/*
void Uart_Data_Conversion(int16_t Conversion_value,uint8_t num)
{
    //uint8_t n=0;

    switch (num){
        case 1:Charbuff[0] = Conversion_value%10+'0';
               Charbuff[1] ='\0'; Charbuff[2] ='\0'; Charbuff[3] ='\0'; Charbuff[4] ='\0'; 
        break;
        case 2:Charbuff[0] = Conversion_value/10+'0';
               Charbuff[1] = Conversion_value%10+'0';
               Charbuff[2] ='\0'; Charbuff[3] ='\0'; Charbuff[4] ='\0'; 
        break;
        case 3:Charbuff[0] = Conversion_value/100+'0';
               Charbuff[1] = Conversion_value%100/10+'0';
               Charbuff[2] = Conversion_value%10+'0';
               Charbuff[3] ='\0'; Charbuff[4] ='\0'; 
        break;
        case 4:Charbuff[0] = Conversion_value/1000+'0';
               Charbuff[1] = Conversion_value%1000/100+'0';
               Charbuff[2] = Conversion_value%100/10+'0';
               Charbuff[3] = Conversion_value%10+'0';
               Charbuff[4] ='\0'; 
        break;
        case 5:Charbuff[0] = Conversion_value/10000+'0';//65536
               Charbuff[1] = Conversion_value%10000/1000+'0';
               Charbuff[2] = Conversion_value%1000/100+'0';
               Charbuff[3] = Conversion_value%100/10+'0';
               Charbuff[4] = Conversion_value%10+'0';
        break;
    }
    
}
*/
/*********************************************/
// Lyk soos n blocking method? Dalk verbeter...
/*
void UART1_SendByte(uint8_t data)
{
    LL_USART_TransmitData8(USART1, data);

    while (LL_USART_IsActiveFlag_TXE(USART1) == RESET);
}
*/
/******************************************************
******************************************************************************/
/*
uint8_t UART1_ReceiveByte(void)
{
    uint8_t UART1_RX_BUF; 
     
    while (LL_USART_IsActiveFlag_RXNE(USART1) == RESET);
    UART1_RX_BUF = LL_USART_ReceiveData8(USART1);
    return  UART1_RX_BUF;
}
*/
/*******************************************************************************

******************************************************************************/
/*
void UART1_SendStr(char *str)
{
    while(*str != '\0')
    {
        UART1_SendByte(*str++); 
    }   
}
*/
//#if 0
////
//void SerialCheck(void) 
//{        
//  printf("----- HyperStepper 20180829 -----\r\n");
//  uint8_t quit=0;
//  char command;
//  while(!quit)
//  {
//    printf("Edit PID gains:\r\n");
//    printf("p ----- kp =%d\r\n",kp);
//    printf("i ----- ki =%d\r\n",ki);
//    printf("d ----- kd =%d\r\n",kd);
//    printf("q ----- quit\r\n");
//	printf("Please input your command:\r\n"); 
//	scanf("%c",&command);
//	printf("Your input command is:%c\r\n",command);
//    switch(command) 
//	{
//      case 'p':
//      {
//        printf("kp = ?\r\n");
//		scanf("%hd",&kp);
//        printf("new kp = %d\r\n",kp);
//      }
//      break;
//      case 'i':
//      {
//        printf("ki = ?\r\n");
//		scanf("%hd",&ki);
//		printf("new ki = %d\r\n",ki);
//      }
//      break;
//      case 'd':
//      {
//        printf("kd = ?\r\n");
//		scanf("%hd",&kd);
//        printf("new kd = %d\r\n",kd);
//      }
//      break;
//      case 'q':
//      {  
//        quit=1;
//        printf("done...\r\n");
//      }
//      default:
////      {quit=1;}
//      break;
//    }
//  }
//}
//#endif

// USART1 Interrupt Service Handler
void USART1_IRQHandler(void)
{
    // Test for received bytes    
    if((USART1->ISR & USART_ISR_RXNE) == USART_ISR_RXNE)
    {
        // New byte has been received. Interrupt flag is cleared by reading the register.
        uint8_t data = USART1->RDR;
        
        if (!rxbFull)
        {
            rxBuffer[rxbIndexIn] = data;
            rxbIndexIn++;

            if (rxbIndexIn >= BUFFER_RX_SIZE)
                rxbIndexIn = 0;
    
            rxbEmpty = false;

            if (rxbIndexIn == rxbIndexOut)
                rxbFull = true;
        }
    }
    // Test for idle line (end of transmission)
    else if(LL_USART_IsActiveFlag_IDLE(USART1) == 1)
    {
        // The idle line interrupt is generated when the line goes to idle
        // which typically means the end of transmission. This could be a
        // good time to perform processing of the received bytes.
        // Currently not used anymore.

        LL_USART_ClearFlag_IDLE(USART1);
    } 
    // Test if new byte can be send
    else if ((USART1->ISR & USART_ISR_TXE) == USART_ISR_TXE)
    {
        // Interrupt is generated when the transmit data register is empty.
        // Now would be a good time to load the next byte that needs to be
        // transmitted. Flag is cleared by writing to the data register. 

        // Check if the software buffer has bytes to send and if so loads
        // the next byte. 
        if (!txbEmpty)
        {
            USART1->TDR = txBuffer[txbIndexOut];
            txbIndexOut++;

            if (txbIndexOut >= BUFFER_TX_SIZE)
                txbIndexOut = 0;

            txbFull = false;

            if (txbIndexOut == txbIndexIn)
                txbEmpty = true;
        }
        else
        {
            // No more bytes in software buffer so disable the interrupt
            LL_USART_DisableIT_TXE(USART1);
        }
    }
}

void PID_Cal_value_init(void)
{
    s       = 0;            // TIM1 counts which is step counts
    s_1     = 0;            // Previous step count
    
    s_sum   = 0;            // Step sum offset
    r       = 0;            // Desired angle
    r_1     = 0;            // Previous desired angle
    y       = 0;            // Output theta
    y_1     = 0;            // Previous output theta
    yw      = 0;
    pid_e   = 0;
    u       = 0;
    dterm   = 0;
    wrap_count=0;
    //LL_TIM_SetCounter(TIM1,0);
    WRITE_REG(TIM1->CNT, 0);
    
    r=*(volatile uint16_t*)((ReadValue(READ_ANGLE_VALUE)>>1)*2+0x08008000); //
    s_sum=r;   //
    y=r;
    y_1=y;
    yw=y;  
    yw_1=yw;
    
}




















