/*! *******************************************************************************************************
* Copyright (c) 2018 Kristóf Szabolcs Horváth
*
* All rights reserved
*
* \file types.h
*
* \brief Own types
*
* \author Kristóf Sz. Horváth
*
**********************************************************************************************************/

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include <stdint.h>
//#include <stdbool.h>


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
//#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#define STATIC_ASSERT(X) ({ extern int __attribute__((error("assertion failure: '" #X "' not true"))) compile_time_check(); ((X)?0:compile_time_check()),0; })


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/
typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
//typedef uint64_t U64;

typedef enum
{
  TRUE  = 0xA3,
  FALSE = 0x00
} BOOL;

/*
typedef bool BOOL;
#define TRUE true
#define FALSE false
*/

#endif // TYPES_H_INCLUDED
/******************************<EOF>**********************************/
