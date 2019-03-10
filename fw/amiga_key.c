/*! *******************************************************************************************************
* Copyright (c) 2018 Kristóf Szabolcs Horváth
*
* All rights reserved
*
* \file amiga_key.c
*
* \brief Amiga keyboard protocol implementation
*
* \author Kristóf Sz. Horváth
*
**********************************************************************************************************/

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include <string.h>
#include "stm8s.h"
#include "types.h"
#include "delay.h"

// Own include
#include "amiga_key.h"


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
#define AMIGA_CLK_PORT     (GPIOB)
#define AMIGA_CLK_PIN      (GPIO_PIN_0)
#define AMIGA_DAT_PORT     (GPIOB)
#define AMIGA_DAT_PIN      (GPIO_PIN_1)
#define AMIGA_RST_PORT     (GPIOA)
#define AMIGA_RST_PIN      (GPIO_PIN_1)
#define AMIGA_CAPSLED_PORT (GPIOA)
#define AMIGA_CAPSLED_PIN  (GPIO_PIN_2)

#define SCANCODE_FIFO_SIZE        20u  //!< Buffer for outgoing scancodes (max. 256)
#define TIMEOUT_US            143000u  //!< Timeout for the ACK from computer

#define AMIGA_RESET_WARNING     0x78u  //!< Reset warning, sent before reset
#define AMIGA_LAST_KEYCODE_BAD  0xF9u  //!< Last keycode was bad, retransmitting
#define AMIGA_KEYBUFFER_FULL    0xFAu  //!< Keycode buffer was full
#define AMIGA_SELFTEST_FAILED   0xFCu  //!< Self-test failed in keyboard controller
#define AMIGA_INIT_KEYSTREAM    0xFDu  //!< Sent after initialization, marks the initition of power-up key stream
#define AMIGA_TERM_KEYSTREAM    0xFEu  //!< Sent after initial key stream, marks the termination of key stream


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Constants
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
//! \brief Scancode buffer -- stores scancodes waiting to be sent
static volatile struct
{
  U8 au8ScancodeBuffer[ SCANCODE_FIFO_SIZE ];  //!< Scancodes are stored here
  U8 u8ProduceIndex;                           //!< The index where the new scancode will be written to
  U8 u8ConsumeIndex;                           //!< The index where the scancode will be read from
} gsScancodeFIFO;

volatile static BOOL gbIsSynchronized;  //!< Is the communication with the Amiga computer synchronized?
volatile static BOOL gbReTransmit;      //!< Is getting out-of-sync happened when transmitting a character?
volatile static BOOL gbIsCapsLockOn;    //!< State of the Caps Lock key


//--------------------------------------------------------------------------------------------------------/
// Static function declarations
//--------------------------------------------------------------------------------------------------------/
static BOOL ReadScancodeFIFO( U8* pu8ScanCode );
static BOOL RemoveElementFromScancodeFIFO( void );
static void FlushScancodeFIFO( void );
static void SynchronizeCommunication( void );
static BOOL SendScancode( U8 u8Scancode );


//--------------------------------------------------------------------------------------------------------/
// Static functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Reads the oldest element in the FIFO
 * \param  pu8ScanCode: the element will be put here
 * \return TRUE, if success; FALSE, if FIFO is empty
 * \note   This function does not remove elements from the FIFO!
 *********************************************************************/
static BOOL ReadScancodeFIFO( U8* pu8ScanCode )
{
  U8   u8NewIndex;
  BOOL bRet = FALSE;
  
  if( gsScancodeFIFO.u8ProduceIndex != gsScancodeFIFO.u8ConsumeIndex )  // if there is something in the FIFO
  {
    // calculate next consume index
    u8NewIndex = gsScancodeFIFO.u8ConsumeIndex + 1u;
    if( u8NewIndex >= SCANCODE_FIFO_SIZE )
    {
      u8NewIndex = 0u;
    }
    
    // send the scancode back to caller
    *pu8ScanCode = gsScancodeFIFO.au8ScancodeBuffer[ gsScancodeFIFO.u8ConsumeIndex ];
    bRet = TRUE;
  }
  return bRet;
}

/*! *******************************************************************
 * \brief  Reads the oldest element in the FIFO
 * \param  -
 * \return TRUE, if success; FALSE, if FIFO is empty
 *********************************************************************/
static BOOL RemoveElementFromScancodeFIFO( void )
{
  U8   u8NewIndex;
  BOOL bRet = FALSE;
  
  if( gsScancodeFIFO.u8ProduceIndex != gsScancodeFIFO.u8ConsumeIndex )  // if there is something in the FIFO
  {
    // calculate next consume index
    u8NewIndex = gsScancodeFIFO.u8ConsumeIndex + 1u;
    if( u8NewIndex >= SCANCODE_FIFO_SIZE )
    {
      u8NewIndex = 0u;
    }
    
    gsScancodeFIFO.u8ConsumeIndex = u8NewIndex;
    bRet = TRUE;
  }
  return bRet;
}

/*! *******************************************************************
 * \brief  Flush the scancode FIFO
 * \param  -
 * \return -
 *********************************************************************/
static void FlushScancodeFIFO( void )
{
  gsScancodeFIFO.u8ProduceIndex = 0u;
  gsScancodeFIFO.u8ConsumeIndex = 0u;
}

/*! *******************************************************************
 * \brief  Synchronizes the communication with the Amiga computer
 * \param  -
 * \return -
 * \note   Blocking function!
 *********************************************************************/
static void SynchronizeCommunication( void )
{
  U32 u32Wait;

  while( FALSE == gbIsSynchronized )
  {
    GPIO_WriteLow( AMIGA_DAT_PORT, AMIGA_DAT_PIN );  // NOTE: data line is inverted!
    delay_us( 20u );
    GPIO_WriteLow( AMIGA_CLK_PORT, AMIGA_CLK_PIN );
    delay_us( 20u );
    GPIO_WriteHigh( AMIGA_CLK_PORT, AMIGA_CLK_PIN );
    delay_us( 20u );
    GPIO_WriteHigh( AMIGA_DAT_PORT, AMIGA_DAT_PIN );
    
    for( u32Wait = 0u; u32Wait < TIMEOUT_US/6u; u32Wait++ )
    {
      if( RESET == GPIO_ReadInputPin( AMIGA_DAT_PORT, AMIGA_DAT_PIN ) )
      {
        gbIsSynchronized = TRUE;
        u32Wait = TIMEOUT_US;
      }
      delay_us( 2u );  //TODO: not correct, according to the documentation, the ACK can be as short as 1us!
    }
  }

  // Update the state of the Caps lock LED
  if( TRUE == gbIsCapsLockOn )
  {
    GPIO_WriteHigh( AMIGA_CAPSLED_PORT, AMIGA_CAPSLED_PIN );
  }
  else
  {
    GPIO_WriteLow( AMIGA_CAPSLED_PORT, AMIGA_CAPSLED_PIN );
  }
}

/*! *******************************************************************
 * \brief  Sends the scancode to the Amiga computer
 * \param  u8Scancode: the scancode itself
 * \return TRUE, if success; FALSE, if timeout occured
 * \note   Blocking function!
 *********************************************************************/
static BOOL SendScancode( U8 u8Scancode )
{
  U32 u32Wait;
  BOOL bRet = FALSE;
  U8 u8Index;

  for( u32Wait = 0u; u32Wait < TIMEOUT_US/6u; u32Wait++ )
  {
    delay_us( 2u );
    if( RESET != GPIO_ReadInputPin( AMIGA_DAT_PORT, AMIGA_DAT_PIN ) )  // waiting for the data pin to get high
    {
      u32Wait = TIMEOUT_US;
      bRet = TRUE;
    }
  }
  
  // if the data pin got released
  if( TRUE == bRet )
  {
    // pulse the data line before sending
    GPIO_WriteLow( AMIGA_DAT_PORT, AMIGA_DAT_PIN );
    delay_us( 20u );
    GPIO_WriteHigh( AMIGA_DAT_PORT, AMIGA_DAT_PIN );
    delay_us( 100u );  // TODO: where this came from?
    for( u8Index = 0u; u8Index < 8u; u8Index++ )
    {
      if( 0u == ( u8Scancode & (128u>>u8Index) ) )  // NOTE: data line is inverted!
      {
        GPIO_WriteHigh( AMIGA_DAT_PORT, AMIGA_DAT_PIN );
      }
      else
      {
        GPIO_WriteLow( AMIGA_DAT_PORT, AMIGA_DAT_PIN );
      }
      delay_us( 20u );
      GPIO_WriteLow( AMIGA_CLK_PORT, AMIGA_CLK_PIN );
      delay_us( 20u );
      GPIO_WriteHigh( AMIGA_CLK_PORT, AMIGA_CLK_PIN );
      delay_us( 20u );
    }
    GPIO_WriteHigh( AMIGA_DAT_PORT, AMIGA_DAT_PIN );
    delay_us( 2u );

    bRet = FALSE;
    for( u32Wait = 0u; u32Wait < TIMEOUT_US/6u; u32Wait++ )
    {
      delay_us( 2u );  //TODO: not correct, according to the documentation, the ACK can be as short as 1us!
      if( RESET == GPIO_ReadInputPin( AMIGA_DAT_PORT, AMIGA_DAT_PIN ) )
      {
        u32Wait = TIMEOUT_US;
        bRet = TRUE;
      }
    }
  }

  return bRet;
}


//--------------------------------------------------------------------------------------------------------/
// Public functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Module init
 * \param  -
 * \return -
 * \note   Must be called before AmigaKey_Cycle(), or the IT routine!
 *********************************************************************/
void AmigaKey_Init( void )
{
  // GPIO init
  GPIO_Init( AMIGA_CLK_PORT, (GPIO_Pin_TypeDef)AMIGA_CLK_PIN, GPIO_MODE_OUT_OD_HIZ_FAST );
  GPIO_Init( AMIGA_DAT_PORT, (GPIO_Pin_TypeDef)AMIGA_DAT_PIN, GPIO_MODE_OUT_OD_HIZ_FAST );
  GPIO_Init( AMIGA_RST_PORT, (GPIO_Pin_TypeDef)AMIGA_RST_PIN, GPIO_MODE_OUT_OD_HIZ_FAST );
  GPIO_Init( AMIGA_CAPSLED_PORT, (GPIO_Pin_TypeDef)AMIGA_CAPSLED_PIN, GPIO_MODE_OUT_PP_HIGH_FAST );
  
  // Switching on the Caps lock LED
  GPIO_WriteHigh( AMIGA_CAPSLED_PORT, (GPIO_Pin_TypeDef)AMIGA_CAPSLED_PIN );
  
  // TODO: configure Timer 1 as edge counter
  
  // Global variables init
  memset( (void*)&gsScancodeFIFO, 0x00u, sizeof( gsScancodeFIFO ) );
  gbIsSynchronized = FALSE;  // this way the controller will start communication by synchronizing first
  gbReTransmit = FALSE;
  gbIsCapsLockOn = FALSE;
  
  // Standard initialization sequence -- 0xFD, 0xFE --> note: synchronization will be performed before sending any of these
  AmigaKey_RegisterScanCode( AMIGA_INIT_KEYSTREAM, FALSE );
  AmigaKey_RegisterScanCode( AMIGA_TERM_KEYSTREAM, FALSE );
}

/*! *******************************************************************
 * \brief  Main cycle
 * \param  -
 * \return -
 * \note   Must be called from main cycle!
 *********************************************************************/
void AmigaKey_Cycle( void )
{
  U8 u8Scancode;

  // If the keyboard is out-of-sync, then resynchronize
  if( TRUE != gbIsSynchronized )
  {
    while( RESET == GPIO_ReadInputPin( AMIGA_DAT_PORT, AMIGA_DAT_PIN ) );  // wait for the data pin to reach 1
    SynchronizeCommunication();  // blocking call!
    if( TRUE == gbReTransmit )
    {
      gbIsSynchronized = SendScancode( (AMIGA_LAST_KEYCODE_BAD<<1u) | 0x01u );  // so the computer knows, that the last scancode was bad
    }
  }

  // Sending scancodes
  if( TRUE == ReadScancodeFIFO( &u8Scancode ) )
  {
    if( TRUE == SendScancode( u8Scancode ) )  // if the send succeeded
    {
      RemoveElementFromScancodeFIFO();  //TODO: if this function returns with FALSE, then there is a huge error in somewhere...
    }
    else
    {
      gbIsSynchronized = FALSE;
      gbReTransmit = TRUE;
    }
  }
}

/*! *******************************************************************
 * \brief  Put scancode in out FIFO
 * \param  u8Code: scancode to send
 * \param  bIsPressed: TRUE, if button is pressed; FALSE, if it's released
 * \return TRUE, if success; FALSE, if output FIFO is full
 *********************************************************************/
BOOL AmigaKey_RegisterScanCode( U8 u8Code, BOOL bIsPressed )
{
  U8   u8NewIndex;
  BOOL bRet = FALSE;
  
  // Caps lock key is special: only pressed events will be sent
  if( 0x62u == u8Code )
  {
    if( TRUE == bIsPressed )
    {
      gbIsCapsLockOn = (TRUE == gbIsCapsLockOn) ? FALSE : TRUE;
      bIsPressed = gbIsCapsLockOn;
      
      // Switch the LED on or off
      if( TRUE == gbIsCapsLockOn )
      {
        GPIO_WriteHigh( AMIGA_CAPSLED_PORT, AMIGA_CAPSLED_PIN );
      }
      else
      {
        GPIO_WriteLow( AMIGA_CAPSLED_PORT, AMIGA_CAPSLED_PIN );
      }
    }
    else
    {
      return TRUE;  // discard the release event
    }
  }
  
  // calculate next produce index
  u8NewIndex = gsScancodeFIFO.u8ProduceIndex + 1u;
  if( u8NewIndex >= SCANCODE_FIFO_SIZE )
  {
    u8NewIndex = 0u;
  }
  
  if( u8NewIndex != gsScancodeFIFO.u8ConsumeIndex )  // if the FIFO is not full yet
  {
    // add the new element to the FIFO
    gsScancodeFIFO.au8ScancodeBuffer[ gsScancodeFIFO.u8ProduceIndex ] = ( u8Code << 1u ) | ( TRUE == bIsPressed ? 0u : 1u );  // Amiga communication format
    gsScancodeFIFO.u8ProduceIndex = u8NewIndex;
    bRet = TRUE;
  }
  
  return bRet;
}

/*! *******************************************************************
 * \brief  Send reset signal to the computer
 * \param  -
 * \return -
 * \note   There can be a delay between this function call (--> reset warning) and the actual reset. This function is blocking.
 *********************************************************************/
void AmigaKey_Reset( void )
{
  U32 u32Wait;
  
  FlushScancodeFIFO();
  //TODO: send reset warning, wait and pull the reset line
  
  // Pull the reset line
  GPIO_WriteLow( AMIGA_RST_PORT, AMIGA_RST_PIN );
  
  // Wait for at least 500 ms
  for( u32Wait = 0u; u32Wait < 10u; u32Wait++ )
  {
    delay_us( 50000u );
  }

  //TODO: wait for releasing Ctrl+LAmiga+RAmiga
  
  // Release the reset line
  GPIO_WriteHigh( AMIGA_RST_PORT, AMIGA_RST_PIN );

  // Reset self -- there is no soft reset instruction on STM8, so unconditional jump will be used
/*  disableInterrupts();
  __asm( "JPF $008000" );  //TODO: is the reset vector a jump instruction??
  */
  
  AmigaKey_Init();
}

/******************************<EOF>**********************************/
