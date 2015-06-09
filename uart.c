#include "uart.h"

/*
 *  module global variables
 */
static volatile unsigned char UART_TxBuf[UART_TX_BUFFER_SIZE];
static volatile unsigned char UART_RxBuf[UART_RX_BUFFER_SIZE];
static volatile unsigned char UART_TxHead;
static volatile unsigned char UART_TxTail;
static volatile unsigned char UART_RxHead;
static volatile unsigned char UART_RxTail;
static volatile unsigned char UART_LastRxError;


/*
 * 	Module Interrupt Service Routines
 */
ISR(UART_RECEIVE_INTERRUPT)
/*************************************************************************
Function: UART Receive Complete interrupt
Purpose:  called when the UART has received a character
**************************************************************************/
{
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError;

    /* read UART status register and UART data register */ 
    usr  = UART_STATUS;
    data = UART_DATA;
    
    /* */

    lastRxError = (usr & ((1<<FE)|(1<<DOR)));

    /* calculate buffer index */ 
    tmphead = (UART_RxHead + 1) & UART_RX_BUFFER_MASK;
    
    if ( tmphead == UART_RxTail ) {
        /* error: receive buffer overflow */
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }else{
        /* store new index */
        UART_RxHead = tmphead;
        /* store received data in buffer */
        UART_RxBuf[tmphead] = data;
    }
    UART_LastRxError = lastRxError;   
}


ISR(UART_TRANSMIT_INTERRUPT)
/*************************************************************************
Function: UART Data Register Empty interrupt
Purpose:  called when the UART is ready to transmit the next byte
**************************************************************************/
{
    unsigned char tmptail;
    
    if ( UART_TxHead != UART_TxTail) {
        /* calculate and store new buffer index */
        tmptail = (UART_TxTail + 1) & UART_TX_BUFFER_MASK;
        UART_TxTail = tmptail;
        /* get one byte from buffer and write it to UART */
        UART_DATA = UART_TxBuf[tmptail];  /* start transmission */
    }else{
        /* tx buffer empty, disable UDRE interrupt */
        UART_CONTROL &= ~ (1<<UART_UDRIE);
    }
}

/*
** functions
*/

/*************************************************************************
Function: UART_Init()
Purpose:  initialize UART and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/
void UART_Init(unsigned int baudrate)
{
    UART_TxHead = 0;

    UART_TxTail = 0;
    UART_RxHead = 0;
    UART_RxTail = 0;

    /* Set baud rate */
    if ( baudrate & 0x8000 )
    {
    	 UART_STATUS = (1<<U2X);  //Enable 2x speed
    	 baudrate &= ~0x8000;
    }

    UBRRH = (unsigned char)(baudrate>>8);
    UBRRL = (unsigned char) baudrate;
   
    /* Enable USART receiver and transmitter and receive complete interrupt */
    UART_CONTROL = (1<<RXCIE)|(1<<RXEN)|(1<<TXEN);
    
    /* Set frame format: asynchronous, 8data, no parity, 1stop bit */
    UCSRC = (1<<URSEL)|(3<<UCSZ0);

    /*Enable Global Interrupt*/
    sei();
    
}/* UART_init */


/*************************************************************************
Function: UART_CharGetNonBlocking()
Purpose:  return byte from ringbuffer  
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/
unsigned int UART_CharGetNonBlocking(void)
{    
    unsigned char tmptail;
    unsigned char data;


    if ( UART_RxHead == UART_RxTail ) {
        return UART_NO_DATA;   /* no data available */
    }

    /* calculate /store buffer index */
    tmptail = (UART_RxTail + 1) & UART_RX_BUFFER_MASK;
    UART_RxTail = tmptail;

    /* get data from receive buffer */
    data = UART_RxBuf[tmptail];

    return (UART_LastRxError << 8) + data;

}/* UART_getc */


/*************************************************************************
Function: UART_CharPutNonBlocking()
Purpose:  write byte to ringbuffer for transmitting via UART
Input:    byte to be transmitted
Returns:  none          
**************************************************************************/
void UART_CharPutNonBlocking(unsigned char data)
{
    unsigned char tmphead;


    tmphead  = (UART_TxHead + 1) & UART_TX_BUFFER_MASK;

    while ( tmphead == UART_TxTail ){
        ;/* wait for free space in buffer */
    }

    UART_TxBuf[tmphead] = data;
    UART_TxHead = tmphead;

    /* enable UDRE interrupt */
    UART_CONTROL    |= (1<<UART_UDRIE);

}/* uart_putc */


/*************************************************************************
Function: UART_StringPutNonBlocking()
Purpose:  transmit string to UART
Input:    string to be transmitted
Returns:  none          
**************************************************************************/
void UART_StringPutNonBlocking(const char *s )
{
    while (*s) 
      UART_CharPutNonBlocking(*s++);

}/* uart_puts */


/*************************************************************************
Function: UART_CharsAvail()
Purpose:  Determine the number of bytes waiting in the receive buffer
Input:    None
Returns:  Integer number of bytes in the receive buffer
**************************************************************************/
int UART_CharsAvail(void)
{
        return (UART_RX_BUFFER_MASK + UART_RxHead - UART_RxTail) % UART_RX_BUFFER_MASK;
}/* uart_available */


/*************************************************************************
Function: UART_FlushBuffer()
Purpose:  Flush bytes waiting the receive buffer.  Acutally ignores them.
Input:    None
Returns:  None
**************************************************************************/
void UART_FlushBuffer(void)
{
        UART_RxHead = UART_RxTail;
}/* uart_flush */
