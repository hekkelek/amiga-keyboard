                NAME delay
                
                
                
                RTMODEL "__SystemLibrary", "DLib"
                RTMODEL "__code_model", "small"
                RTMODEL "__core", "stm8"
                RTMODEL "__data_model", "medium"
                RTMODEL "__rt_version", "4"

                EXTERN ?b0
                EXTERN ?b10
                EXTERN ?b11
                EXTERN ?epilogue_l2
                EXTERN ?push_l2
                EXTERN ?w0
                EXTERN ?w1
                EXTERN ?w4
                EXTERN assert_failed
                
                
                
                PUBLIC  Delay_10cycle


                CFI Names cfiNames0
                CFI StackFrame CFA SP DATA
                CFI Resource A:8, XL:8, XH:8, YL:8, YH:8, SP:16, CC:8, PC:24, PCL:8
                CFI Resource PCH:8, PCE:8, ?b0:8, ?b1:8, ?b2:8, ?b3:8, ?b4:8, ?b5:8
                CFI Resource ?b6:8, ?b7:8, ?b8:8, ?b9:8, ?b10:8, ?b11:8, ?b12:8, ?b13:8
                CFI Resource ?b14:8, ?b15:8
                CFI ResourceParts PC PCE, PCH, PCL
                CFI EndNames cfiNames0
              
                CFI Common cfiCommon0 Using cfiNames0
                CFI CodeAlign 1
                CFI DataAlign 1
                CFI ReturnAddress PC CODE
                CFI CFA SP+2
                CFI A Undefined
                CFI XL Undefined
                CFI XH Undefined
                CFI YL Undefined
                CFI YH Undefined
                CFI CC Undefined
                CFI PC Concat
                CFI PCL Frame(CFA, 0)
                CFI PCH Frame(CFA, -1)
                CFI PCE SameValue
                CFI ?b0 Undefined
                CFI ?b1 Undefined
                CFI ?b2 Undefined
                CFI ?b3 Undefined
                CFI ?b4 Undefined
                CFI ?b5 Undefined
                CFI ?b6 Undefined
                CFI ?b7 Undefined
                CFI ?b8 SameValue
                CFI ?b9 SameValue
                CFI ?b10 SameValue
                CFI ?b11 SameValue
                CFI ?b12 SameValue
                CFI ?b13 SameValue
                CFI ?b14 SameValue
                CFI ?b15 SameValue
                CFI EndCommon cfiCommon0



                SECTION `.near_func.text`:CODE:REORDER:NOROOT(0)
                  CFI Block cfiBlock1 Using cfiCommon0
                  CFI Function Delay_us
                CODE
Delay_10cycle:  nop                       // 1cyc  //NOTE: parameter minimum value: 2
                nop                       // 1cyc
                nop                       // 1cyc
                nop                       // 1cyc
                decw X                    // 2cyc
delay_loop:     nop                       // 1cyc
                nop                       // 1cyc
                nop                       // 1cyc
                nop                       // 1cyc
                decw X                    // 2cyc
                tnzw X                    // 2cyc
                jrne delay_loop           // 1/2cyc
                nop                       // (1cyc)
                ret                       // 4 cyc
                  CFI EndBlock cfiBlock1

                END
