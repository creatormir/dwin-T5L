#include "sys.h"
#include <intrins.h>


u16 count=0;
static u16 SysTick=0;


 /**
    * @brief Software ms delay / Программная задержка мс
    *   Программная задержка Период колебаний T=1/206438400
    * @param [in] 
    *      n: задержка мс
    * @param [out]
    *      none
    * @retval 
    *      none
    */
void delays_ms(u16 n)
{
    int i,j;
    for(i=0;i<n;i++)
        for(j=0;j<17203;j++);    
}



 /**
    * @brief INIT CPU
    *   Инициализировать периферийные устройства ОС, включая инициализацию UART2, UART3, 
    *   Close the door dog / закрытие дверной собачки, мощность привода порта ввода-вывода +/-8 мА
    *   UART4, UART5, таймер T0 T1 T2, включен только UART2/UART3, остальные периферийные устройства не включены
    * @param  none
    *
    *   
    * @retval   none
    * 
    */  
void INIT_CPU(void)
{
    EA=0;
    RS0=0;
    RS1=0;
    //SP=0xC0;

    CKCON=0x00;
    T2CON=0x70;
    DPC=0x00;
    PAGESEL=0x01;
    D_PAGESEL=0x02;   //DATA RAM  0x8000-0xFFFF
    MUX_SEL=0x60;   //UART2, UART2 on, WDT off
    RAMMODE=0x00;
    PORTDRV=0x01;   //Drive Strength +/-8mA
    IEN0=0x00;      //turn off all interrupts   /   отключить все прерывания
    IEN1=0x00;
    IEN2=0x00;
    IP0=0x00;      //Interrupt priority default /   Приоритет прерывания по умолчанию
    IP1=0x00;

    WDT_OFF();      //close the door dog    /   закрыть дверь собака

    //IO port configuration     /   Конфигурация порта ввода-вывода
    P0=0x00;
    P1=0x00;
    P2=0x00;
    P3=0x00;
    P0MDOUT=0x50;
    P1MDOUT=0xFF;
    P2MDOUT=0x00; 
    P3MDOUT=0x0F;

    //Конфигурация UART2 8N1  15200
    ADCON=0x80;
    SCON0=0x50;
    SREL0H=0x03;        //FCLK/64*(1024-SREL1)
    SREL0L=0xE4;

    //Конфигурация UART3 8N1   115200
    SCON1=0x50;
    SREL1H=0x03;       //FCLK/64*(1024-SREL1)
    SREL1L=0xC8;

    //Конфигурация UART4 8N1      115200
    SCON2T=0x80;
    SCON2R=0x80;
    BODE2_DIV_H=0x00;     //FCLK/8*DIV
    BODE2_DIV_L=0xE0;

    //Конфигурация UART5 8N1      115200
    SCON3T=0x80;
    SCON3R=0x80;
    BODE3_DIV_H=0x00;       //FCLK/8*DIV
    BODE3_DIV_L=0xE0;

    
    TMOD=0x11;          //16-bit timer  /   16-битный таймер
    //T0
    TH0=0x00;
    TL0=0x00;
    TR0=0x00;

    //T1
    TH1=0x00;
    TL1=0x00;
    TR1=0x00;
    
    TCON=0x05;      //External interrupt falling edge trigger   /   Триггер заднего фронта внешнего прерывания

    //T2  Autoload mode    /   Режим автозагрузки T2
    T2CON=0x70;
    TH2=0x00;
    TL2=0x00;
    TRL2H=0xBC;
    TRL2L=0xCD;        //1ms timer

}

 /**
    * @brief Инициализация таймера T0
    *        Инициализировать таймер T0, временной интервал равен 1 мс.
    * @param [in] 
    *      none
    * @param [out]
    *      none
    * @retval 
    *      none
    */
void T0_Init(void)
{
    TMOD|=0x01;
    TH0=T1MS>>8;        //таймер 1 мс
    TL0=T1MS;
    ET0=1;              //Enable timer 0 interrupt  /   Включить прерывание таймера 0
    EA=1;               //open total interrupt  /   открытое полное прерывание
    TR0=1;              //запуск таймера
}

 /**
    * @brief Timer T2 initialization    /   Инициализация таймера T2
    *        Initialize timer T2, the timing interval is 1ms    /   Инициализировать таймер T2, временной интервал равен 1 мс.
    * @param [in] 
    *      none
    * @param [out]
    *      none
    * @retval 
    *      none
    */
void T2_Init(void)
{
    T2CON=0x70;
    TH2=0x00;
    TL2=0x00;
    TRL2H=0xBC;
    TRL2L=0xCD;        //1ms的定时器
    IEN0|=0x20;     //开启定时器2
    TR2=0x01;
}


 /**
    * @brief Read DGUS II VP data   /   Чтение данных DGUS II VP
    *        Чтение и запись должны занимать 4 байта, потому что длина len для чтения и записи указана в словах,
    *        Таким образом, чтение и запись данных разделены на 4 разные ситуации: ситуация другая, и чтение и запись разные.
    * @param [in] 
    *      addr:адрес начала VP
    *      len: Чтение длины слова VP
    * @param [out]
    *      buf:Прочитанные данные сохраняются, а длина байта buf равна len*2.
    * @retval 
    *      none
    */  
void read_dgusii_vp(int addr,char* buf,int len)
{
    int OS_addr=0,OS_addr_offset=0;
    int OS_len=0,OS_len_offset=0;
    EA=0;
    OS_addr=addr/2;
    OS_addr_offset=addr%2;
    ADR_H=(u8)(OS_addr>>16)&0xFF;
    ADR_M=(u8)(OS_addr>>8)&0xFF;
    ADR_L=(u8)OS_addr&0xFF;
    ADR_INC=1;                 //Адрес памяти ОС DGUS автоматически увеличивается на 1 после чтения и записи.
    RAMMODE=0xAF;               //Запустить режим чтения
    if(OS_addr_offset==1)       //Первый адрес имеет смещение, исправьте его
    {
        while(APP_ACK==0);      //ждать
        APP_EN=1;
        while(APP_EN==1); 
        *buf++=DATA1;
        *buf++=DATA0;              
        len--;
        OS_addr_offset=0;
    }
    OS_len=len/2;
    OS_len_offset=len%2;
    if(OS_len_offset==1)
    {
         OS_len++;
    }
    while(OS_len--)
    {
        if((OS_len_offset==1)&&(OS_len==0))
        {           
            while(APP_ACK==0);
            APP_EN=1;
            while(APP_EN==1);       //Чтение и запись выполнения завершаются
            *buf++=DATA3;
            *buf++=DATA2;           
            break;    
        } 
        while(APP_ACK==0);
        APP_EN=1;
        while(APP_EN==1);       //Чтение и запись выполнения завершаются 
        *buf++=DATA3;
        *buf++=DATA2;
        *buf++=DATA1;
        *buf++=DATA0;                          
    }   
    RAMMODE=0x00;           //RAMMODE должен быть установлен на ноль после завершения чтения и записи.
    EA=1;         
}


 /**
    * @brief Запись данных DGUS II VP
    *        Чтение и запись должны занимать 4 байта, потому что длина len для чтения и записи указана в словах,
    *        Таким образом, чтение и запись данных разделены на 4 разные ситуации: ситуация другая, и чтение и запись разные.
    * @param [in] 
    *      addr:первый адрес VP
    *      len: Напишите длину слова VP
    * @param [out]
    *      buf:Прочитанные данные сохраняются, а длина байта buf равна len*2.
    * @retval 
    *      none
    */ 
void write_dgusii_vp(int addr,u8* buf,int len)
{
    int OS_addr=0,OS_addr_offset=0;
    int OS_len=0,OS_len_offset=0;
    EA=0;
    OS_addr=addr/2;
    OS_addr_offset=addr%2; 
    ADR_H=(u8)(OS_addr>>16)&0xFF;
    ADR_M=(u8)(OS_addr>>8)&0xFF;
    ADR_L=(u8)OS_addr&0xFF;
    ADR_INC=0x01;                 //Адрес памяти ОС DGUS автоматически увеличивается на 1 после чтения и записи.
    RAMMODE=0x8F;               //Запустить режим записи
    if(OS_addr_offset==1)
    {
        ADR_INC=0;                     
        RAMMODE=0xAF;
        while(APP_ACK==0);
        APP_EN=1;
        while(APP_EN==1);       //Чтение и запись выполнения завершаются
        ADR_INC=0x01; 
        RAMMODE=0x8F;
        while(APP_ACK==0);      
        DATA1=*buf++;
        DATA0=*buf++;
        APP_EN=1;
        while(APP_EN==1);       //Чтение и запись выполнения завершаются
        len--;
        OS_addr_offset=0;
    }
    OS_len=len/2;
    OS_len_offset=len%2; 
    if(OS_len_offset==1)
    {
         OS_len++;
    } 
    while(OS_len--)
    {
        if((OS_len_offset==1)&&(OS_len==0))
        {
            ADR_INC=0;
            RAMMODE=0xAF;
            while(APP_ACK==0);
            APP_EN=1;                //Добавьте процесс чтения, чтобы избежать записи 0 в другое слово при написании слова
            while(APP_EN==1);       //Чтение и запись выполнения завершаются
            ADR_INC=0x01;
            RAMMODE=0x8F;
            while(APP_ACK==0);           
            DATA3=*buf++;
            DATA2=*buf++;
            APP_EN=1;
            while(APP_EN==1);       //Чтение и запись выполнения завершаются
            break;
        }
        while(APP_ACK==0);        
        DATA3=*buf++;
        DATA2=*buf++;
        DATA1=*buf++;
        DATA0=*buf++;
        APP_EN=1;
        while(APP_EN==1);       //Чтение и запись выполнения завершаются
    } 
    RAMMODE=0x00;       //RAMMODE должен быть установлен на ноль после завершения чтения и записи.
    EA=1;  
}  




 /**
    * @brief Функция обработчика прерывания, номер прерывания определен, изменять его не требуется.
    *        using n: Не нужно, так как обработка ЦП ОС и так очень быстрая, постарайтесь не прерывать вложение
    *        Он был изменен на время функции интерфейса.Необходимо ли увеличить прерывание n, это необходимо подтвердить.
    *        После того, как плата есть, ее можно проверить.
    * @param [in] 
    *      none
    * @param [out]
    *      none
    * @retval 
    *      none
    */ 
void EX0_ISR_PC(void)    interrupt 0
{
    EA=0;
    //code
    EA=1;
}


 /**
    * @brief Обработчик прерывания таймера T0
    *        Определяемые пользователем
    * @param [in] 
    *      none
    * @param [out]
    *      none
    * @retval 
    *      none
    */
void T0_ISR_PC(void)    interrupt 1
{
    static u16 i=0;
    EA=0;
    TH0=T1MS>>8;
    TL0=T1MS;
    i++;
    if(i>=1000)
    {
        i=0;
        P1_0=!P1_0;        
        count++; 
    }          
    EA=1;
}

void EX1_ISR_PC(void)    interrupt 2
{
    EA=0;
    //code
    EA=1;
}

void T1_ISR_PC(void)    interrupt 3
{
    EA=0;
    //code
    EA=1;
}

void UART2_ISR_PC(void)    interrupt 4
{
    EA=0;
    //code
    EA=1;
}


 /**
    * @brief Задержка таймера T2
    *   Задержка таймера T2, точная задержка
    * @param [in] 
    *      n:задержка мс
    * @param [out]
    *      none
    * @retval 
    *      none
    */
void delay_ms(u16 n)
{
    SysTick=n;
    while(SysTick)
    {
        if(ET2==0)
        {   
            SysTick--;
            ET2=1;
        }
    }    
}

void T2_ISR_PC(void)    interrupt 5
{
    TF2=0;    
    SysTick--;    
}


void CAN_ISR_PC(void)    interrupt 9
{
    EA=0;
    //code
    EA=1;
}

void UART4_TX_ISR_PC(void)    interrupt 10
{
    EA=0;
    //code
    EA=1;
}

void UART4_RX_ISR_PC(void)    interrupt 11
{
    EA=0;
    //code
    EA=1;
}

void UART5_TX_ISR_PC(void)    interrupt 12
{
    EA=0;
    //code
    EA=1;
}

void UART5_RX_ISR_PC(void)    interrupt 13
{
    EA=0;
    //code
    EA=1;
}

void UART3_ISR_PC(void)    interrupt 16
{
    EA=0;
    //code
    EA=1;
}

