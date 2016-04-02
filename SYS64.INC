;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                               ;;
;;  Tensor  process management, protected ring3                  ;;
;;                                                               ;;
;;  Distributed under GPL. See file COPYING for details.         ;;
;;  Copyright 2013 Marco K                                       ;;
;;                                                               ;;
;;  Datasheets:                                                  ;;
;;      Intel IA-32 (64ia.pdf) [http://www.intel.com/content]    ;;
;;      AMD Family 16h [http://developer.amd.com/resources]      ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


align 8

irq32:
        cmp     [error_interrupt_long],-1
        je      no_error_in_previous_process_long

        mov     r8,[error_interrupt_long]
        mov     rdi,r8
        imul    rdi,16
        mov     [rdi+tss0i_l_lo +5],word 01010000b *256 + 11101001b

        mov     rdi,qword [error_interrupt_long]
        imul    rdi,128
        add     rdi,0x520000
        mov     rsi,qword [error_interrupt_entry_long]
        mov     [rdi+l.rsi-tss_register_long],rsi
        mov     r8,0x11002
        mov     [rdi+l.rflags-tss_register_long],r8

        mov     [0x1FFFE],byte 0

        mov     [error_interrupt_long],-1

     no_error_in_previous_process_long:
        mov     rdi,[0x6000]
        imul    rdi,16
        mov     [rdi+gdts_lo+ tss0 +5],word 01010000b *256 +11101001b

        inc     qword [0x1FBE0]

        mov     rax,[0x1FBE0]
        cmp     rax,qword [next_usage_update_long]
        jb      nocounter_long
        add     rax,100
        mov     [next_usage_update_long],rax
        call    updatecputimes
      nocounter_long:

        mov     rdi,[0x6020]

        mov     rbx,[rdi+0x36]
        call    _rdtsc
        sub     rax,rbx
        add     rax,[rdi+0x28]
        mov     [rdi+0x28],rax

        mov     rbx,[0x6000]

        cmp     [0x1FFFE],byte 1
        je      do_not_change_task_long

      waiting_for_termination_long:
      waiting_for_reuse_long:
        add     rdi,0x50
        inc     rbx

        cmp     [rdi+0xA],byte 3
        je      waiting_for_termination_long
        cmp     [rdi+0xA],byte 4
        je      waiting_for_termination_long
        cmp     [rdi+0xA],byte 9
        je      waiting_for_reuse_long

        cmp     rbx,[0x6008]
        jbe     nsched0_long
        mov     rbx,1
        mov     rdi,0x6040

      nsched0_long:

        mov     [0x6000],rbx
        mov     [0x6020],rdi

      do_not_change_task_long:

        call    _rdtsc
        mov     [rdi+0x36],rax

        cmp     [0x1FFFE],byte 0
        je      nodecffff_long
        dec     byte [0x1FFFE]
      nodecffff_long:

;        shl     bx,3
;        add     bx,tss0
;        mov     [tss_s],bx

        mov     al,0x20
        mov     dx,0x20
        out     dx,al

;        db      0xEA
;tss_t   dq      0
;tss_s   dw      tss0call
;
;        jmp     irq32

        iretq

next_usage_update_long dq 100

change_task_long:

        mov     [0x1FFFE],byte 2

        dec     qword [0x1FBE0]

        int     0x20

        retq

align 8

gdts_lo:

        dw      0x0000
        dw      0x0000
        db      0x00
        dw      0x0000
        db      0x00

os_code_l:

        dw      0xFFFF
        dw      0x0000
        db      0x00
        dw      10111111b *256 +10011010b
        db      0x00

os_data_l:

        dw      0xFFFF
        dw      0x0000
        db      0x00
        dw      10011111b *256 +10010010b
        db      0x00

graph_data_l_lo:

        dw      0xFF
        dw      0x0000
        db      0x00
        dw      11011111b *256 +11110010b
        db      0x00

ring3_code_l_lo:

        dw      0xFFFF
        dw      0x0000
        db      0x00
        dw      10111111b *256 +11111010b
        db      0x00

ring3_data_l_lo:

        dw      0xFFFF
        dw      0x0000
        db      0x00
        dw      11011111b *256 +11110010b
        db      0x00

ring2_code_l_lo:

        dw      0xFFFF
        dw      0x0000
        db      0x00
        dw      10111111b *256 +11011010b
        db      0x00

ring2_data_l_lo:

        dw      0xFFFF
        dw      0x0000
        db      0x00
        dw      11011111b *256 +11010010b
        db      0x00

ring1_code_l_lo:

        dw      0xFFFF
        dw      0x0000
        db      0x00
        dw      10111111b *256 +10111010b
        db      0x00

ring1_data_l_lo:

        dw      0xFFFF
        dw      0x0000
        db      0x00
        dw      11011111b *256 +10110010b
        db      0x00

int_code_l_lo:

        dw      0xFFFF
        dw      0x0000
        db      0x00
        dw      10111111b *256 +10011110b
        db      0x00

int_data_l_lo:

        dw      0xFFFF
        dw      0x0000
        db      0x00
        dw      11011111b *256 +10010010b
        db      0x00

tss0_l_lo:

        times (max_processes+10) dq 0,0

tss0i_l_lo:

        times (256+10) dd 0,0,0,0

app_code_l_lo:

        times (max_processes+10) dq 0

app_data_l_lo:

        times (max_processes+10) dq 0

tss0sys_l_lo:

        times (max_processes+10) dd 0,0,0,0

tss0call_l_lo:

        times (max_processes+10) dd 0,0,0,0

gdte_lo:

idts_long:
        rept 17  n: 0   {dw s#n         and 0x0FFFF, os_code, 0x8F00, s#n         shr 16,0,0,0,0}
        rept 15  n:17   {dw i_unknown#n and 0x0FFFF, os_code, 0x8E00, i_unknown#n shr 16,0,0,0,0}
        rept 1   n:32   {dw irq#n       and 0x0FFFF, os_code, 0x8F00, irq#n       shr 16,0,0,0,0}
        rept 10  n:34   {dw p_irq#n     and 0x0FFFF, os_code, 0x8F00, p_irq#n     shr 16,0,0,0,0}
                         dw irq45       and 0x0FFFF, os_code, 0x8E00, irq45       shr 16,0,0,0,0
        rept 1   n:46   {dw p_irq#n     and 0x0FFFF, os_code, 0x8F00, p_irq#n     shr 16,0,0,0,0}
        rept 51  n:48   {dw i_unknown#n and 0x0FFFF, os_code, 0x8E00, i_unknown#n shr 16,0,0,0,0}
                         dw i64         and 0x0FFFF, os_code, 0x8E00, i64         shr 16,0,0,0,0
        rept 154 n:101  {dw i_unknown#n and 0x0FFFF, os_code, 0x8E00, i_unknown#n shr 16,0,0,0,0}
idte_long:

align 8

idt_lo:
        dw      idte_long-idts_long-1
        dq      idts_long

align 8

gdts_long:

        dw      gdte_lo-gdts_lo-1
        dq      gdts_lo

align 8

ist_numb           db   0x00

build_process_gdt_tss_pointer_long:
        mov     rcx,tss_data
        mov     rdi,0
        mov     r10,tss_step
      setgdtl2_long:
        mov     [rdi+gdts_lo+ tss0 +0],word tss_step
        mov     [rdi+gdts_lo+ tss0 +2],cx
        mov     rax,rcx
        shr     rax,16
        mov     [rdi+gdts_lo+ tss0 +4],al
        mov     [rdi+gdts_lo+ tss0 +7],ah
        shr     rax,16
        mov     [rdi+gdts_lo+ tss0 +8],eax
        mov     [rdi+gdts_lo+ tss0 +5],word 10010000b *256 +11101001b
        mov     [rdi+gdts_lo+ tss0 +12],r9d
        add     rcx,r10
        add     rdi,16
        cmp     rdi,16*(max_processes+5)
        jbe     setgdtl2_long
        retq

build_process_gdt_gate_pointer_long:
        mov     rdi,0
        mov     rdx,os_code
        mov     r8,16
        and     rdi,0x0FFFF
        mov     r9,qword [sys_int_long+rdi*8]
        mov     rcx,rdi
      setidtl1_long:
        inc     rcx
        push    r9
        mov     [rdi+gdts_lo+ tss0call +0],r9w
        mov     [rdi+gdts_lo+ tss0call +2],dx
        mov     [rdi+gdts_lo+ tss0call +4],word 10001100b*256 +00000000b
        shr     r9,16
        mov     [rdi+gdts_lo+ tss0call +6],r9w
        shr     r9,16
        mov     [rdi+gdts_lo+ tss0call +8],r9d
        mov     [rdi+gdts_lo+ tss0call +12],dword 0
        pop     r9
        push    rdi
        mov     rdi,rcx
        and     rdi,0x0FFFF
        mov     r9,qword [sys_int_long+rdi*8]
        pop     rdi
        add     rdi,r8
        cmp     rdi,16*(max_processes+5)
        jb      setidtl1_long

        retq

build_interrupt_table_long:
        mov     [l.rflags],0x11002
        mov     [l.rsp0+0],0x52000
        mov     [l.rsp0+4],int_data
        mov     [l.rsp1+0],0x53000
        mov     [l.rsp1+4],ring1_data
        mov     [l.rsp2+0],0x54000
        mov     [l.rsp2+4],ring2_data
        mov     [l.ist1],0x62000
        mov     [l.ist2],0x64000
        mov     [l.ist3],0x66000
        mov     [l.ist4],0x68000
        mov     [l.ist5],0x70000
        mov     [l.ist6],0x72000
        mov     [l.ist7],0x74000

        mov     rax,cr3
        mov     [l.cr3_long],rax
        mov     r8,int_code
        mov     [l.cs_long],r8w
        mov     r8,int_data
        mov     [l.ds_long],r8w

        mov     rax,sys_int_long
        mov     [l.rsp],0x720000
        mov     rdi,0x520000
        mov     r9,sys_int_long+8*0x60

      newint_long:
        push    rdi
        mov     rbx,[rax]
        mov     [l.rip],rbx
        mov     rsi,tss_sceleton_long
        mov     rcx,120/8
        cld
        rep     movsq
        pop     rdi

        add     rdi,128
        add     [l.rsp],2048
        add     rax,8

        cmp     rax,r9
        ja      newint_long

;;

        mov     rsi,qword boot_sched_3_2_long
        call    boot_log

        mov     rcx,0x520000
        mov     rdi,0

      setgdtl2i_long:
        mov     [rdi+gdts_lo+ tss0i +0],word 256
        mov     [rdi+gdts_lo+ tss0i +2],cx
        mov     rax,rcx
        shr     rax,16
        mov     [rdi+gdts_lo+ tss0i +4],al
        mov     [rdi+gdts_lo+ tss0i +7],ah
        mov     [rdi+gdts_lo+ tss0i +5],word 01010000b *256 +11101001b
        shr     rax,16
        mov     [rdi+gdts_lo+ tss0i +8],eax
        mov     [rdi+gdts_lo+ tss0i +12],r11d
        add     rcx,128
        add     rdi,16
        cmp     rdi,16*0x60
        jbe     setgdtl2i_long
;;

        mov     rsi,boot_sched_3_3_long
        call    boot_log

        mov     rdi,0
;        mov     rdx,os_code
;        mov     r8b,0x04
;        mov     r9,s0_long-sys_int_long
;        xor     r11,r11
;        mov     r12,i_unknown12_long-sys_int_long
;        jmp     $
;      setidtl2_long:
;        push    r9
;        mov     [rdi+idts_long+ 8 +0],r9w
;        shr     r9,16
;        mov     [rdi+idts_long+ 8 +6],r9w
;        shr     r9,16
;        mov     [rdi+idts_long+ 8 +8],r9d
;        mov     [rdi+idts_long+ 8 +2],dx
;        mov     [rdi+idts_long+ 8 +5],byte 10001110b
;        mov     [rdi+idts_long+ 8 +4],r8b
;        mov     [rdi+idts_long+ 8 +12],r11d
;        pop     r9
;        cmp     rdi,0x64*16
;        jne     no_sw_int_long
;        mov     [rdi+idts_long+ 8 +5],byte 11101110b
;      no_sw_int_long:
;        add     rdi,16
;        add     r9,8
;
;        cmp     rdi,16*0x65
;        jb      setidtl2_long
;
        retq
build_syscall_interrupt_table_long:
        mov     [l.rflags],0x11002
        mov     [l.rsp0+0],0x52000
        mov     [l.rsp0+4],int_data
        mov     [l.rsp1+0],0x53000
        mov     [l.rsp1+4],ring1_data
        mov     [l.rsp2+0],0x54000
        mov     [l.rsp2+4],ring2_data
        mov     [l.ist1],0x62000
        mov     [l.ist2],0x64000
        mov     [l.ist3],0x66000
        mov     [l.ist4],0x68000
        mov     [l.ist5],0x70000
        mov     [l.ist6],0x72000
        mov     [l.ist7],0x74000

        mov     rax,cr3
        mov     [l.cr3_long],rax
        mov     [r9d],dword int_code
        mov     [r10d],dword int_data

        mov     [l.rsp],sysint_stack_data
        mov     rdi,0x530000

      newint2_long:
        push    rdi
        mov     rbx,i64
        mov     [l.rip],rbx
        mov     rsi,tss_sceleton_long
        mov     rcx,120/8
        cld
        rep     movsq
        pop     rdi

        add     [l.rsp],4096
        add     rdi,128
        add     rax,8

        cmp     rdi,0x530000+128*(max_processes+5)
        jb      newint2_long

;;

        mov     rcx,0x530000
        mov     rdi,0
      setgdtl2i2_long:
        mov     [rdi+gdts_lo+ tss0i +0],word 128
        mov     [rdi+gdts_lo+ tss0i +2],cx
        mov     rax,rcx
        shr     rax,16
        mov     [rdi+gdts_lo+ tss0i +4],al
        mov     [rdi+gdts_lo+ tss0i +7],ah
        mov     [rdi+gdts_lo+ tss0i +5],word 01010000b *256 +11101001b
        shr     rax,16
        mov     [rdi+gdts_lo+ tss0i +8],eax
        mov     [rdi+gdts_lo+ tss0i +12],r11d
        add     rcx,128
        add     rdi,16
        cmp     rdi,16*(max_processes+5)
        jbe     setgdtl2i2_long

;;
;        mov     r9,i64-sys_int_long
;        mov     rdi,16*0x64
;
;        mov     rdx,os_code
;        mov     [rdi+idts_long+ 8 +0],r9w
;        shr     r9,16
;        mov     [rdi+idts_long+ 8 +6],r9w
;        shr     r9,16
;        mov     [rdi+idts_long+ 8 +8],r9d
;        mov     [rdi+idts_long+ 8 +2],dx
;        mov     [rdi+idts_long+ 8 +5],byte 10001110b
;        mov     [rdi+idts_long+ 8 +4],byte 0x05
;        mov     [rdi+idts_long+ 8 +12],r11d
;
        retq

updatecputimes:
        pushdu

        call    _rdtsc
        mov     rax,[idleuse]
        mov     [idleusesec],rax
        mov     [idleuse],0
        mov     rcx,[0x6008]
        mov     rdi,0x6040
      newupdate_long:
        mov     r8,0
        mov     rbx,[rdi+0x2E]
        mov     [rdi+0x3E],rbx
        mov     [rdi+0x2E],r8
        mov     [rdi+0x2E+4],r8
        add     rdi,0x46
        loop    newupdate_long

        popdu

        retq

sys_int_long:

        dq      s0,s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12,s13,s14,s15

        dq      s16        ,s17        ,i_unknown18,i_unknown19
        dq      i_unknown20,i_unknown21,i_unknown22,i_unknown23
        dq      i_unknown24,i_unknown25,i_unknown26,i_unknown27
        dq      i_unknown28,i_unknown29,i_unknown30,i_unknown31

        dq      irq32  ,irq33  ,p_irq34,p_irq35,p_irq36,p_irq37,p_irq38 ,p_irq39
        dq      p_irq40,p_irq41,p_irq42,p_irq43,p_irq44,irq45  ,p_irq46,p_irq47

        dq      i_unknown48,i_unknown49,i_unknown50,i_unknown51
        dq      i_unknown52,i_unknown53,i_unknown54,i_unknown55
        dq      i_unknown56,i_unknown57,i_unknown58,i_unknown59
        dq      i_unknown60,i_unknown61,i_unknown62,i_unknown63

        dq      i_unknown64,i_unknown65,i_unknown66,i_unknown67
        dq      i_unknown68,i_unknown69,i_unknown70,i_unknown71
        dq      i_unknown72,i_unknown73,i_unknown74,i_unknown75
        dq      i_unknown76,i_unknown77,i_unknown78,i_unknown79

        dq      i_unknown80,i_unknown81,i_unknown82,i_unknown83
        dq      i_unknown84,i_unknown85,i_unknown86,i_unknown87
        dq      i_unknown88,i_unknown89,i_unknown90,i_unknown91
        dq      i_unknown92,i_unknown93,i_unknown94,i_unknown95

        dq      i_unknown96,i_unknown97,i_unknown98,i_unknown99
        dq      i64        ,i_unknown101,i_unknown102,i_unknown103
        dq      i_unknown104,i_unknown105,i_unknown106,i_unknown107
        dq      i_unknown108,i_unknown109,i_unknown110,i_unknown111

        dq      i_unknown112,i_unknown113,i_unknown114,i_unknown115
        dq      i_unknown116,i_unknown117,i_unknown118,i_unknown119
        dq      i_unknown120,i_unknown121,i_unknown122,i_unknown123
        dq      i_unknown124,i_unknown125,i_unknown126,i_unknown127

        dq      i_unknown128,i_unknown129,i_unknown130,i_unknown131
        dq      i_unknown132,i_unknown133,i_unknown134,i_unknown135
        dq      i_unknown136,i_unknown137,i_unknown138,i_unknown139
        dq      i_unknown140,i_unknown141,i_unknown142,i_unknown143

        dq      i_unknown144,i_unknown145,i_unknown146,i_unknown147
        dq      i_unknown148,i_unknown149,i_unknown150,i_unknown151
        dq      i_unknown152,i_unknown153,i_unknown154,i_unknown155
        dq      i_unknown156,i_unknown157,i_unknown158,i_unknown159

        dq      i_unknown160,i_unknown161,i_unknown162,i_unknown163
        dq      i_unknown164,i_unknown165,i_unknown166,i_unknown167
        dq      i_unknown168,i_unknown169,i_unknown170,i_unknown171
        dq      i_unknown172,i_unknown173,i_unknown174,i_unknown175

        dq      i_unknown176,i_unknown177,i_unknown178,i_unknown179
        dq      i_unknown180,i_unknown181,i_unknown182,i_unknown183
        dq      i_unknown184,i_unknown185,i_unknown186,i_unknown187
        dq      i_unknown188,i_unknown189,i_unknown190,i_unknown191

        dq      i_unknown192,i_unknown193,i_unknown194,i_unknown195
        dq      i_unknown196,i_unknown197,i_unknown198,i_unknown199
        dq      i_unknown200,i_unknown201,i_unknown202,i_unknown203
        dq      i_unknown204,i_unknown205,i_unknown206,i_unknown207

        dq      i_unknown208,i_unknown209,i_unknown210,i_unknown211
        dq      i_unknown212,i_unknown213,i_unknown214,i_unknown215
        dq      i_unknown216,i_unknown217,i_unknown218,i_unknown219
        dq      i_unknown220,i_unknown221,i_unknown222,i_unknown223

        dq      i_unknown224,i_unknown225,i_unknown226,i_unknown227
        dq      i_unknown228,i_unknown229,i_unknown230,i_unknown231
        dq      i_unknown232,i_unknown233,i_unknown234,i_unknown235
        dq      i_unknown236,i_unknown237,i_unknown238,i_unknown239

        dq      i_unknown240,i_unknown241,i_unknown242,i_unknown243
        dq      i_unknown244,i_unknown245,i_unknown246,i_unknown247
        dq      i_unknown248,i_unknown249,i_unknown250,i_unknown251
        dq      i_unknown252,i_unknown253,i_unknown254,i_unknown255

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;                                                            ;;
;;;                   SYSTEM CALL ENTRY                        ;;
;;;                                                            ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
align 8

i64:
        cli

; rdi holds the TSS Selector Selector Index

        mov     rdi,[0x6000]
        imul    rdi,8
        mov     [rdi+gdts_lo+ tss0 +5],word 01010000b *256 +11101001b

        mov     rax,[schd_long]
        mov     [usedi64+rax],byte 1
        push    rax

        mov     rdi,[0x6000]
        imul    rdi,256
        mov     [rdi+0x800000+0x13F],rax

        mov     rax,1                                                           ; search from 1 ( 0 reserved for 'not used' in app)
      search_free_i64:
        cmp     [usedi64+rax],byte 0
        je      found_free_i64
        inc     rax
        cmp     rax,max_processes+10
        jbe     search_free_i64
        jmp     $
      found_free_i64:
        mov     [schd_long],rax

        add     rdx,int_code
        imul    rdx,[schd_long]
        push    rdx
        mov     rdi,16*0x64
        mov     word [ist_numb],0x06

        mov     [rdi+gdts_lo+tss0call +2],dx
        pop     rdx
        push    rdx
        push    rdx
        mov     [rdi+gdts_lo+tss0call +0],dx
        pop     rdx
        shr     rdx,16
        mov     [rdi+gdts_lo+tss0call +6],dx
        pop     rdx
        shr     rdx,32
        mov     [rdi+gdts_lo+tss0call +8],edx
        mov     [rdi+gdts_lo+tss0call +5],byte 10001110b
        mov     dl,[ist_numb]
        and     dl,0x07
        mov     [rdi+gdts_lo+tss0call +4],dl                                       ; we keep it under the 2 bit (00000XXXb).
        mov     [rdi+gdts_lo+tss0call +12],dword 0

        mov     rbx,[0x6000]
        shl     rbx,3
        push    rax
        mov     rax,tss0_l_lo
        add     rbx,rax
        pop     rax

        mov     rcx,[0x6000]
        shl     rcx,3

        mov     rax,[0x6000]
        mov     [tasknum_long+rcx],rax

        mov     rax,[rbx]
        mov     [reg1_long+rcx],rax
        mov     rax,[rbx+8]
        mov     [reg2_long+rcx],rax

        mov     rcx,8
        imul    rcx,[rsp]
        mov     rax,qword [tss0sys_l_lo+rcx]
        mov     [rbx],rax

        call    save_registers_long

        mov     rsi,[0x6000]
        imul    rsi,tss_step
        mov     r9,tss_data
        add     rsi,r9

        mov     r11,[rsi+l.rax-tss_register_long]
        mov     r12,[rsi+l.rbx-tss_register_long]
        mov     r13,[rsi+l.rcx-tss_register_long]

        ; after an software interrupt, we get the states
        ; and search in the servetable for the syscall.

        pushdu

        mov     rdi,[rsi+l.rax-tss_register_long]
        mov     rax,[rsi+l.rbx-tss_register_long]
        mov     rbx,[rsi+l.rcx-tss_register_long]
        mov     rcx,[rsi+l.rdx-tss_register_long]
        mov     rdx,[rsi+l.rsi-tss_register_long]
        mov     rsi,[rsi+l.rdi-tss_register_long]

        sti
        push    rax
        and     rdi,0xFF
        call    qword [servetable_long+rdi*8]
        pop     rax
        cli

        popdu

        mov     rsi,[0x6000]
        imul    rsi,tss_step
        mov     r9,tss_data
        add     rsi,r9

        mov     [rsi+l.rax-tss_register_long],r11
        mov     [rsi+l.rbx-tss_register_long],r12
        mov     [rsi+l.rcx-tss_register_long],r13

        mov     rbx,[0x6000]
        shl     rbx,3
        push    rax
        mov     rax,tss0_l_lo
        add     rbx,rax
        pop     rax

        mov     rcx,[0x6000]
        shl     rcx,3

        mov     rax,[reg1_long+rcx]
        mov     [rbx],rax
        mov     rax,[reg2_long+rcx]
        mov     [rbx+8],rax

        mov     rdi,[0x6000]                                            ; no syscall interrupt in use anymore
        imul    rdi,256
        mov     [rdi+0x800000+0x13F],rax

        mov     [tasknum_long+rcx],dword 0
        mov     [tasknum_long+rcx+4],dword 0

        mov     rdi,8
        pop     rax                                                     ; this handler
        mov     [usedi64+rax],byte 0
        imul    rdi,rax
        mov     [rdi+tss0sys_l_lo +5],word 01010000b *256 +11101001b

        mov     rbx,[0x6000]
        shl     bx,3
        add     bx,tss0call
        mov     [tss_s3_long],bx

                db      0xEA
tss_t3_long     dd      0
tss_s3_long     dw      tss0call

        jmp     i64

tasknum_long:   times (max_processes+10) dq 0,0
reg1_long:      times (max_processes+10) dq 0,0
reg2_long:      times (max_processes+10) dq 0,0
usedi64:        times (max_processes+10) db 0,0
schd_long       dq    0x0

save_registers_long:
        mov     rsi,[0x6000]
        imul    rsi,tss_step
        mov     r9,tss_data
        add     rsi,r9

        mov     rax,[rsi+l.rax-tss_register_long]
        mov     rbx,[rsi+l.rbx-tss_register_long]
        mov     rcx,[rsi+l.rcx-tss_register_long]
        mov     rdx,[rsi+l.rdx-tss_register_long]
        mov     rdi,[rsi+l.rdi-tss_register_long]
        mov     rbp,[rsi+l.rbp-tss_register_long]

        mov     rsi,[rsi+l.rsi-tss_register_long]

        pushdu

        mov     rsi,[0x6020]
        mov     rax,[rsi+0x8]
        mov     rsi,rsp
        inc     [save_syscall_count_long]
        mov     rdi,[save_syscall_count_long]
        and     rdi,15
        shl     rdi,6
        push    rbx
        mov     rbx,save_syscall_data_long+64
        add     rdi,rbx
        pop     rbx
        mov     [rdi-64],rax
        mov     rcx,64 / 8
        cld
        rep     movsq

        popdu

        ret

save_syscall_count_long                       dq      0x0
save_syscall_data_long:      times 100*16     dq      0x0

align 8

servetable_long:

;      dq sys_drawwindow                                                 ; 0-DrawWindow
;      dq syscall_setpixel_long                                          ; 1-SetPixel
;      dq sys_getkey_long                                                ; 2-GetKey
;      dq sys_clock_long                                                 ; 3-GetTime
;      dq syscall_writetext_long                                         ; 4-WriteText
;      dq delay_hs_long                                                  ; 5-DelayHs
;      dq syscall_openramdiskfile_long                                   ; 6-OpenRamdiskFile
;      dq syscall_putimage_long                                          ; 7-PutImage
;      dq sys_button_long                                                ; 8-DefineButton
;      dq sys_cpuusage_long                                              ; 9-GetProcessInfo
;      dq sys_waitforevent_long                                          ; 10-WaitForEvent
;      dq sys_getevent_long                                              ; 11-CheckForEvent
;      dq sys_redrawstat_long                                            ; 12-BeginDraw and EndDraw
;      dq syscall_drawrect_long                                          ; 13-DrawRect
;      dq syscall_getscreensize_long                                     ; 14-GetScreenSize
;      dq sys_background_long                                            ; 15-bgr
;      dq sys_cachetodiskette_long                                       ; 16-FlushFloppyCache
;      dq sys_getbutton_long                                             ; 17-GetButton
;      dq syscall_system_long                                            ; 18-Shutdown,KillApp,WindowActivate
;      dq syscall_startapp_long                                          ; 19-StartApp
;      dq sys_midi                                                       ; 20-ResetMidi and OutputMidi
;      dq sys_setup_long                                                 ; 21-SetMidiBase,SetKeymap,SetShiftKeymap,.
;      dq undefined_syscall_long                                         ; 22-not used
;      dq sys_wait_event_timeout_long                                    ; 23-TimeOutWaitForEvent
;      dq syscall_cdaudio_long                                           ; 24-PlayCdTrack,StopCd and GetCdPlaylist
;      dq sys_sb16_long                                                  ; 25-SetSb16
;      dq sys_getsetup_long                                              ; 26-GetMidiBase,GetKeymap,GetShiftKeymap,.
;      dq sys_wss_long                                                   ; 27-SetWssMainVol and SetWssCdVol
;      dq sys_sb16II_long                                                ; 28-SetSb16
;      dq sys_date_long                                                  ; 29-GetDate
;      dq syscall_readhd_long                                            ; 30-ReadHd
;      dq syscall_starthdapp_long                                        ; 31-StartHdApp
;      dq syscall_delramdiskfile_long                                    ; 32-DelRamdiskFile
;      dq syscall_writeramdiskfile_long                                  ; 33-WriteRamdiskFile
;      dq read_floppy_file_long                                          ; 34-ReadFloppyDrive
;      dq syscall_getpixel_long                                          ; 35-GetPixel
;      dq syscall_readstring_long                                        ; 36-ReadString (not yet ready)
;      dq readmousepos_long                                              ; 37-GetMousePosition_ScreenRelative,.
;      dq syscall_drawline_long                                          ; 38-DrawLine
;      dq sys_getbackground_long                                         ; 39-GetBackgroundSize,ReadBgrData,.
;      dq set_app_param_long                                             ; 40-WantEvents
;      dq syscall_getirqowner_long                                       ; 41-GetIrqOwner
;      dq get_irq_data_long                                              ; 42-ReadIrqData
;      dq sys_outport_long                                               ; 43-SendDeviceData
;      dq sys_programirq_long                                            ; 44-ProgramIrqs
;      dq reserve_free_irq_long                                          ; 45-ReserveIrq and FreeIrq
;      dq syscall_reserveportarea_long                                   ; 46-ReservePortArea and FreePortArea
;      dq display_number_long                                            ; 47-WriteNum
;      dq display_settings_long                                          ; 48-SetRedrawType and SetButtonType
;      dq syscall_appints_long                                           ; 49-AppInts
;      dq random_shaped_window_long                                      ; 50-Window shape & scale
;      dq syscall_threads_long                                           ; 51-Threads
;      dq stack_driver_stat_long                                         ; 52-Stack driver status
;      dq socket_long                                                    ; 53-Socket interface
;      dq user_events_long                                               ; 54-User events
;      dq sound_interface_long                                           ; 55-Sound interface
;      dq write_to_hd_long                                               ; 56-Write a file to hd
;      dq delete_from_hd_long                                            ; 57-Delete a file from hd
;      dq file_system_long                                               ; 58-Common file system interface
;      dq sys_trace_long                                                 ; 59-System call trace
;      dq sys_ipc_long                                                   ; 60-Inter Process Communication
;      dq sys_gs_long                                                    ; 61-Direct graphics access
;      dq sys_pci_long                                                   ; 62-PCI functions
;      dq sys_msg_board_long                                             ; 63-System message board
;      dq sys_resize_app_memory_long                                     ; 64-Resize application memory usage
;      dq undefined_syscall_long                                         ; 65-UTF
;      dq sys_process_def_long                                           ; 66-Process definitions - keyboard
;      dq sys_window_move_long                                           ; 67-Window move or resize
;      dq sys_delay_long                                                 ; 105-Delay (ms)
;      dq sys_data_long                                                  ; 110-Get data from running process
;      dq sys_info_long                                                  ; 111-Who am I
;      dq sys_config_param_long                                          ; 112-Get configuration parameter
;      dq scroll_bar_long                                                ; 113-Vertical and horizontal scroll bars
;      dq cd_audio_long                                                  ; 114-CD audio
;      dq sys_pci_long                                                   ; 115-PCI
;      dq sys_driver_long                                                ; 116-Driver state access
;      dq sys_audio_long                                                 ; 117-Audio
;      dq sys_midi_long                                                  ; 118-MIDI
;      dq draw_cycle_long                                                ; 119-Draw Circle
;      dq window_skinning_long                                           ; 120-Window skinning
;      dq drag_drop_long                                                 ; 121-Drag'n drop
;      dq sys_api_long                                                   ; 122-3D API
;      dq event_timeout_long                                             ; 123-Wait for event with timeout (ms)
;      dq sys_window_long                                                ; 124-Window hide, show, activate
;      dq window_transp_long                                             ; 125-Window transparency
;      dq sys_switch_long                                                ; 126-Switch process to 32bit or 64bit mode
;      dq sys_usb_info_long                                              ; 127-USB 2.0 device information
;      dq sys_usb_winterface_long                                        ; 128-USB 2.0 webcam interface
;      dq sys_usb_pinterface_long                                        ; 129-USB 2.0 printer interface
;      dq sys_usb_tinterface_long                                        ; 130-USB 2.0 tuner interface
;      dq sys_usb_ousb_long                                              ; 131-USB 1.X device information
;      dq sys_multi_long                                                 ; 140-Multi-Processor information
;      dq sys_font_long                                                  ; 141-System font
;      dq sys_copy_long                                                  ; 142-Copy/Paste
;      dq sys_gzip_long                                                  ; 143-Gzip
;      dq graphic_driver_long                                            ; 144-Graphics driver
;      dq sys_signal_long                                                ; 150-Signal processing (FFT,resampling,equalizer)
;      dq sys_mathlib_long                                               ; 151-Mathlib
;      dq sys_start_long                                                 ; 256-Start process
;      dq sys_mstart_long                                                ; 257-Start process from memory
;      dq sys_page_long                                                  ; 400-Set page type (driver only)
;      dq system_long                                                    ; 500-System
;      dq sys_terminate_long                                             ; 512-Terminate process
;
;      times (113-67-1) dq undefined_syscall
;
;      dq sys_scroll                                                     ; 113-Srolls
;
;      times 255 - ( ($-servetable) /8 )  dq undefined_syscall
;
;      dq sys_end                                                        ; -1-end application

tss_sceleton_long:

l.rsp0          dq      0
l.rsp1          dq      0
l.rsp2          dq      0
l.ist1          dq      0
l.ist2          dq      0
l.ist3          dq      0
l.ist4          dq      0
l.ist5          dq      0
l.ist6          dq      0
l.ist7          dq      0
l.io_long       dw      0

tss_register_long:

l.es_long       dw      0,0
l.cs_long       dw      0,0
l.ss_long       dw      0,0
l.ds_long       dw      0,0
l.fs_long       dw      0,0
l.gs_long       dw      0,0
l.cr3_long      dq      0
l.rip           dq      0
l.rflags        dq      0
l.rsp           dq      0
l.rbp           dq      0
l.rsi           dq      0
l.rax           dq      0
l.rbx           dq      0
l.rcx           dq      0
l.rdx           dq      0
l.rdi           dq      0

s0:
        cli

        mov     [error_interrupt_long],0x0
        mov     r8,s0
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s1:

        cli

        mov     [error_interrupt_long],0x1
        mov     r8,s1
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s2:

        cli

        mov     [error_interrupt_long],0x2
        mov     r8,s2
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s3:

        cli

        mov     [error_interrupt_long],0x3
        mov     r8,s3
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s4:

        cli

        mov     [error_interrupt_long],0x4
        mov     r8,s4
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s5:

        cli

        mov     [error_interrupt_long],0x5
        mov     r8,s5
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s6:

        cli

        mov     [error_interrupt_long],0x6
        mov     r8,s6
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

prev_user_of_fpu_long dq 0x1                                                    ; set to OS

s7:

        mov     rdi,7
        imul    rdi,8
        mov     [rdi+gdts_long+ tss0i +5],word 00010000b *256 +11101001b

        mov     rdi,[0x6000]
        imul    rdi,8
        mov     [rdi+gdts_long+ tss0 +5],word 00010000b *256 +11101001b

        mov     rsi,[0x6000]
        imul    rsi,tss_step
        mov     r9,tss_data
        add     rsi,r9
        mov     rdi,fpu_tss_long
        mov     rcx,120
        cld
        rep     movsb

        mov     rsi,[0x6000]
        imul    rsi,tss_step
        mov     r9,tss_data
        add     rsi,r9

        mov     word [rsi+l.cs_long-tss_register_long],int_code
        mov     word [rsi+l.ss_long-tss_register_long],int_data
        mov     word [rsi+l.ds_long-tss_register_long],int_data
        mov     word [rsi+l.es_long-tss_register_long],int_data
        mov     word [rsi+l.fs_long-tss_register_long],int_data
        mov     word [rsi+l.gs_long-tss_register_long],int_data
        mov     r8,fpu_stack_long+8*8
        mov     qword [rsi+l.rsp-tss_register_long],r8
        mov     r8,fpu_handler_long
        mov     qword [rsi+l.rip-tss_register_long],r8
        mov     qword [rsi+l.rflags-tss_register_long],0x11002

        mov     rbx,[0x6000]
        shl     bx,3
        add     bx,tss0call
        mov     [tss_s7_long],bx

        db      0xEA
        dq      0
tss_s7_long     dw        tss0call

        jmp     s7

fpu_tss_long:   times 128 db 0

fpu_handler_long:

        clts

        mov     rax,[prev_user_of_fpu_long]
        shl     rax,8
        add     rax,0x800000+0x10
        fsave   [rax]

        mov     rax, [0x6000]
        mov     [prev_user_of_fpu_long],rax
        shl     rax,8
        add     rax,0x800000
        cmp     [rax+0xDE],byte 0
        je      bs7_first_fpu_long
        frstor  [rax+0x10]
      bs7_first_fpu_long:
        mov     [rax+0xDE],byte 1

        movzx   rax,word [fpu_tss_long+l.ss_long-tss_register_long]            ; push ss
        push    rax
        mov     rax,[fpu_tss_long+l.rsp-tss_register_long]                     ; push rsp
        push    rax
        mov     rax,[fpu_tss_long+l.rflags-tss_register_long]                  ; push rflags
        push    rax

        movzx   rax,word [fpu_tss_long+l.cs_long-tss_register_long]            ; push cs
        push    rax
        mov     rax,[fpu_tss_long+l.rip-tss_register_long]                     ; push rip
        push    rax

        mov     rax,qword [fpu_tss_long+l.rax-tss_register_long]               ; push rax
        push    rax

        mov     ax,[fpu_tss_long+l.es_long-tss_register_long]
        mov     es,ax
        mov     ax,[fpu_tss_long+l.fs_long-tss_register_long]
        mov     fs,ax
        mov     ax,[fpu_tss_long+l.gs_long-tss_register_long]
        mov     gs,ax
        mov     ax,[fpu_tss_long+l.ds_long-tss_register_long]
        mov     ds,ax

        pop     rax

        iret

fpu_stack_long: times 6*8 db 0

s8:
        cli

        mov     [error_interrupt_long],0x8
        mov     r8,s8
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s9:
        cli

        mov     [error_interrupt_long],0x9
        mov     r8,s9
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s10:
        cli

        mov     [error_interrupt_long],0xA
        mov     r8,s10
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s11:
        cli

        mov     [error_interrupt_long],0xB
        mov     r8,s11
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s12:
        cli

        mov     [error_interrupt_long],0xC
        mov     r8,s12
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s13:
        cli

        mov     [error_interrupt_long],0xD
        mov     r8,s13
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s14:
        cli

        mov     [error_interrupt_long],0xE
        mov     r8,s14
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s15:
        cli

        mov     [error_interrupt_long],0xF
        mov     r8,s15
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s16:
        cli

        mov     [error_interrupt_long],0x10
        mov     r8,s16
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

s17:
        cli

        mov     [error_interrupt_long],0x11
        mov     r8,s17
        mov     [error_interrupt_entry_long],r8

        call    show_error_parameters_long

        mov     rdx,[0x6020]
        mov     [rdx+0x1A],byte 4

        jmp     change_task_long

writehex_long:

      pushdu

        mov     rcx,rax
        mov     ax,word [printerrorat_long]
        shl     rax,16
        mov     ax,[rsp+64+8]
        sub     ax,60
        mov     rdx,1
        mov     rsi,8
        mov     rbx,0xFFFFFFFFFFFF
    whl1_long:
        push    rcx
        and     rcx,0xF
        push    rax
        mov     rax,hexletters_long
        add     rcx,rax
        pop     rax
        mov     rdi,1

        mov     cl,[rcx]
        mov     rdi,[write_error_to_long]
        mov     [rdi],cl
        dec     [write_error_to_long]

        pop     rcx
        shr     rcx,4
        sub     rax,6*65536
        dec     rsi
        jnz     whl1_long

        popdu
        ret


hexletters_long  db '0123456789ABCDEF'

error_interrupt_long         dq  -1
error_interrupt_entry_long   dq  -1

printerrorat_long        dq 300

process_error_long  db 'Kernel : Process - Forced terminate INT: 00000000',13,10,0
process_pid_long    db 'Kernel : Process - Forced terminate PID: 00000000',13,10,0
process_rip         db 'Kernel : Process - Forced terminate RIP: 00000000',13,10,0

write_error_to_long  dq  0x0

show_error_parameters_long:

        push    rax
        mov     rax,process_pid_long+48
        mov     [write_error_to_long],rax
        pop     rax
        mov     rax,[0x6000]
        imul    rax,0x20
        mov     rax,[0x6000+8+rax]
        call    writehex_long

        push    rax
        mov     rax,process_error_long+48
        mov     [write_error_to_long],rax
        pop     rax
        mov     rax,[error_interrupt_long]
        call    writehex_long

        mov     rax,[0x6000]
        imul    rax,tss_step
        mov     rax,[rax+tss_data+l.rip-tss_register_long]
        push    rbx
        mov     rbx,process_rip+48
        mov     [write_error_to_long],rbx
        pop     rbx
        call    writehex_long

        mov     rsi,process_error_long
        call    sys_msg_board_str

        mov     rsi,process_pid_long
        call    sys_msg_board_str

        mov     rsi,process_rip
        call    sys_msg_board_str

        ret

keyboard_mode_long db 0
keyboard_data_long db 0


irq33:

        call    restore_caller_long


        xor     rax,rax
        mov     dx,0x60
        in      al,dx

        mov     [keyboard_data_long],al

        movzx   rdi,word [0x6008]                                               ; top window process
        shl     rdi,1
        add     rdi,0x18800
        movzx   rdi,word [rdi]
        imul    rdi,256
        add     rdi,0x8000E8
        mov     rdx,[rdi]

        mov     [keyboard_mode_long],dl

        cmp     al,29                                                           ; CTRL
        jne     noctrlon_long
        mov     [ctrl_long],byte 1
        jmp     nokey_long
      noctrlon_long:

        cmp     al,29+128
        jne     noctrloff_long
        mov     [ctrl_long],byte 0
        jmp     nokey_long
      noctrloff_long:

        cmp     al,56                                                           ; ALT
        jnz     noalton1_long
        mov     [alt_long],byte 1
        jmp     nokey_long
      noalton1_long:

        cmp     al,184
        jnz     noaltoff1_long
        mov     [alt_long],byte 0
        jmp     nokey_long
      noaltoff1_long:

        cmp     al,42                                                           ; SHIFT
        jnz     noshifton1_long
        or      byte [shift_long],byte 1
        jmp     nokey_long
      noshifton1_long:
        cmp     al,54
        jnz     noshifton2_long
        or      byte [shift_long],byte 2
        jmp     nokey_long
      noshifton2_long:

        cmp     al,42+128
        jnz     noshiftoff1_long
        and     byte [shift_long],byte 2
        jmp     nokey_long
      noshiftoff1_long:
        cmp     al,54+128
        jnz     noshiftoff2_long
        and     byte [shift_long],byte 1
        jmp     nokey_long
      noshiftoff2_long:

        cmp     al,byte 128
        jb      yeskey_long
        jmp     nokey_long

      yeskey_long:

        mov     rcx,rax                                                         ; plain key
        add     rcx,keymap
        mov     bl,[rcx]

        cmp     [alt_long],1                                                    ; ctrl alt del
        jne     noctrlaltdel_long
        cmp     [ctrl_long],1
        jne     noctrlaltdel_long
        cmp     bl,134+48
        jne     noctrlaltdel_long
        mov     [ctrl_alt_del_long],1
        jmp     nokey_long

      ctrl_alt_del_long db 0

      noctrlaltdel_long:

        cmp     [ctrl_long],byte 1                                              ; ctrl on ?
        jne     nco_long
        sub     bl,96
      nco_long:

        cmp     [shift_long],byte 0                                             ; shift on ?
        je      nso_long
        mov     rcx,rax
        add     rcx,keymap_shift
        mov     bl,[rcx]
      nso_long:

        cmp     [alt_long],byte 1                                               ; alt on ?
        jz      yao_long
        jmp     nao_long
      yao_long:
        mov     rcx,rax
        add     rcx,keymap_alt
        mov     bl,[rcx]

        ; alt mouse ?

        xor     rdx,rdx
        cmp     bl,178
        jnz     noaltup_long
        mov     rdx,5*65536
        mov     bl,255
      noaltup_long:
        cmp     bl,177
        jnz     noaltdown_long
        mov     rdx,251*65536
        mov     bl,255
      noaltdown_long:
        cmp     bl,176
        jnz     noaltleft_long
        mov     rdx,251*256
        mov     bl,255
      noaltleft_long:
        cmp     bl,179
        jnz     noaltright_long
        mov     rdx,5*256
        mov     bl,255
      noaltright_long:
        cmp     bl,' '
        jnz     noaltmbd_long
        mov     bl,[altmouseb_long]
        add     bl,1
        and     bl,1
        mov     [altmouseb_long],bl
        mov     bl,255
      noaltmbd_long:
        cmp     bl,255
        jnz     nao_long
        mov     [0x1EC08],byte 1                                                 ; ps2 data
        mov     [0x1F600],byte 0                                                 ; ps2 chunk count
        mov     [0x5C0000+4096*24],word 3                                        ; mouse data count
        movzx   rbx,byte [altmouseb_long]
        or      rdx,rbx
        mov     [0x5C0000+4096*24+0x20],rdx
        mov     bl,0
      nao_long:

        cmp     [keyboard_mode_long],0                                           ; return from keymap
        jne     nokey_long

        cmp     bl,0
        jz      nokey_long

        movzx   rdx,byte [0x1E800]
        cmp     rdx,120
        jge     nokey_long
        inc     rdx
        mov     [0x1E800],dl
        mov     [0x1E800+rdx],bl

      nokey_long:

        cmp     [keyboard_mode_long],1                                           ; return scancode
        jne     no_scancode_long

        movzx   rdx,byte [0x1E800]
        cmp     rdx,120
        jge     no_scancode_long

        inc     rdx
        mov     [0x1E800],dl

        mov     al,[keyboard_data_long]
        mov     [0x1E800+rdx],al

      no_scancode_long:


        mov     [check_idle_semaphore],5

        mov     al,0x20                                                          ; ready for next irq
        mov     dx,0x20
        out     dx,al

        call    return_to_caller_long

        jmp     irq33


shift_long      db  0x0
ctrl_long       db  0x0
alt_long        db  0x0
altgr_long      db  0x0
altmouseb_long  db  0x0

irq47:

        call    restore_caller_long

        mov     dx,word [sb16]
        add     dx,0xe
        in      al,dx

;       mov     byte [SB16_Status],0

        mov     [check_idle_semaphore],5

        mov     al,0x20
        out     0x20,al

        call    return_to_caller_long

        jmp     irq33

irq45:

        call    restore_caller_long

        mov     dx,0xf0
        mov     al,0
        out     dx,al

        mov     dx,0xa0
        mov     al,0x20
        out     dx,al
        mov     dx,0x20
        out     dx,al

        mov     ds,cx
        mov     es,cx
        mov     fs,cx

        call    return_to_caller_long

        jmp     irq45

p_irq34:

        call    restore_caller_long

        mov     rdi,2                                                            ; 1
        call    irqhandler_long                                                  ; 2/5

        call    return_to_caller_long

        jmp     p_irq34

p_irq35:

        call    restore_caller_long

        mov     rdi,3
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq35

p_irq36:

        call    restore_caller_long

        mov     rdi,4
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq36

p_irq37:

        call    restore_caller_long

        mov     rdi,5
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq37

p_irq38:

        call    restore_caller_long

        call    fdc_irq

        mov     rdi,6
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq38

p_irq39:

        call    restore_caller_long

        mov     rdi,7
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq39

p_irq40:

        call    restore_caller_long

        mov     rdi,8
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq40

p_irq41:

        call    restore_caller_long

        mov     rdi,9
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq41

p_irq42:

        call    restore_caller_long

        mov     rdi,10
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq42

p_irq43:

        call    restore_caller_long

        mov     rdi,11
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq43

p_irq44:

        call    restore_caller_long

        mov     rdi,12
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq44

p_irq45:

        call    restore_caller_long

        mov     rdi,13
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq45

p_irq46:

        call    restore_caller_long

        mov     rdi,14
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq46

p_irq47:

        call    restore_caller_long

        mov     rdi,15
        call    irqhandler_long

        call    return_to_caller_long

        jmp     p_irq47

restore_caller_long:

        mov     rdi,[0x6000]
        imul    rdi,8
        mov     [rdi+gdts_long+ tss0 +5], word 01010000b *256 +11101001b

        ret

return_to_caller_long:

        mov     rbx,[0x6000]
        shl     bx,3
        add     bx,tss0call
        mov     [tss_irq12_long],bx

           db   0xEA
           dq   0
tss_irq12_long  dw    tss0call

        ret

irqh_long  dq   0x0

irqhandler_long:

        push    rdi

        mov     rsi,rdi                          ; 1
        shl     rsi,6                            ; 1
        add     rsi,irq00read                    ; 1
        shl     rdi,12                           ; 1
        add     rdi,0x5C0000

        mov     [check_idle_semaphore],5

   irqnewread_long:

        mov     dx,[rsi]                         ; 2+

        cmp     dx,0                             ; 1
        jz      irqover_long
        cmp     [rsi+7],byte 1                   ; 2     ; byte read
        jne     noirqbyte_long                   ; 4-11

        in      al,dx

        mov     rdx,[rdi]
        cmp     rdx,4000
        je      irqfull_long
        mov     rbx,rdi
        add     rbx,0x10
        add     rbx,rdx
        mov     [rbx],al
        add     rdx,1
        mov     [rdi],rdx

        add     rsi,4
        jmp     irqnewread_long

   noirqbyte_long:

        cmp     [rsi+3],byte 2                   ; word read
        jne     noirqword_long

        in      ax,dx

        mov     rdx,[rdi]
        cmp     rdx,4000
        je      irqfull_long
        mov     rbx,rdi
        add     rbx,0x10
        add     rbx,rdx
        mov     [rbx],ax
        add     rdx,2
        mov     [rdi],rdx
        add     rsi,4
        jmp     irqnewread_long

   noirqword_long:
   irqfull_long:
   irqover_long:

        mov     al,0x20                          ; ready for next irq
        out     0x20,al

        pop     rbx
        cmp     rbx,7
        jbe     noa0_long
        out     0xa0,al
   noa0_long:

        ret

i_unknown16:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown17:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown18:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown19:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown20:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown21:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown22:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown23:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown24:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown25:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown26:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown27:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown28:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown29:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown30:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown31:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown48:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown49:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown50:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown51:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown52:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown53:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown54:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown55:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown56:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown57:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown58:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown59:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown60:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown61:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown62:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown63:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown64:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown65:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown66:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown67:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown68:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown69:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown70:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown71:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown72:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown73:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown74:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown75:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown76:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown77:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown78:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown79:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown80:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown81:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown82:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown83:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown84:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown85:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown86:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown87:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown88:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown89:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown90:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown91:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown92:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown93:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown94:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown95:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long



i_unknown96:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown97:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown98:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown99:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown101:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown102:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown103:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown104:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown105:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown106:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown107:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown108:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown109:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown110:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown111:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown112:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown113:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown114:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown115:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown116:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown117:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown118:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown119:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown120:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown121:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown122:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown123:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown124:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown125:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown126:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown127:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown128:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown129:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown130:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown131:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown132:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown133:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown134:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown135:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown136:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown137:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown138:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown139:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown140:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown141:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown142:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown143:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown144:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown145:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown146:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown147:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown148:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown149:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown150:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown151:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown152:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown153:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown154:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown155:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown156:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown157:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown158:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown159:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown160:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown161:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown162:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown163:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown164:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown165:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown166:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown167:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown168:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown169:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown170:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown171:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown172:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown173:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown174:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown175:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown176:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown177:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown178:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown179:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown180:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown181:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown182:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown183:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown184:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown185:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown186:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown187:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown188:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown189:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown190:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown191:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown192:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown193:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown194:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown195:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown196:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown197:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown198:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown199:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown200:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown201:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown202:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown203:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown204:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown205:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown206:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown207:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown208:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown209:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown210:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown211:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown212:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown213:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown214:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown215:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown216:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown217:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown218:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown219:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown220:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown221:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown222:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown223:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown224:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown225:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown226:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown227:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown228:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown229:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown230:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown231:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown232:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown233:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown234:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown235:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown236:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown237:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown238:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown239:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown240:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long


i_unknown241:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown242:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown243:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown244:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown245:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown246:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown247:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown248:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown249:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown250:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown251:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown252:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown253:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown254:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long
i_unknown255:
        mov     rax,$
        push    rax
        jmp     i_u_errorhandle_long

i_u_errorhandle_long:

        cli

        pop     rax

        sti

        jmp     $

compare_to_thread_long:

        push    rbx

        mov     rax,rdx
        imul    rax,8
        add     rax,gdts_long+app_code-3
        mov     rbx,[rax]
        cmp     rbx,[old_code]
        jne     ctt0_long

        pop     rbx
        mov     rax,1
        ret

       ctt0_long:

        pop     rbx
        mov     rax,0
        ret



check_for_thread_mem_long:

        pushdu

        mov     rcx,[0x6008]
       cftm0_long:
        mov     rax,rcx
        imul    rax,8
        add     rax,gdts_long+ app_code-3
        mov     rbx,[rax]
        cmp     rbx,[old_code]
        jne     cftm1_long

        mov     rax,rcx                                   ; new code segments
        imul    rax,8
        add     rax,gdts_long+ app_code-3

        mov     rbx,[new_code]
        mov     [rax],rbx

        mov     rax,rcx                                   ; new data segments
        imul    rax,8
        add     rax,gdts_long+ app_code-3

        mov     rbx,[new_data]
        mov     [rax],rbx

        cmp     [new_pos],0                            ; new memory position segments
        je      no_new_postition_for_thread_long
        mov     rax,rcx
        imul    rax,32
        add     rax,0x6000
        mov     rbx,[new_pos]
        mov     [rax+0x20],rbx
       no_new_postition_for_thread_long:

        mov     rax,rcx                                   ; new amount of memory
        imul    rax,256
        add     rax,0x800000
        mov     rbx,[new_amount]
        mov     [rax+0x98],rbx

       cftm1_long:

        dec     rcx
        jnz     cftm0_long

        popdu

        ret


save_for_thread_check_long:

        ; save for thread check

        pushdu
        mov     rsi,[0x6000]
        imul    rsi,8
        add     rsi,gdts_long+ app_code-3 +0
        lea     rdi,dword [old_code+0]
        mov     rcx,8
        cld
        rep     movsb
        popdu

        ret


save_new_position_for_threads_long:

        ; new code segment for thread check
        pushdu
        mov     rsi,[0x6000]
        imul    rsi,8
        add     rsi,gdts_long+ app_code-3 +0
        lea     rdi,dword [new_code+0]
        mov     rcx,8
        cld
        rep     movsb
        popdu

        ; new data segment for thread check
        pushdu
        mov     esi,[0x6000]
        imul    esi,8
        add     esi,gdts_long+ app_data-3 +0
        lea     edi,dword [new_data+0]
        mov     ecx,8
        cld
        rep     movsb
        popdu

        ret


set_application_table_status_long:

        push    rax

        mov     rax,[0x6000]
        imul    rax,32
        add     rax,0x6000+8
        mov     rax,[rax]

        mov     [application_table_status],rax

        pop     rax

        ret

clear_application_table_status_long:

        push    rax

        mov     rax,[0x6000]
        imul    rax,32
        add     rax,0x6000+8
        mov     rax,[rax]

        cmp     rax,[application_table_status]
        jne     apptsl1_long
        mov     [application_table_status],0
      apptsl1_long:

        pop     rax

        ret


old_code dq 0x0

new_code dq 0x0

new_data dq 0x0

new_pos    dq 0x0
new_amount dq 0x0


sys_resize_app_memory_long:

        ; rax = 1 - resize
        ;     rbx = new amount of memory

        cmp     rax,1
        jne     no_application_mem_resize_long

        add     rbx,4095
        shr     rbx,12
        shl     rbx,12
        mov     rbp,rbx

        ; wait for process table to be free

      rsm0_long:

        cli
        cmp     [application_table_status],0
        je      rsm1_long
        sti
        call    change_task_long
        jmp     rsm0_long

      rsm1_long:

        call    set_application_table_status_long
        sti

        cmp     rbx,0                                            ; other than zero
        je      mem_resize_unsuccess_long

        call    save_for_thread_check_long

        ; find a free place

        mov     rsi,[0x1FD08]                                    ; application memory start
        mov     rdi,rbp
        add     rdi,rsi
        dec     rdi

      rfgdt_long:

        mov     rdx,2

      rfindgdtl1_long:

        call    compare_to_thread_long
        cmp     rax,1
        je      rfindfl3_long

        mov     rcx,rdx
        shl     rcx,3

        ; rax run base -> rbx limit

        mov     al,[rcx+gdts_long+ app_code-3 +4]
        mov     ah,[rcx+gdts_long+ app_code-3 +7]
        shl     rax,16
        mov     ax,[rcx+gdts_long+ app_code-3 +2]

        movzx   rbx,word [rcx+gdts_long+ app_code-3 +0]
        shl     rbx,12
        add     rbx,rax

        cmp     rax,rdi
        jg      rfindfl3_long
        cmp     rbx,rsi
        jb      rfindfl3_long

        add     rsi,4096
        add     rdi,4096

        cmp     rdi,[0x1FD18]                                    ; < c_memory
        jbe     rfgdt_long

        jmp     rfind_free_ret_2_long                            ; not enough memory

      rfindfl3_long:

        inc     rdx
        cmp     rdx,[0x6008]
        jbe     rfindgdtl1_long

     rfindfl1_long:
     rthread_c_long:

        mov     rcx,[0x6000]
        shl     rcx,3

        inc     rdi
        sub     rdi,rsi
        add     rdi,4095
        shr     rdi,12
        dec     rdi

        ; code

        mov     rax,rsi
        mov     rbx,rdi

        mov     [rcx+gdts_long+ app_code-3 +2],ax             ;  base  0:15
        shr     rax,16
        mov     [rcx+gdts_long+ app_code-3 +4],al             ;  base  23:16
        mov     [rcx+gdts_long+ app_code-3 +7],ah             ;  base  31:24
        mov     [rcx+gdts_long+ app_code-3 +0],bx             ;  limit

        ; data

        mov     rax,rsi
        mov     [rcx+gdts_long+ app_data-3 +2],ax             ;  base  0:15
        shr     rax,16
        mov     [rcx+gdts_long+ app_data-3 +4],al             ;  base  23:16
        mov     [rcx+gdts_long+ app_data-3 +7],ah             ;  base  31:24

        movzx   rdx,word [rcx+gdts_long+ app_code-3 +0]       ;  save limit

        mov     [rcx+gdts_long+ app_data-3 +0],bx             ;  limit

        and     rbx,0xFFFF

        cmp     rbx,rdx                                          ; copy smaller from memory sizes
        jge     noedxebxxchg_long
        mov     rdx,rbx
      noedxebxxchg_long:

        movzx   rcx,dx
        shl     rcx,12
        add     rcx,4096

        mov     rdi,rsi

        mov     rax,[0x6020]
        mov     rsi,[rax+0x46]

        mov     [rax+0x46],rdi                                   ; new memory position

        mov     rax,[0x6000]                                     ; new memory size
        shl     rax,8
        add     rax,0x800000
        mov     [rax+0x98],rbp

        mov     [new_pos],rdi                               ; new position for threads
        mov     [new_amount],rbp                            ; new amount of mem for threads

        cmp     rsi,rdi
        je      no_app_move_long

        cld
        rep     movsb                                            ; move the app image to the new position

      no_app_move_long:

        call    save_new_position_for_threads_long
        call    check_for_thread_mem_long

        mov     [application_table_status],0

        mov     [rsp+72],dword 0                                       ; rax <- 0 ; successfull
        mov     [rsp+72+4],dword 0

        ret

     rfind_free_ret_2_long:

     mem_resize_unsuccess_long:

        mov     [application_table_status],0

        mov     [rsp+72],dword 1                                       ; rax <- 1 ; unsuccessfull
        mov     [rsp+72+4],dword 0

        ret

     no_application_mem_resize_long:


        ret



find_free_mem_long:

        push    rax
        push    rbx
        push    rcx
        push    rdx
        push    rdi

        call    find_free_process_slot_long
        mov     rax,[new_process_place_long]

        cmp     rax,max_processes
        jg      find_free_ret_2_long

        cmp     [thread_create_long],1
        je      thread_c_long

        mov     rsi,[0x1FD08]
        add     rdi,rsi
        dec     rdi

        mov     rax,2
        cmp     qword [0x6008],1
        je      findf4_long

     fgdt_long:

        mov     rdx,2

     findgdtl1_long:

        mov     rcx,rdx
        shl     rcx,3

        ; rax run base -> ebx limit

        mov     al,[rcx+gdts_long+ app_code-3 +4]
        mov     ah,[rcx+gdts_long+ app_code-3 +7]
        shl     rax,16
        mov     ax,[rcx+gdts_long+ app_code-3 +2]

        movzx   rbx,word [rcx+gdts_long+ app_code-3 +0]
        shl     rbx,12
        add     rbx,rax

        cmp     rax,rdi
        jg      findfl3_long
        cmp     rbx,rsi
        jb      findfl3_long

        add     rsi,4096
        add     rdi,4096

        cmp     rdi,[0x1FD18]                                                    ; < c_memory
        jbe     fgdt_long

        jmp     find_free_ret_2_long

     findfl3_long:

        inc     rdx
        cmp     rdx,[check_processes_long]
        jbe     findgdtl1_long

     findfl1_long:
     thread_c_long:

         mov    rax,[new_process_place_long]

     findf4_long:

        mov     qword [first_gdt_search_long],rax
        mov     [gdt_place_long],rax

        mov     rcx,rax
        shl     rcx,3

        inc     rdi
        sub     rdi,rsi
        add     rdi,4095
        shr     rdi,12
        dec     rdi

        ; code

        mov     rax,rsi
        mov     rbx,rdi

        mov     [rcx+gdts_long+ app_code-3 +2],ax                            ;  base  0:15
        shr     rax,16
        mov     [rcx+gdts_long+ app_code-3 +4],al                            ;  base  23:16
        mov     [rcx+gdts_long+ app_code-3 +7],ah                            ;  base  31:24
        mov     [rcx+gdts_long+ app_code-3 +0],bx                            ;  limit
        mov     [rcx+gdts_long+ app_code-3 +5],word 11010000b *256 +11111010b

        ; data

        mov     rax,rsi
        mov     [rcx+gdts_long+ app_data-3 +2],ax                            ;  base  0:15
        shr     rax,16
        mov     [rcx+gdts_long+ app_data-3 +4],al                            ;  base  23:16
        mov     [rcx+gdts_long+ app_data-3 +7],ah                            ;  base  31:24
        mov     [rcx+gdts_long+ app_data-3 +0],bx                            ;  limit
        mov     [rcx+gdts_long+ app_data-3 +5],word 11010000b *256 +11110010b

;        push   rsi
;        mov    rsi,process_loading_long
;        call   sys_msg_board_str_long
;        pop    rsi

      find_free_ret_long:

        pop     rdi
        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax
        ret

      find_free_ret_2_long:

        pop     rdi
        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax
        mov     rsi,0
        ret


get_app_params_long:

        push    rax

        cmp     [0x120000+12],dword '00'
        jne     no_00_header_long

        mov     rax,[0x120000+24]
        mov     [app_start_long],rax
        mov     rax,[0x120000+32]
        mov     [app_i_end_long],rax
        mov     rax,[0x120000+40]
        mov     [app_mem_long],rax
        shr     rax,1
        sub     rax,0x20
        mov     [app_rsp],rax
        mov     rax,[0x120000+48]
        mov     [app_i_param_long],rax
        mov     [app_i_icon_long],0

        pop     rax
        mov     rsi,1
        ret

  no_00_header_long:


        cmp     [0x120000+12],dword '01'
        jne     no_01_header_long

        mov     rax,[0x120000+24]
        mov     [app_start_long],rax
        mov     rax,[0x120000+32]
        mov     [app_i_end_long],rax
        mov     rax,[0x120000+40]
        mov     [app_mem_long],rax
        mov     rax,[0x120000+48]
        mov     [app_rsp],rax
        mov     rax,[0x120000+56]
        mov     [app_i_param_long],rax
        mov     rax,[0x120000+64]
        mov     [app_i_icon_long],rax

        pop     rax
        mov     rsi,1
        ret

   no_01_header_long:

        pop     rax
        mov     rsi,0
        ret


start_application_fl:

        ; rax - pointer to filename
        ; rbx - parameters to pass

        cli
        cmp     [application_table_status],0
        je      stfl9_long
        sti
        call    change_task_long
        jmp     start_application_fl
  stfl9_long:

        call    set_application_table_status_long

        mov     [thread_create_long],0

        push    rbx
        push    rax

        mov     rbx,1
        mov     rcx,2
        mov     rdx,0x120000
        mov     rsi,12
        cli
        call    fileread
        cli
        cmp     rax,0
        jne     noflrun_long
        cmp     [0x120000+0],dword 'TENS'
        jnz     noflrun_long
        cmp     [0x120000+4],word  'OR'
        jnz     noflrun_long
        call    get_app_params_long
        cmp     rsi,0
        je      noflrun_long
        mov     rdi,[app_mem_long]
        call    find_free_mem_long
        cmp     rsi,0
        jne     start_app_c_long

  noflrun_long:

        pop     rax
        pop     rbx
        mov     rax,-1
        mov     [application_table_status],0
        sti
        retq

  start_app_c_long:

        mov     [app_mem_pos_long],rsi

        mov     rdi,[app_mem_pos_long]                                          ; clear run location
        mov     rcx,[app_mem_long]
        mov     rax,0x0
        cld
        rep     stosb

        mov     rax,[rsp+0]                                                     ; read all of file to it's run location
        mov     rbx,0
        mov     rcx,-1
        mov     rdx,[app_mem_pos_long]
        mov     rsi,12
        cli
        call    fileread
        cli

        jmp     add_app_parameters_long


start_application_hd:

        ; rax - file name, abs address
        ; rbx - file length
        ; rcx - work area, abs address
        ; rbp - parameters to pass

        cli
        cmp     [application_table_status],0
        je      sthd9_long
        sti
        call    change_task_long
        jmp     start_application_hd
  sthd9_long:

        call    set_application_table_status_long

        sti

        mov     [thread_create_long],0

        push    rbx                                                             ; file length
        push    rax                                                             ; file name    - abs address
        push    rcx                                                             ; work area    - abs address

        mov     rsi,rcx

        mov     rcx,1
        mov     rdx,1

        pushdu
        call    read_hd_file
        cmp     rax,0
        je      rhdfl5_long
        popdu

        add     rsp,4*3
        mov     rax,1
        neg     rax
        mov     [application_table_status],0
        ret

  rhdfl5_long:
        popdu

        cmp     [rsi+1024+0],dword 'TENS'
        jnz     nohdrun_long
        cmp     [rsi+1024+4],word  'OR'
        jnz     nohdrun_long

        add     rsi,1024
        mov     rdi,0x120000
        mov     rcx,512/8
        cld
        rep     movsq

        call    get_app_params_long
        cmp     rsi,0
        je      nohdrun_long

        mov     rdi,[app_mem_long]
        call    find_free_mem_long
        cmp     rsi,0
        jne     read_file_to_place_long

  nohdrun_long:

        add     rsp,4*3
        mov     rax,1
        mov     [application_table_status],0
        sti
        ret

  read_file_to_place_long:

        cli

        mov     [thread_create_long],0
        mov     [app_mem_pos_long],rsi

        pop     rsi

        mov     rdi,[app_mem_pos_long]                                          ; clear run location
        mov     rcx,[app_mem_long]
        mov     rax,0x0
        cld
        rep     stosb

        mov     rcx,1                                                           ; read the file to its run-location

  hdnw_long:

        push    rcx
        push    rsi
        mov     rax,[rsp+16]
        mov     rbx,[rsp+24]
        mov     rdx,1
        call    read_hd_file
        pop     rsi
        pop     rcx
        cmp     rax,0
        jne     stnshd_long

        push    rcx
        mov     rax,rsi
        add     rax,1024
        mov     rbx,rcx
        sub     rbx,1
        shl     rbx,9
        add     rbx,[app_mem_pos_long]
        mov     rcx,512
        call    memmove
        pop     rcx

        add     rcx,1
        jmp     hdnw_long

  stnshd_long:

        cli

        pop     rsi rdi  ; pos - len

        add     rsi , rdi

        inc     rdi
  newesidec_long:
        dec     rdi
        jz      nonewesidec_long
        dec     rsi
        cmp     [rsi],byte '/'
        jne     newesidec_long
  nonewesidec_long:
        inc     rsi

        push    rbp  ; qword 0
        push    rsi  ; name position

        jmp     add_app_parameters_long


threadstring_long       dq  0x0
new_process_place_long  dq  0x0
check_processes_long    dq  0x0

app_start_long    dq  0x0
app_i_end_long    dq  0x0
app_mem_long      dq  0x0
app_rsp           dq  0x0
app_i_param_long  dq  0x0
app_i_icon_long   dq  0x0
app_mem_pos_long  dq  0x0

process_loading_long  db 'Kernel : Process - loading ',13,10,0
process_running_long  db 'Kernel : Process - done',13,10,0
first_gdt_search_long dq 0x2
thread_create_long    dq 0x0
gdt_place_long        dq 0x0


sys_threads_long:

; rax=1 create thread
;
;   rbx=thread start
;   rcx=thread stack value
;
; on return : rax = pid

        cli
        cmp     [application_table_status],0
        je      stth9_long
        sti
        call    change_task_long
        jmp     sys_threads_long
  stth9_long:

        call    set_application_table_status_long

        sti

        cmp     rax,1
        jne     no_sys_thread_create_long
        cli

        mov     rax,[0x6020]
        mov     rax,[rax+0x20]
        mov     [app_mem_pos_long],rax

        mov     [app_i_param_long],0
        mov     [app_i_icon_long],0

        mov     [app_start_long],rbx
        mov     [app_rsp],rcx

        mov     rbx,[0x6000]
        shl     rbx,8
        add     rbx,0x800000
        mov     [threadstring_long],rbx
        mov     rbx,[rbx+0x98]
        mov     [app_mem_long],rbx

        mov     rsi,[app_mem_pos_long]
        mov     rdi,[app_mem_long]
        add     rdi,rsi
        dec     rdi
        mov     [thread_create_long],1
        call    find_free_mem_long
        cmp     rsi,0
        jne     th_cr1_long
        mov     [application_table_status],0
        mov     rax,1                                                           ; no free memory
        sti
        ret
  th_cr1_long:
        mov     rdx,[threadstring_long]
        sub     rsp,2
        mov     [rsp],word 0
        sub     rsp,8
        mov     [rsp],rdx


        jmp     add_app_parameters_long
  no_sys_thread_create_long:

        mov     rax,-1
        mov     [application_table_status],0
        ret


find_free_process_slot_long:

        pushdu

        mov     rbx,[0x6008]
        mov     [check_processes_long],rbx
        inc     rbx
        mov     [new_process_place_long],rbx

        mov     rbx,2

  newfps_long:

        mov     rax,rbx
        imul    rax,0x50
        add     rax,0x6000+0xA
        cmp     [rax],byte 9
        je      ffpl_long

        inc     rbx
        cmp     rbx,[0x6008]
        jbe     newfps_long

        cmp     qword [0x6008], max_processes
        jae     noaddnewp_long

        mov     rbx,[0x6008]
        add     rbx,1
        imul    rbx,0x50
        mov     [0x6000+rbx+0xA],byte 9

        add     qword [0x6008],1
        mov     rbx,[0x6008]
        mov     [check_processes_long],rbx
        mov     [new_process_place_long],rbx

  noaddnewp_long:

        popdu
        ret

  ffpl_long:

        mov     [new_process_place_long],rbx

        popdu
        ret




add_app_parameters_long:

; returns:  rax = pid or -1 if unsuccesfull

        cmp     [app_i_param_long], 0                                           ; parameter
        jz      no_app_params_long
        mov     rax,0
        mov     rdi,[app_i_param_long]
        add     rdi,[app_mem_pos_long]
        mov     rcx,256/4
        cld
        rep     stosq
        mov     rsi,[rsp+8]
        cmp     rsi,0
        jz      no_app_params_long
        mov     rax,[app_i_param_long]
        add     rax,[app_mem_pos_long]
        mov     rdi,rax
        mov     rcx,256
        cld
  app_new_param_long:
        cmp     [rsi],byte 0
        jz      no_app_params_long
        movsb
        loop    app_new_param_long
  no_app_params_long:

        mov     rbx,[new_process_place_long]

        mov     rdi,rbx                                                         ; clear 0x80000 (256 bytes)
        shl     rdi,8
        add     rdi,0x800000
        mov     rcx,256 / 4
        mov     rax,0
        cld
        rep     stosq

        shl     rbx,6                                                           ; * 64 +0x6000
        add     rbx,0x6000

        mov     al,byte [new_process_place_long]                                ; screen id ?
        mov     [rbx+0x22],al

        mov     [rbx],dword 1+2+4                                               ; report events: windowdraw, key, button

        inc     [process_number]                                                ; process id number
        mov     rax,[process_number]
        mov     [rbx+8],rax

        mov     rcx,rbx                                                         ; set draw limits
        add     rcx,draw_data-0x6000
        mov     qword [rcx+0], 0
        mov     rax,[0x1FC00]
        mov     [rcx+8],rax
        mov     rax,[0x1FC08]
        mov     [rcx+16],rax

        mov     rax,[app_mem_pos_long]                                          ; position in memory
        mov     [rbx+0x26],rax

        ; TSS

        mov     rax,cr3
        mov     [l.cr3_long],rax
        mov     rax,[app_start_long]
        mov     [l.rip],rax
        mov     rax,[app_rsp]
        mov     [l.rsp],rax

        mov     rbx,[new_process_place_long]                                    ; GDT's
        shl     rbx,3

        mov     ax,app_code
        add     ax,bx
        mov     [l.cs_long],ax
        mov     ax,app_data
        add     ax,bx
        mov     [l.ss_long],ax
        mov     [l.ds_long],ax
        mov     [l.es_long],ax
        mov     [l.fs_long],ax
        mov     ax,graph_data
        mov     [l.gs_long],ax
        mov     [l.io_long],word 128
        mov     [l.rflags],0x11202
        mov     [l.rsp0+0],os_data
        mov     [l.rsp1+0],ring1_data
        mov     [l.rsp2+0],ring2_data
        mov     [l.rsp0+4],0x55000
        mov     [l.rsp1+4],0x56000
        mov     [l.rsp2+4],0x57000
        mov     [l.ist1],0x62000
        mov     [l.ist2],0x64000
        mov     [l.ist3],0x66000
        mov     [l.ist4],0x68000
        mov     [l.ist5],0x70000
        mov     [l.ist6],0x72000
        mov     [l.ist7],0x74000

        mov     rax,tss_sceleton_long                                           ; move tss to tss_data_long+
        mov     rbx,[new_process_place_long]
        imul    rbx,tss_step
        mov     r9,tss_data
        add     rbx,r9
        mov     rcx,120
        call    memmove

        ; Add IO access table

        mov     rax,0xFF
        mov     rdi,[new_process_place_long]
        imul    rdi,tss_step
        mov     r9,tss_data
        add     rdi,r9
        add     rdi,128
        mov     rcx,8192                                                        ; for 8192 * 8 bits = 16384 ports
        cld
        rep     stosb

        ; make sure GDT is pointing to the process
        ; and not to i64 handler

        mov     rcx,rbx
        mov     rdi,[new_process_place_long]
        imul    rdi,8

        mov     [rdi+gdts_long+ tss0 +0], word tss_step                         ; limit 0:15
        mov     [rdi+gdts_long+ tss0 +2], cx                                    ; base  0:15
        mov     rax,rcx
        shr     rax, 16
        mov     [rdi+gdts_long+ tss0 +4], al                                    ; base  23:16
        mov     [rdi+gdts_long+ tss0 +7], ah                                    ; base  31:24
        shr     rax, 16
        mov     [rdi+gdts_long+ tss0 +8], eax
        mov     [rdi+gdts_long+ tss0 +5], word 01010000b *256 +11101001b
        mov     [rdi+gdts_long+ tss0 +12],dword 0

        pop     rax
        pop     rbx

        mov     rbx,[new_process_place_long]                                    ; save name of the process
        shl     rbx,8
        add     rbx,0x80000
        mov     rcx,11
    ;    call  memmove_long

        mov     rbx,[new_process_place_long]                                    ; save image size
        shl     rbx,8
        add     rbx,0x800000+0x98
        mov     rax,[app_mem_long]
        mov     [rbx],rax

        mov     [0x1E800],byte 0                                                ; empty keyboard buffer
        mov     [0x1EA00],byte 0                                                ; empty button buffer

        mov     [application_table_status],0
        mov     rax,[process_number]

        mov     rbx,[new_process_place_long]
        imul    rbx,0x40
        mov     [0x6000+rbx+0x1A],byte 0

        mov     rdi,[new_process_place_long]
        shl     rdi,5
        add     rdi,window_data
        mov     rbx,[new_process_place_long]
        shl     rbx,1
        add     rbx,0x18000
        mov     rsi,[rbx]
        and     rsi,65535
        shl     rsi,1
        add     rsi,0x18000
        movzx   rax,word [rsi]
        call    windowactivate

        cmp     [boot_application_load],byte 1                                  ; for AMD64
        je      no_boot_enable_int_long
        sti
  no_boot_enable_int_long:

        push    rsi
        mov     rsi,process_running_long
        call    sys_msg_board_str
        pop     rsi

        ret

process_terminating_long   db 'Kernel : Process - terminating',13,10,0
process_terminated_long    db 'Kernel : Process - done',13,10,0

terminate_long:                                                                 ; terminate application

        push    rsi
        mov     rsi,process_terminating_long
        call    sys_msg_board_str
        pop     rsi

        cli
        cmp     [application_table_status],0
        je      term9_long
        sti

        ret

  term9_long:

        call    set_application_table_status_long

        mov     [first_gdt_search_long],0x2                                     ; start gdt search from beginning

        cmp     [prev_user_of_fpu_long],rsi                                     ; if user fpu last -> fpu user = 1
        jne     fpu_ok_1_long
        mov     [prev_user_of_fpu_long],1
  fpu_ok_1_long:

        mov     [0x1E800],byte 0                                                ; empty keyboard buffer
        mov     [0x1EA00],byte 0                                                ; empty button buffer

        mov     rcx,rsi                                                         ; clear memory reserv.
        shl     rcx,3
        mov     [rcx+gdts_long+ app_code-3 +0],dword 0
        mov     [rcx+gdts_long+ app_code-3 +4],dword 0
        mov     [rcx+gdts_long+ app_data-3 +0],dword 0
        mov     [rcx+gdts_long+ app_data-3 +4],dword 0

        mov     rdi,rsi
        imul    rdi,0x40
        add     rdi,0x6000
        cmp     [rdi+0x1A],byte 3                                               ; if normal terminate then clear int64 handler
        jne     nocl64

        mov     rdi,rsi                                                         ; free the used interrupt 0x64 handler
        imul    rdi,256
        mov     rax,[rdi+0x800000+0xE0]

        cmp     rax,0                                                           ; is application using a systemcall interrupt  ?
        je      nocl64

        mov     [usedi64+eax],byte 0

        mov     rdi,8
        imul    rdi,rax
        mov     [rdi+tss0sys_l_lo +5], word 01010000b *256 +11101001b

        mov     rdi,128
        imul    rdi,rax
        push    r8
        mov     r8,i64
        mov     [rdi+0x530000+l.rip-tss_register_long], r8
        pop     r8
        mov     [rdi+0x530000+l.rflags-tss_register_long], 0x11002

        mov     rbx,rax
        imul    rbx,4096
        add     rbx,sysint_stack_data
        mov     [rdi+0x530000+l.rsp-tss_register_long],rbx

  nocl64:

        mov     rcx,rsi                                                         ; remove buttons
  bnewba2_long:
        mov     rdi,[0x1FD10]
        mov     rax,rdi
        cld
        movzx   rbx,word [rdi]
        inc     bx
  bnewba_long:
        dec     bx
        jz      bnmba_long
        add     rax,0x20
        cmp     cx,[rax]
        jnz     bnewba_long
        pushdu
        mov     rcx,rbx
        inc     rcx
        shl     rcx,4
        mov     rbx,rax
        add     rax,0x10
        call    memmove
        dec     qword [rdi]
        popdu
        jmp     bnewba2_long
  bnmba_long:

        pushdu                                                                  ; save window coordinates for window restoring
        cld
        shl     rsi,5
        add     rsi,window_data
        mov     rax,[rsi+0]
        mov     qword [dlx],rax
        mov     rbx,[rsi+16]
        add     rax,rbx
        mov     qword [dlxe],rax
        mov     rax,[rsi+8]
        mov     qword [dly],rax
        mov     rbx,[rsi+24]
        add     rax,rbx
        mov     qword [dlye],rax
        mov     [rsi+0],dword 0
        mov     [rsi+16],dword 5
        mov     rax,[0x1FC08]
        mov     [rsi+8],eax
        mov     [rsi+24],dword 5

        mov     [rsi+32],dword 0
        mov     [rsi+36],dword 0
        mov     [rsi+40],dword 0
        mov     [rsi+44],dword 0
        mov     [rsi+48],dword 0
        mov     [rsi+52],dword 0
        mov     [rsi+56],dword 0
        popdu

        pushdu
        mov     rdi,rsi
        shl     rdi,6
        add     rdi,window_data
        mov     rcx,32
        mov     rax,0
        cld
        rep     stosb

        mov     rax,[0x1FC08]                                                   ; set window to start from maxy+1
        add     rax,2

        mov     rdi,rsi
        shl     rdi,6
        add     rdi,window_data
        mov     [rdi+16],rax

        popdu

        pushdu
        mov     rdi,rsi
        shl     rdi,6
        add     rdi,draw_data
        mov     rcx,32
        mov     rax,0
        cld
        rep     stosb
        popdu

        pushdu                                                                  ; at 0x80000+
        mov     rdi,rsi
        shl     rdi,9
        add     rdi,0x800000
        mov     rcx,256
        mov     rax,0
        cld
        rep     stosb
        popdu

        pushdu                                                                  ; name to spaces
        mov     rdi,rsi
        shl     rdi,9
        add     rdi,0x800000
        mov     rcx,11
        mov     rax,32
        cld
        rep     stosb
        popdu

        pushdu                                                                  ; C000 --> C400
        mov     rax,0x18800
        mov     rsi,0
  nlc64:
        add     rax,2
        inc     rsi
        mov     rbx,[0x6008]
        cmp     rbx,rsi
        jb      nlc65
        xor     rcx,rcx
        mov     cx,[rax]
        shl     rcx,1
        add     rcx,0x18800
        mov     rdx,rsi
        mov     [rcx],dx
        jmp     nlc64
  nlc65:
        popdu

        pushdu                                                                  ; remove hd1 reservation
        mov     rdx,rsi
        imul    rdx,0x40
        add     rdx,0x6000
        mov     rdx,[rdx+8]
        cmp     [hd1_status],rdx
        jne     no_hd1_s_remove_long
        mov     [hd1_status],0
  no_hd1_s_remove_long:
        popdu


        pushdu                                                                  ; remove all irq reservations
        mov     rdx,rsi
        imul    rdx,0x40
        add     rdx,0x6000
        mov     rdx,[rdx+8]
        mov     rdi,irq_owner
        mov     rcx,16
  newirqfree_long:
        cmp     [rdi],rdx
        jne     nofreeirq_long
        mov     r8,0
        mov     [rdi],r8
  nofreeirq_long:
        add     rdi,4
        loop    newirqfree_long
        popdu

        pushdu                                                                  ; remove all scrolls
        ; mov    rdx,rsi
        ; imul   rdx,0x40
        ; add    rdx,0x6000
        ; mov    rdx,[rdx+8]
        mov     rdx , rsi
        call    remove_scrolls                                                  ; in: rdx = Process slot
        popdu


        pushdu                                                                  ; remove all port reservations

        mov     rdx,rsi
        imul    rdx,0x40
        add     rdx,0x6000
        mov     rdx,[rdx+8]

  rmpr0_long:

        mov     rsi,[0x5A0000]

        cmp     rsi,0
        je      rmpr9_long

  rmpr3_long:

        mov     rdi,rsi
        shl     rdi,5
        add     rdi,0x5A0000

        cmp     rdx,[rdi]
        je      rmpr4_long

        dec     rsi
        jnz     rmpr3_long

        jmp     rmpr9_long

  rmpr4_long:

        mov     rcx,256
        sub     rcx,rsi
        shl     rcx,4

        mov     rsi,rdi
        add     rsi,16
        cld
        rep     movsb

        dec     qword [0x5A0000]

        jmp     rmpr0_long

  rmpr9_long:

        popdu



        mov     rdi,rsi                                                         ; do not run this process slot
        imul    rdi,0x40
        add     rdi,0x6000
        mov     [rdi+0x1A],byte 9

        sti                                                                     ; .. and life goes on

        movzx   rax,word [dlx]
        movzx   rbx,word [dly]
        movzx   rcx,word [dlxe]
        movzx   rdx,word [dlye]
        call    calculatescreen

        mov     rax,0
        call    redrawscreen

        mov     [0x1FFE8],byte 0                                                ; no mouse background
        mov     [0x1FFEA],byte 0                                                ; draw mouse

        mov     [application_table_status],0

        mov     rsi,process_terminated_long
        call    sys_msg_board_str

        ret

boot_sched_1_long    db   'Building gdt tss pointer',0
boot_sched_2_long    db   'Building gdt gate pointer',0
boot_sched_3_long    db   'Building interrupt table - TSS',0
boot_sched_3_2_long  db   'Building interrupt table - GDT',0
boot_sched_3_3_long  db   'Building interrupt table - IDT',0
boot_sched_4_long    db   'Building syscall interrupt table',0


build_scheduler_long:

        mov     rsi,boot_sched_1_long
        call    boot_log
        call    build_process_gdt_tss_pointer_long

        mov     rsi,boot_sched_2_long
        call    boot_log
        call    build_process_gdt_gate_pointer_long

        mov     rsi,boot_sched_3_long
        call    boot_log
        call    build_interrupt_table_long

        mov     rsi,boot_sched_4_long
        call    boot_log
        call    build_syscall_interrupt_table_long

        retq