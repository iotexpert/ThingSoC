#include "project.h"
#include <stdio.h>

#define I2CEXPANDER 0x73
#define USBUART_BUFFER_SIZE 64

uint8 buffer[USBUART_BUFFER_SIZE];

/* The printBuff function prints out the buff to the USBUART
*/
void printBuff(char *buff)
{
    while (0u == USBUART_CDCIsReady()); /* Wait until component is ready to send data to host. */
    USBUART_PutData((uint8 *)buff, strlen(buff)); /* Send data back to host. */
}

/* 
 * readControlReg - this function read the PCA9546A Control Register and returns the value
 * it will return 0xFF (which is illegal) if the PCA9546A cannot be found
 */
uint8_t readControlReg(uint8 address)
{

    uint8 byte=0xFF;
    if (I2C_I2C_MSTR_NO_ERROR == I2C_I2CMasterSendStart(address,I2C_I2C_READ_XFER_MODE))
    {
        byte=I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA);
        I2C_I2CMasterSendStop();
    }
    return byte;
}

/* The printControlReg function reads the PCA9456A control register at I2C "address" 
 * then prints the value to the USB->UART
 */
void printControlReg(uint8 address)
{
    char buffer[32];
    uint8 val = readControlReg(address);
    sprintf((char *)buffer,"I2C Expander Control Reg = %x\n",val);
    printBuff((char *)buffer);
}

/* The setPort function write the PCA9456A control register at I2C "address" to enable "port" 
 * this function disables the other 3 ports
 */
void setPort(uint8 address,int port)
{ 
    uint8 mask = 0x1<<(port-1);
    if(port == 0) // if the port is set to 0... disable all ports
        mask = 0;
    
    if (I2C_I2C_MSTR_NO_ERROR == I2C_I2CMasterSendStart(address,I2C_I2C_WRITE_XFER_MODE))
    {
        I2C_I2CMasterWriteByte(mask);
        I2C_I2CMasterSendStop();
    }
}

/* The i2c_detect function probes the I2C bus from address 0x0 --> 0x7F with an I2C Write.
*  If there is an ACK it prints the address otherwise it prints --
*/
void i2c_detect()
{
    char buff[16];
    int high,low;
    
    printBuff("\r      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
    
    for(high=0;high<=0x70;high=high+0x10) // Iterate thrugh high nibble
    {
        sprintf(buff,"%2X: ",high);
        printBuff(buff);
        
        for(low = 0 ; low<=0x0F;low++) // Iterate through low nibble
        {
            if (I2C_I2C_MSTR_NO_ERROR == I2C_I2CMasterSendStart((high|low),I2C_I2C_WRITE_XFER_MODE)) 
            {
                sprintf(buff," %2X",high|low); // You got ACK so print address
                printBuff(buff);
            }
            else
                printBuff(" --"); // No ACK so print --
          
            I2C_I2CMasterSendStop();
        }
        printBuff("\n");
    }
}

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    uint16 count;
    USBUART_Start(0, USBUART_5V_OPERATION); 
    I2C_Start(); 
    for(;;)
    {
        if (0u != USBUART_IsConfigurationChanged())
        {
            if (0u != USBUART_GetConfiguration()) /* Initialize IN endpoints when device is configured. */
                USBUART_CDC_Init(); // Enable the CDC Endpoint
        }
        
        /* Service USB CDC when device is configured. */
        if (0u != USBUART_GetConfiguration())
        {
            if (0u != USBUART_DataIsReady()) /* Check for input data from host. */
            {
                count = USBUART_GetAll(buffer); /* Read received data and re-enable OUT endpoint. */
                if (0u != count)
                {
                    switch(buffer[0])
                    {
                        case 'c': printBuff("\033[2J\033[H");                           break;
                        case 'o': LED_Write(0);                                         break;
                        case 'O': LED_Write(1);                                         break;
                        case 'r': printControlReg(I2CEXPANDER);                         break;
                        case '0': setPort(I2CEXPANDER,0); printControlReg(I2CEXPANDER); break;
                        case '1': setPort(I2CEXPANDER,1); printControlReg(I2CEXPANDER); break;
                        case '2': setPort(I2CEXPANDER,2); printControlReg(I2CEXPANDER); break;
                        case '3': setPort(I2CEXPANDER,3); printControlReg(I2CEXPANDER); break;
                        case '4': setPort(I2CEXPANDER,4); printControlReg(I2CEXPANDER); break;
                        case 'l': i2c_detect();                                         break;
                        case '?':
                            printBuff("c - Clear Screen\n");
                            printBuff("l - I2C List Devices\n");
                            printBuff("o - LED Off\n");
                            printBuff("O - LED On\n");
                            printBuff("r - Read I2C Expander Control\n");
                            printBuff("0 - Toogle Port 1\n");
                            printBuff("1 - Toogle Port 1\n");
                            printBuff("2 - Toogle Port 2\n");
                            printBuff("3 - Toogle Port 3\n");
                        break;
                    }              
                }
            }
        }
    }
}
