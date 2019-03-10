/*! *******************************************************************************************************
* Copyright (c) 2018 Kristóf Szabolcs Horváth
*
* All rights reserved
*
* \file matrix.c
*
* \brief Keyboard matrix interface
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
#include "amiga_key.h"

// Own include
#include "matrix.h"


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
// Matrix definitions
#define MATRIX_ROW      6u    //!< Number of rows in the keyboard matrix (max. 8)
#define MATRIX_COL      16u   //!< Number of columns in the keyboard matrix (note, that there are max. 8 rows)
#define MATRIX_SAMPLE   2u    //!< Number of samples per key -- used for debouncing


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/
//! \brief Element of logic bit -- GPIO pin assignment tables
typedef struct
{
  GPIO_TypeDef*    psGPIOPort;
  GPIO_Pin_TypeDef ePin;
} S_MATRIX_GPIO_DESC;


//--------------------------------------------------------------------------------------------------------/
// Constants
//--------------------------------------------------------------------------------------------------------/
//!\brief Matrix rows -- there are max. 8 rows!
static const S_MATRIX_GPIO_DESC gcsKeyMatrixRows[ MATRIX_ROW ] =
{
  { GPIOF, GPIO_PIN_4 },   //!< ROW0
  { GPIOB, GPIO_PIN_7 },   //!< ROW1
  { GPIOB, GPIO_PIN_6 },   //!< ROW2
  { GPIOB, GPIO_PIN_4 },   //!< ROW3
  { GPIOB, GPIO_PIN_5 },   //!< ROW4
  { GPIOB, GPIO_PIN_3 }    //!< ROW5
};

//!\brief Matrix columns
static const S_MATRIX_GPIO_DESC gcsKeyMatrixColumns[ MATRIX_COL ] =
{
  { GPIOA, GPIO_PIN_3 },   //!< COL0
  { GPIOB, GPIO_PIN_2 },   //!< COL1
  { GPIOD, GPIO_PIN_7 },   //!< COL2
  { GPIOD, GPIO_PIN_6 },   //!< COL3
  { GPIOD, GPIO_PIN_5 },   //!< COL4
  { GPIOD, GPIO_PIN_4 },   //!< COL5
  { GPIOD, GPIO_PIN_3 },   //!< COL6
  { GPIOD, GPIO_PIN_2 },   //!< COL7
  { GPIOD, GPIO_PIN_0 },   //!< COL8
  { GPIOC, GPIO_PIN_7 },   //!< COL9
  { GPIOC, GPIO_PIN_6 },   //!< COL10
  { GPIOC, GPIO_PIN_5 },   //!< COL11
  { GPIOC, GPIO_PIN_4 },   //!< COL12
  { GPIOC, GPIO_PIN_3 },   //!< COL13
  { GPIOC, GPIO_PIN_2 },   //!< COL14
  { GPIOE, GPIO_PIN_5 }    //!< COL15
};

//! \brief Matrix to scancode translation tables
//! \note  Invalid keys are marked with 0xFFu
static const U8 gcau8ScanCodeTable[ MATRIX_ROW ][ MATRIX_COL ] =
{
//  COL0   COL1   COL2   COL3   COL4   COL5   COL6   COL7   COL8   COL9   COL10  COL11  COL12  COL13  COL14  COL15
  { 0x2Eu, 0x3Du, 0x1Du, 0x1Eu, 0x5Au, 0x59u, 0x58u, 0x57u, 0x56u, 0x55u, 0x54u, 0x53u, 0x52u, 0x51u, 0x50u, 0x45u },  // ROW0
  { 0x46u, 0x41u, 0x0Du, 0x0Cu, 0x0Bu, 0x0Au, 0x09u, 0x08u, 0x07u, 0x06u, 0x05u, 0x04u, 0x03u, 0x02u, 0x01u, 0x00u },  // ROW1
  { 0x2Fu, 0x5Fu, 0x44u, 0x1Bu, 0x1Au, 0x19u, 0x18u, 0x17u, 0x16u, 0x15u, 0x14u, 0x13u, 0x12u, 0x11u, 0x10u, 0x42u },  // ROW2
  { 0x2Du, 0x4Cu, 0x2Bu, 0x2Au, 0x29u, 0x28u, 0x27u, 0x26u, 0x25u, 0x24u, 0x23u, 0x22u, 0x21u, 0x20u, 0x62u, 0x63u },  // ROW3
  { 0x4Eu, 0x4Du, 0x4Fu, 0x61u, 0x3Au, 0x39u, 0x38u, 0x37u, 0x36u, 0x35u, 0x34u, 0x33u, 0x32u, 0x31u, 0x30u, 0x60u },  // ROW4
  { 0x5Eu, 0x3Eu, 0x0Fu, 0x65u, 0x67u, 0x5Bu, 0x3Cu, 0x1Fu, 0x3Fu, 0x5Cu, 0x43u, 0x4Au, 0x5Du, 0x40u, 0x66u, 0x64u }   // ROW5
};
//-----------------------------------------------------------------------------------------------------------------
// Our keyboard matrix looks like this (Amiga Compatible Keyboard Rev. A) -- marked as German keyboard:
//       COL15 COL14 COL13 COL12 COL11 COL10 COL9  COL8  COL7  COL6  COL5   COL4   COL3   COL2   COL1   COL0 
// ROW0  ESC    F1    F2    F3    F4    F5    F6    F7    F8    F9    F10    N.(    N.2    N.1    N.7    N.5   ROW0
// ROW1   ~     1     2     3     4     5     6     7     8     9     0      ß      '      \     Bkspc   Del   ROW1
// ROW2  TAB    Q     W     E     R     T     Z     U     I     O     P      Ü      +      Ret   Help    N.6   ROW2
// ROW3  Ctrl  Caps   A     S     D     F     G     H     J     K     L      Ö      Ä      #      Up     N.4   ROW3
// ROW4  LShft  <>    Y     X     C     V     B     N     M     ,     .      -     RShift Left   Down   Right  ROW4
// ROW5  L-Alt LAmi  Spc   N.*   N.-   N.Ent N./   N.9   N.3   N..   N.)    RAmi   RAlt    N.0    N.8    N.+   ROW5
//-----------------------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
volatile static U8 gau8KeyMatrixSample[ MATRIX_COL ][ MATRIX_SAMPLE ];  //!< Used for sampling and debouncing (bitfield, 0 means pressed, 1 means not pressed)
volatile static U8 gau8KeyMatrixState[ MATRIX_COL ];                    //!< Current state of the keys (bitfield, 0 means pressed, 1 means not pressed)
volatile static U8 gau8KeyEventPressed[ MATRIX_COL ];                   //!< Press events of the keys (bitfield, 1 means press, 0 means no event)
volatile static U8 gau8KeyEventReleased[ MATRIX_COL ];                  //!< Release events of the keys (bitfield, 1 means release, 0 means no event)


//--------------------------------------------------------------------------------------------------------/
// Static function declarations
//--------------------------------------------------------------------------------------------------------/
static void SetColumn( U8 u8Column );


//--------------------------------------------------------------------------------------------------------/
// Static functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Sets the column of the matrix
 * \param  u8Column: value to set
 * \return -
 *********************************************************************/
static void SetColumn( U8 u8Column )
{
  U8 u8Index;
  
  for( u8Index = 0u; u8Index < MATRIX_COL; u8Index++ )  // Columns
  {
    if( u8Column == u8Index )
    {
      GPIO_WriteLow( gcsKeyMatrixColumns[ u8Index ].psGPIOPort, gcsKeyMatrixColumns[ u8Index ].ePin );
    }
    else
    {
      GPIO_WriteHigh( gcsKeyMatrixColumns[ u8Index ].psGPIOPort, gcsKeyMatrixColumns[ u8Index ].ePin );
    }
  }
}


//--------------------------------------------------------------------------------------------------------/
// Public functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Module init
 * \param  -
 * \return -
 * \note   Must be called before Matrix_Cycle(), or the IT routine!
 *********************************************************************/
void Matrix_Init( void )
{
  U8 u8Index;
  
  // GPIO init
  for( u8Index = 0u; u8Index < MATRIX_ROW; u8Index++ )  // Rows
  {
    GPIO_Init( gcsKeyMatrixRows[ u8Index ].psGPIOPort, gcsKeyMatrixRows[ u8Index ].ePin, GPIO_MODE_IN_FL_NO_IT );
  }
  for( u8Index = 0u; u8Index < MATRIX_COL; u8Index++ )  // Columns
  {
    GPIO_Init( gcsKeyMatrixColumns[ u8Index ].psGPIOPort, gcsKeyMatrixColumns[ u8Index ].ePin, GPIO_MODE_OUT_OD_LOW_FAST );
  }
  
  // Globals init
  memset( (void*)gau8KeyMatrixSample,  0xFFu, sizeof( gau8KeyMatrixSample ) );
  memset( (void*)gau8KeyMatrixState,   0xFFu, sizeof( gau8KeyMatrixState ) );
  memset( (void*)gau8KeyEventPressed,  0x00u, sizeof( gau8KeyEventPressed ) );
  memset( (void*)gau8KeyEventReleased, 0x00u, sizeof( gau8KeyEventReleased ) );
}

/*! *******************************************************************
 * \brief  Main cycle
 * \param  -
 * \return -
 * \note   Must be called from main cycle!
 *********************************************************************/
void Matrix_Cycle( void )
{
  U8 u8Row, u8Column;
  U8 u8ScanCode;

  // note: this cycle can be blocked by AmigaKey_Cycle()
  
  // search for new events
  for( u8Column = 0u; u8Column < MATRIX_COL; u8Column++ )
  {
    for( u8Row = 0u; u8Row < MATRIX_ROW; u8Row++ )
    {
      if( 0u != ( (1u<<u8Row) & gau8KeyEventPressed[ u8Column ] ) )  // if there is a press event
      {
        u8ScanCode = gcau8ScanCodeTable[ u8Row ][ u8Column ];  // translating the matrix code to scancode
        if( TRUE == AmigaKey_RegisterScanCode( u8ScanCode, TRUE ) )
        {
          gau8KeyEventPressed[ u8Column ] &= ~(1<<u8Row);
        }
        else  // scancode buffer full, terminate cycle
        {
          u8Row = MATRIX_ROW;
          u8Column = MATRIX_COL;
        }
      }
      else if( 0u != ( (1u<<u8Row) & gau8KeyEventReleased[ u8Column ] ) )  // if there is a release event
      {
        u8ScanCode = gcau8ScanCodeTable[ u8Row ][ u8Column ];  // translating the matrix code to scancode
        if( TRUE == AmigaKey_RegisterScanCode( u8ScanCode, FALSE ) )
        {
          gau8KeyEventReleased[ u8Column ] &= ~(1<<u8Row);
        }
        else  // scancode buffer full, terminate cycle
        {
          u8Row = MATRIX_ROW;
          u8Column = MATRIX_COL;
        }
      }
    }
  }
}

/*! *******************************************************************
 * \brief  Sample the keys and generate events
 * \param  -
 * \return -
 * \note   Must be called from timed IT routine!
 *********************************************************************/
void Matrix_Sample( void )
{
  static U8 u8Column = 0u;
  static U8 u8Sample = 0u;
  U8 u8Index;
  U8 u8Row;
  
  // read row pins
  u8Row = 0u;
  for( u8Index = 0u; u8Index < MATRIX_ROW; u8Index++ )
  {
    u8Row |= ( RESET != GPIO_ReadInputPin( gcsKeyMatrixRows[ u8Index ].psGPIOPort, gcsKeyMatrixRows[ u8Index ].ePin ) ) ? (1u<<u8Index) : 0u;
  }
  
  // store sampled value of rows
  gau8KeyMatrixSample[ u8Column ][ u8Sample ] = u8Row;

  // generate actual state of keys based on samples
  u8Row = 0u;
  for( u8Index = 0u; u8Index < MATRIX_SAMPLE; u8Index++ )
  {
    u8Row |= gau8KeyMatrixSample[ u8Column ][ u8Index ];  // key presses are registered immediately, releases will be registered after MATRIX_SAMPLE times of the sampling period
  }
  
  // search for events
  gau8KeyEventPressed[ u8Column ]  |= ( gau8KeyMatrixState[ u8Column ] ^ u8Row ) & ~u8Row;
  gau8KeyEventReleased[ u8Column ] |= ( gau8KeyMatrixState[ u8Column ] ^ u8Row ) & u8Row;
  
  // the new state will be the one after the events
  gau8KeyMatrixState[ u8Column ] &= ~gau8KeyEventPressed[ u8Column ];
  gau8KeyMatrixState[ u8Column ] |=  gau8KeyEventReleased[ u8Column ];
  
  // Next column, and next sample
  u8Column++;
  if( MATRIX_COL == u8Column )
  {
    u8Column = 0u;
    u8Sample++;
    if( MATRIX_SAMPLE == u8Sample )
    {
      u8Sample = 0u;
    }
  }
  
  // look for special key combinations, eg. CTRL + LAmiga + RAmiga
  if( ( 0u == ( gau8KeyMatrixState[ 14u ] & (1u<<5u) ) )    // ROW5 + COL14 = LAmiga
   && ( 0u == ( gau8KeyMatrixState[  4u ] & (1u<<5u) ) )    // ROW5 + COL4  = RAmiga
   && ( 0u == ( gau8KeyMatrixState[ 15u ] & (1u<<3u) ) ) )  // ROW3 + COL15 = Ctrl
  {
    AmigaKey_Reset();
  }
  
  // Increment MUX state
  SetColumn( u8Column );
}
 
/******************************<EOF>**********************************/
