/*! *******************************************************************************************************
* Copyright (c) 2018 Krist�f Szabolcs Horv�th
*
* All rights reserved
*
* \file delay.h
*
* \brief Delay functions
*
* \author Krist�f Sz. Horv�th
*
**********************************************************************************************************/

#ifndef DELAY_H_INCLUDED
#define DELAY_H_INCLUDED

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
#warning "��This macro only works at 16 MHz CPU clock speed!"
#define delay_us(us) (Delay_10cycle((us)*1.6))  // 1.6 = (16 MHz / (10* 1000000 ) )


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Global functions
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Public functions
//--------------------------------------------------------------------------------------------------------/
void Delay_10cycle( U16 u16cyc );  // implemented in assembly


#endif // DELAY_H_INCLUDED
/******************************<EOF>**********************************/