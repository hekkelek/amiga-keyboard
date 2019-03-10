/**
  ******************************************************************************
  * @file    Project/main.c 
  * @author  MCD Application Team
  * @version V2.2.0
  * @date    30-September-2014
  * @brief   Main program body
   ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 


/* Includes ------------------------------------------------------------------*/
#include <intrinsics.h>
#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_spi.h"
#include "stm8s_tim2.h"

#include "types.h"
#include "delay.h"
#include "matrix.h"
#include "amiga_key.h"

/* Private defines -----------------------------------------------------------*/
#define  F_CPU        16000000u

#if !defined(F_CPU)
#error F_CPU is not defined!
#endif

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

void main(void)
{
  // Clock init -- after reset, internal 2 MHz is configured as CPU clock
  CLK_SYSCLKConfig( CLK_PRESCALER_HSIDIV1 );  // this is needed for 16 MHz CPU clock
  CLK_ClockSwitchConfig( CLK_SWITCHMODE_MANUAL, CLK_SOURCE_HSI, DISABLE, CLK_CURRENTCLOCKSTATE_DISABLE );
  
  //TODO: selftests --  flash CRC, watchdog, timers, etc.
  
  Matrix_Init();
  AmigaKey_Init();
  
  // Timer 2 init -- this will be used for sampling the keys
  TIM2_TimeBaseInit( TIM2_PRESCALER_1, 5000u );  //TODO: az 5000u a 16 MHz-es órajelből, az 5 ms prellegési időből és a 16 oszlopból (column) jön ki. A konstansokból kéretik számolni!
  TIM2_ITConfig( TIM2_IT_UPDATE, ENABLE );
  TIM2_Cmd( ENABLE );
  
  enableInterrupts();
  
  /* Main cycle */
  while( TRUE )
  {
    Matrix_Cycle();
    AmigaKey_Cycle();
  }
}


#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while( 1 )
  {
  }
}
#endif


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
