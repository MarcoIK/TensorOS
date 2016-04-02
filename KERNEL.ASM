;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;   Tensor Copyright 2013-2015 Marco K
;;
;;   See file COPYING for details with these additional details:
;;     - Written in 32 & 64 bit x86 & x86_64 assembly language
;;     - No external code (eg. bios) at process execution time
;;
;;     Marco K, oldklickers@gmail.com
;;      - main os coding/design
;;      - bugfixes in mouse & display drivers
;;      - code for cd-player
;;      - AMD K6-II compatible IRQ's
;;      - APM management
;;      - Sound Blaster
;;      - Fat32 write
;;      - 320x200 palette & convert
;;      - Vesa 1.2 bankswitch for S3 cards
;;      - Voodoo compatible graphics
;;      - Graphics optimizations
;;      - Bootfix for some Pentium models
;;      - SLIP driver and TCPIP stack (skeleton)
;;      - jumptable and modifications for syscalls
;;      - AMD compatible MTRR's
;;
;;   Compile with DASM 1.50+
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

max_processes      equ   255

window_data        equ   0x0000
tss_data           equ   0x9A0000
tss_step           equ   (128+8192) ; tss & i/o - 65536 ports, * 256=2129920
draw_data          equ   0xC00000
sysint_stack_data  equ   0xC03000

twdw               equ   (0x6000-window_data)

fat_base           equ   0x100000       ; ramdisk base
fat_table          equ   0x280000       ; 0xD80000

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;   Included files:
;;
;;   Kernel16.inc
;;    - Booteng.inc   English text for bootup
;;    - Bootcode.inc  Hardware setup
;;    - Pci16.inc     PCI functions
;;    - CD.inc        CD-ROM format
;;    - HDD.inc       Hard disk setup
;;
;;   Kernel32.inc
;;    - Sys32.inc     Process management
;;    - Shutdown.inc  Shutdown and restart
;;    - Fat32.inc     Read / write hd
;;    - Vesa12.inc    Vesa 1.2 driver
;;    - Vesa20.inc    Vesa 2.0 driver
;;    - Vga.inc       VGA driver
;;    - Stack.inc     Network interface
;;    - Mouse.inc     Mouse pointer
;;    - Scincode.inc  Window skinning
;;    - Pci32.inc     PCI functions
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                      ;;
;;                  16 BIT ENTRY FROM BOOTSECTOR                        ;;
;;                                                                      ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


use16

                  org   0x0 ; For 16 bit code

kernel_start:

                  jmp   start_of_code

version           db    'Tensor64 v0.86                   ',13,10,13,10,0
                  dd    endofcode-0x10000

                  db   'Boot02'

display_modechg   db    0  ; display mode change for text, yes/no (0 or 2)
                           ;
                           ; Important!!
                           ; Must be set to 2, to avoid two screenmode
                           ; changes within a very short period of time.

display_atboot    db    0  ; display text, yes/no (0 or 2)
preboot_graph     db    0  ; graphics mode
preboot_mouse     db    0  ; mouse port
preboot_mtrr      db    0  ; mtrr graphics acceleration
preboot_lfb       db    0  ; linear frame buffer
preboot_blogesc   db    0  ; start immediately after bootlog
preboot_device    db    0  ; load ramdisk from floppy/hd/kernelrestart
preboot_memory    db    0  ; amount of memory
preboot_gprobe    db    0  ; probe with vesa 2.0+



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                      ;;
;;                      16 BIT INCLUDED FILES                           ;;
;;                                                                      ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

include "KERNEL16.INC"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                      ;;
;;                  SWITCH TO 64 BIT LONG MODE                          ;;
;;                                                                      ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

null_desc      equ      null_descriptor-gdts_lo
os_data        equ      os_data_l-gdts_lo
os_code        equ      os_code_l-gdts_lo
int_code       equ      int_code_l_lo-gdts_lo
int_data       equ      int_data_l_lo-gdts_lo
tss0sys        equ      tss0sys_l_lo-gdts_lo
graph_data     equ      3+graph_data_l_lo-gdts_lo
tss0           equ      tss0_l_lo-gdts_lo
tss0i          equ      tss0i_l_lo-gdts_lo
tss0call       equ      tss0call_l_lo-gdts_lo
app_code       equ      3+app_code_l_lo-gdts_lo
app_data       equ      3+app_data_l_lo-gdts_lo
ring3_data     equ      3+ring3_data_l_lo-gdts_lo
ring3_code     equ      3+ring3_code_l_lo-gdts_lo
ring2_data     equ      2+ring2_data_l_lo-gdts_lo
ring2_code     equ      2+ring2_code_l_lo-gdts_lo
ring1_data     equ      1+ring1_data_l_lo-gdts_lo
ring1_code     equ      1+ring1_code_l_lo-gdts_lo

; CR0 Flags - Long mode and Paging


        push    word 0
        pop     es
        mov     ecx,0x00000001
        mov     bx,[es:9008]
        and     ebx,0xffff
        cmp     ebx,0100000000000000b
        jb      no_paging
        mov     al,[es:0x901E]
        cmp     al,1
        je      no_paging
        or      ecx,0x80000000
      no_paging:

; Enabling 64 bit long mode

        cli
        cld
        mov     al,255
        out     0xa1,al
        out     0x21,al
   l.5: in      al, 0x64
        test    al, 2
        jnz     l.5
        mov     al, 0xD1
        out     0x64, al
   l.6: in      al, 0x64
        test    al, 2
        jnz     l.6
        mov     al, 0xDF
        out     0x60, al
        lgdt    [cs:gdts_long-0x10000]
        mov     eax,cr4
        or      eax,1 shl 5 + 1 shl 8 + 1 shl 9
        mov     cr4,eax
        mov     ecx,0x0C0000080
        push    edx
        rdmsr
        or      eax,1 shl 8
        wrmsr
        pop     edx
        mov     eax,cr0
        or      eax,0x80000001
        and     eax,0x09FFFFFFF
        mov     cr0,eax
        jmp     shortjmp
      shortjmp:
        mov     eax,os_data
        mov     ds,ax
        mov     es,ax
        mov     fs,ax
        mov     gs,ax
        mov     ss,ax
        mov     esp,0x4FFF0
        jmp     pword os_code:B64

use64

kernel_64bit:

org ( 0x10000 + ( kernel_64bit - kernel_start ) )

macro align value { rb (value-1) - ($ + value-1) mod value }

macro pushdu
{
  push rax rbx rcx rdx rsi rdi rbp r8 r9 r10 r11 r12 r13 r14 r15
}
macro popdu
{
  pop r15 r14 r13 r12 r11 r10 r9 r8 rbp rdi rsi rdx rcx rbx rax
}

boot_fonts        db   'Fonts loaded',0
boot_tss          db   'Setting TSSs',0
boot_cpuid        db   'Reading CPUIDs',0
boot_devices      db   'Detecting devices',0
boot_timer        db   'Setting timer',0
boot_irqs         db   'Reprogramming IRQs',0
boot_setmouse     db   'Setting mouse',0
boot_windefs      db   'Setting window defaults',0
boot_bgr          db   'Calculating background',0
boot_resirqports  db   'Reserving IRQs & ports',0
boot_setrports    db   'Setting addresses for IRQs',0
boot_setostask    db   'Setting OS task',0
boot_allirqs      db   'Unmasking all IRQs',0
boot_tsc          db   'Reading TSC',0
boot_pal_ega      db   'Setting EGA/CGA 320x200 palette',0
boot_pal_vga      db   'Setting VGA 640x480 palette',0
boot_mtrr         db   'Setting MTRR',0
boot_tasking      db   'All set - press ESC to start',0

boot_y dq 10

boot_log:

         retq

         pushdu

         mov    rdx,rsi
.bll3:   inc    rdx
         cmp    [rdx],byte 0
         jne    .bll3
         sub    rdx,rsi
         mov    rax,10*65536
         mov    ax,word [boot_y]
         mov    r8,10
         add    [boot_y],r8
         mov    rbx,0xFFFFFF
         mov    rcx,rsi
         mov    rdi,1
         call   dtext

         mov    [novesachecksum],1000
         call   checkEgaCga

         cmp    [preboot_blogesc],byte 1
         je     .bll2

         cmp    rsi,boot_tasking
         jne    .bll2

.bll1:   in     al,0x64
         in     al,0x60
         cmp    al,129
         jne    .bll1

.bll2:   popdu

         retq

cpuid_0    dq  0,0
cpuid_1    dq  0,0
cpuid_2    dq  0,0
cpuid_3    dq  0,0

firstapp   db  'L','A','U','N','C','H','E','R',' ',' ',' '
char       db  'C','H','A','R',' ',' ',' ',' ','M','T',' '
char2      db  'C','H','A','R','2',' ',' ',' ','M','T',' '
hdsysimage db  'M','S','E','T','U','P',' ',' ','E','X','E'
bootpath   db  0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                      ;;
;;                          64 BIT ENTRY                                ;;
;;                                                                      ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 8

B64:

; CLEAR 0x280000-0xF00000

        xor     rax,rax
        mov     rdi,0x280000
        mov     rcx,(0x100000*0xF-0x280000) / 8
        cld
        rep     stosq

; SAVE & CLEAR 0-0xffff

        mov     rsi,0x0000
        mov     rdi,0x2F0000
        mov     rcx,0x10000 / 8
        cld
        rep     movsq
        xor     rax,rax
        mov     rdi,0
        mov     rcx,0x10000 / 8
        cld
        rep     stosq

; SAVE REAL MODE VARIABLES

        movzx   eax,byte [0x2F0000+0x9010]              ; mouse port
        mov     [0xF604],al
        mov     al,[0x2F0000+0x9000]                    ; bpp
        mov     [0xFBF1],al
        movzx   eax,word [0x2F0000+0x900A]              ; X max
        sub     eax,1
        mov     [0xFE00],eax
        movzx   eax,word [0x2F0000+0x900C]              ; Y max
        sub     eax,1
        mov     [0xFE04],eax
        movzx   eax,word [0x2F0000+0x9008]              ; screen mode
        mov     [0xFE0C],eax
        mov     eax,[0x2F0000+0x9014]                   ; Vesa 1.2 bnk sw add
        mov     [0xE030],eax
        mov     [0xFE08],word 640*4                     ; Bytes PerScanLine
        cmp     [0xFE0C],word 0x13                      ; 320x200
        je      srmvl1
        cmp     [0xFE0C],word 0x12                      ; VGA 640x480
        je      srmvl1
        mov     ax,[0x2F0000+0x9001]                    ; for other modes
        mov     [0xFE08],ax
      srmvl1:

; GRAPHICS ADDRESSES

        mov     rax,0xE0000080                          ; LFB address
        cmp     [0xFE0C],word 0x13
        je      no_d_lfb
        cmp     [0xFE0C],word 0x12
        je      no_d_lfb
        cmp     [0x2F0000+0x901E],byte 1
        jne     no_d_lfb
        mov     rax,[0x2F0000+0x9018]
      no_d_lfb:
        mov     [0xFE80],eax
        cmp     [0xFE0C],word 0100000000000000b
        jge     setvesa20
        cmp     [0xFE0C],word 0x13
        je      v20ga32
        mov     [0xE020],dword Vesa12_putpixel24        ; Vesa 1.2
        mov     [0xE024],dword Vesa12_getpixel24
        cmp     [0xFBF1],byte 24
        jz      ga24
        mov     [0xE020],dword Vesa12_putpixel32
        mov     [0xE024],dword Vesa12_getpixel32
      ga24:
        jmp     v20ga24
      setvesa20:
        mov     [0xE020],dword Vesa20_putpixel24        ; Vesa 2.0
        mov     [0xE024],dword Vesa20_getpixel24
        cmp     [0xFBF1],byte 24
        jz      v20ga24
      v20ga32:
        mov     [0xE020],dword Vesa20_putpixel32
        mov     [0xE024],dword Vesa20_getpixel32
      v20ga24:
        cmp     [0xFE0C],word 0x12                      ; 16 C VGA 640x480
        jne     no_mode_0x12
        mov     [0xE020],dword VGA_putpixel
        mov     [0xE024],dword Vesa20_getpixel32
      no_mode_0x12:
        mov     rax,[0xFE80]                            ; set for gs
        jmp     $
        mov     [graph_data_l_lo+2],ax
        shr     rax,16
        mov     [graph_data_l_lo+4],al
        mov     [graph_data_l_lo+7],ah

; MEMORY MODEL

        mov     [0xFE84],dword 0x100000*18              ; apps mem base address
        movzx   rcx,byte [0x2F0000+0x9030]
        dec     rcx
        mov     rax,32*0x100000
        shl     rax,cl
        mov     [0xFE8C],eax                            ; memory for use
        cmp     rax,16*0x100000
        jne     no16mb
        mov     [0xFE84],dword 0x100000*10
      no16mb:

; READ RAMDISK IMAGE FROM HD

        cmp     [boot_dev],byte 1
        jne     no_sys_on_hd

        mov     [fat32part],1                           ; Partition
        mov     [hdbase],0x1F0                          ; Controller base
        mov     [hdpos],1                               ;
        mov     [hdid],0x0                              ;
        mov     [0xFE10],dword 0                        ; entries in hd cache
        call    set_FAT32_variables

        mov     rsi,40
        mov     rcx,fat_base ; 0x100000
      hdbootl1:
        mov     rax,hdsysimage
        mov     rdi,12
        mov     rbx,18*2*5
        mov     rdx,bootpath

        pushdu
        call    file_read
        cmp     rax,0                                   ; image not found
        jne     $
        popdu

        add     rcx,512*18*2*5
        add     rsi,18*2*5
        cmp     rsi,1474560/512+41-1
        jb      hdbootl1
      no_sys_on_hd:


; CALCULATE FAT CHAIN FOR RAMDISK

        call    calculatefatchain

; LOAD FONTS I and II

        mov     [0x6000],dword 1
        mov     [0x6008],dword 1
        mov     [0x6020],dword 0x6040

        mov     rax,char
        mov     rsi,12
        mov     rbx,0
        mov     rcx,26000
        mov     rdx,0x37000
        call    fileread

        mov     rax,char2
        mov     rsi,12
        mov     rbx,0
        mov     rcx,26000
        mov     rdx,0x30000
        call    fileread

        mov     rsi,boot_fonts
        call    boot_log

; REDIRECT ALL IRQ'S TO INT'S 0x20-0x2f

        mov     rsi,boot_irqs
        call    boot_log
        call    rerouteirqs

        mov     rsi,boot_tss
        call    boot_log

; BUILD SCHEDULER

        push    rdx rbx
        call    build_scheduler_long                    ; sys64.inc
        pop     rbx rdx

; LOAD IDT

        lidt    [idt_lo]

; READ CPUID RESULT

        mov     rsi,boot_cpuid
        call    boot_log
        pushfq                  ; get current flags
        pop     rax
        mov     rcx,rax
        xor     rax,0x00200000  ; attempt to toggle ID bit
        push    rax
        popfq
        pushfq                  ; get new RFLAGS
        pop     rax
        push    rcx             ; restore original flags
        popfq
        and     rax,0x00200000  ; if we couldn't toggle ID,
        and     rcx,0x00200000  ; then this is i486
        cmp     rax,rcx
        jz      nopentium
        ; It's Pentium or later. Use CPUID
        mov     rdi,cpuid_0
        mov     rsi,0
      cpuid_new_read:
        mov     rax,rsi
        cpuid
        call    cpuid_save
        add     rdi,4*8
        cmp     rsi,3
        jge     cpuid_done
        cmp     rsi,[cpuid_0]
        jge     cpuid_done
        inc     rsi
        jmp     cpuid_new_read
      cpuid_save:
        mov     [rdi+00],eax
        mov     [rdi+04],ebx
        mov     [rdi+8],ecx
        mov     [rdi+12],edx
        ret
      cpuid_done:
      nopentium:

; CR4 flags - enable fxsave / fxrstore
;
;        finit
;        mov     rax,1
;        cpuid
;        test    rdx,1000000h
;        jz      fail_fpu
;        mov     rax,cr4
;        or      rax,200h        ; Enable fxsave/fxstor
;        mov     cr4,rax
;     fail_fpu:

; DETECT DEVICES

        mov     rsi,boot_devices
        call    boot_log
        call    detect_devices

; TIMER SET TO 1/100 S

        mov     rsi,boot_timer
        call    boot_log
        mov     al,0x34              ; set to 100Hz
        out     0x43,al
        mov     al,0x9B              ; lsb    1193180 / 1193
        out     0x40,al
        mov     al,0x2E              ; msb
        out     0x40,al

; SET MOUSE

        mov     rsi,boot_setmouse
        call    boot_log

        call    setmouse

; SET PRELIMINARY WINDOW STACK AND POSITIONS

        mov     rsi,boot_windefs
        call    boot_log
        call    setwindowdefaults

; SET BACKGROUND DEFAULTS

        mov     rsi,boot_bgr
        call    boot_log
        call    calculatebackground

; RESERVE SYSTEM IRQ'S JA PORT'S

        mov     rsi,boot_resirqports
        call    boot_log
        call    reserve_irqs_ports

; SET PORTS FOR IRQ HANDLERS

        mov     rsi,boot_setrports
        call    boot_log
        call    setirqreadports

; SET UP OS TASK

        mov     rsi,boot_setostask
        call    boot_log
        ; name for OS/IDLE process
        mov     [0x80000+256+0],dword 'OS/I'
        mov     [0x80000+256+4],dword 'DLE '
        ; task list
        mov     [0x6008],dword 2              ; number of processes
        mov     [0x6000],dword 0              ; process count - start with os task
        mov     [0x6040+0x22],byte 1     ; on screen number
        mov     [0x6040+0x8],dword 1     ; process id number


        mov     r8,0x11202
        mov     [l.rflags],r8                           ; sti and resume
        mov     [l.rsp0+0],os_data
        mov     [l.rsp1+0],ring1_data
        mov     [l.rsp2+0],ring2_data
        mov     [l.rsp0+4],0x52000
        mov     [l.rsp1+4],0x53000
        mov     [l.rsp2+4],0x54000
        mov     [l.ist1],0x62000
        mov     [l.ist2],0x64000
        mov     [l.ist3],0x66000
        mov     [l.ist4],0x68000
        mov     [l.ist5],0x70000
        mov     [l.ist6],0x72000
        mov     [l.ist7],0x74000
        ; osloop - TSS
        mov     rax,cr3
        mov     [l.cr3_long],rax
        mov     [l.rip],osloop
        mov     [l.rsp],0x4fff0
        mov     [l.cs_long],os_code
        mov     [l.ss_long],os_data
        mov     [l.ds_long],os_data
        mov     [l.es_long],os_data
        mov     [l.fs_long],os_data
        mov     [l.gs_long],os_data
        ; move tss to tss_data+tss_step
        mov     rsi,tss_sceleton_long
        mov     rdi,tss_data+tss_step
        mov     rcx,120/8
        cld
        rep     movsq

        mov     ax,tss0
        ltr     ax

; READ TSC / SECOND

        mov     rsi,boot_tsc
        call    boot_log
        call    _rdtsc
        mov     rcx,rax
        mov     rsi,250               ; wait 1/4 a second
        call    delay_ms
        call    _rdtsc
        sub     rax,rcx
        shl     rax,2
        mov     [0xF600],eax          ; save tsc / sec

; SET VARIABLES

        call    set_variables

; STACK AND FDC

        call    stack_init
        call    fdc_init

; PALETTE FOR 320x200 and 640x480 16 col

        cmp     [0xFE0C],word 0x12
        jne     no_pal_vga
        mov     rsi,boot_pal_vga
        call    boot_log
        call    paletteVGA
      no_pal_vga:

        cmp     [0xFE0C],word 0x13
        jne     no_pal_ega
        mov     rsi,boot_pal_ega
        call    boot_log
        call    palette320x200
      no_pal_ega:

; LOAD DEFAULT SKIN

        call    load_default_skin

; MTRR'S

        call    enable_mtrr

; LOAD FIRST APPLICATION

        mov     [0x6000],dword 1
        mov     [0x6008],dword 1

        mov     [boot_application_load],byte 1
        mov     rax,firstapp
        call    start_application_fl
        jmp     $
        mov     [boot_application_load],byte 0

        cmp     rax,2                  ; if no first app found - halt
        je      first_app_found

        cli

boot_application_load: dq 0x0

      first_app_found:

        mov     r8,2
        mov     [0x6008],r8
        mov     r8,0
        mov     [0x6000],r8


; START MULTITASKING

        mov     rsi,boot_tasking
        call    boot_log

        mov     [0xE000],byte 1        ; multitasking enabled


; UNMASK ALL IRQ'S

        mov     rsi,boot_allirqs
        call    boot_log

        mov     al,0                   ; unmask all irq's
        out     0xA1,al
        out     0x21,al

        mov     rcx,32

     ready_for_irqs:

        mov     al,0x20                ; ready for irqs
        out     0x20,al
        out     0xa0,al

        loop    ready_for_irqs         ; flush the queue

        sti
        jmp     $                      ; wait here for timer to take control




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                    ;
;;                         MAIN OS LOOP                               ;
;;                                                                    ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

osloop:

        call    check_mouse_data
        call    draw_pointer

        call    check_menus
        call    check_scrolls
        call    checkbuttons
        call    checkwindows
        call    check_window_move_request

        call    checkmisc
        call    checkEgaCga

        call    stack_handler

        call    checkidle

        jmp     osloop



osloop_without_gui_response:

        call    check_mouse_data
        call    draw_pointer

        call    checkmisc
        call    checkEgaCga

        call    stack_handler

        call    checkidle

        ret


checkidle:

        pushdu

        cmp     [check_idle_semaphore],0
        jne     no_idle_state

        call    change_task_long
        mov     rax,[idlemem]
        mov     rbx,[0xfdf0]
        cmp     rax,rbx
        jnz     idle_exit
        call    _rdtsc
        mov     rcx,rax
      idle_loop:
        hlt
        cmp     [check_idle_semaphore],0
        jne     idle_loop_exit
        mov     rax,[0xfdf0]
        cmp     rbx,rax
        jz      idle_loop
      idle_loop_exit:
        mov     [idlemem],rax
        call    _rdtsc
        sub     rax,rcx
        mov     rbx,[idleuse]
        add     rbx,rax
        mov     [idleuse],rbx

        popdu
        ret

      idle_exit:

        mov     rbx,[0xfdf0]
        mov     [idlemem],rbx
        call    change_task_long

        popdu
        ret

      no_idle_state:

        dec     [check_idle_semaphore]

        mov     rbx,[0xfdf0]
        mov     [idlemem],rbx
        call    change_task_long

        popdu
        ret


idlemem               dq   0x0
idleuse               dq   0x0
idleusesec            dq   0x0
check_idle_semaphore  dq   0x0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                                                      ;
;                   INCLUDED SYSTEM FILES                              ;
;                                                                      ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


include "KERNEL64.INC"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                                                      ;
;                       KERNEL FUNCTIONS                               ;
;                                                                      ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

enable_mtrr:

        pushdu

        cmp     [0x2f0000+0x901c],byte 2
        je      no_mtrr
        mov     rax,[0xFE0C]                ; if no LFB then no MTRR
        test    rax,0100000000000000b
        jz      no_mtrr
        mov     rdx,[cpuid_1+3*4]           ; edx - MTRR's supported ?
        test    rdx,1000000000000b
        jz      no_mtrr
        call    find_empty_mtrr
        cmp     rcx,0
        jz      no_mtrr
        mov     rsi,boot_mtrr               ; 'setting mtrr'
        call    boot_log
        mov     rdx,0x0                     ; LFB , +8 M , write combine
        mov     rax,[0x2F9018]
        or      rax,1
        wrmsr
        inc     rcx
        mov     rdx,0xf
        mov     rax,0xFF800800
        wrmsr
        mov     rcx,0x2FF                   ; enable mtrr's
        rdmsr
        or      rax,100000000000b           ; set
        wrmsr
     no_mtrr:

        popdu
        ret

delay_ms:     ; delay in 1/1000 sec


        push    rax
        push    rcx

        mov     rcx,rsi

        imul    rcx, 33941
        shr     rcx, 9

        in      al,0x61
        and     al,0x10
        mov     ah,al
        cld

 cnt1:  in      al,0x61
        and     al,0x10
        cmp     al,ah
        jz      cnt1

        mov     ah,al
        loop    cnt1

        pop     rcx
        pop     rax

        retq

calculatebackground:   ; background


        ; all black

        mov     [0x400000-8],dword 4      ; size x
        mov     [0x400000-4],dword 2      ; size y
        mov     rdi,0x300000              ; set background to black
        mov     rax,0
        mov     rcx,0x0FFF00 / 8
        cld
        rep     stosq

        mov     rdi,0x400000              ; set os to use all pixels
        mov     rax,0x01010101
        mov     rcx,0x1FFF00 / 8
        cld
        rep     stosq

        retq

setwindowdefaults:

        pushdu

        xor     rax,rax
        mov     rcx,0xC000
       swdl:
        inc     rax
        add     rcx,2
        mov     [rcx+0x000],ax          ; process no
        mov     [rcx+0x400],ax          ; positions in stack
        cmp     rcx,0xC400-2            ; the more high, the more surface
        jnz     swdl

        popdu
        retq

_rdtsc:

        mov     rdx,[cpuid_1+3*8]
        test    rdx,00010000b
        jz      ret_rdtsc
        rdtsc
        retq
   ret_rdtsc:
        mov     rdx,0xFFFFFFFF
        mov     rax,0xFFFFFFFF
        retq

find_empty_mtrr:  ; 8 pairs checked

        mov     rcx,0x201-2
      mtrr_find:
        add     rcx,2
        cmp     rcx,0x200+8*2
        jge     no_free_mtrr
        rdmsr
        test    rax,0x0800
        jnz     mtrr_find
        dec     rcx
        ret
      no_free_mtrr:
        mov     rcx,0
        ret

reserve_irqs_ports:

        pushdu

        mov     [irq_owner+8*0],byte 1    ; timer
        mov     [irq_owner+8*1],byte 1    ; keyboard
        mov     [irq_owner+8*5],byte 1    ; sound blaster
        mov     [irq_owner+8*6],byte 1    ; floppy diskette
        mov     [irq_owner+8*13],byte 1   ; math co-pros
        mov     [irq_owner+8*14],byte 1   ; ide I
        mov     [irq_owner+8*15],byte 1   ; ide II
        movzx   rax,byte [0xF604]         ; mouse irq
        dec     rax
        add     rax,mouseirqtable
        movzx   rax,byte [rax]
        shl     rax,3
        mov     [irq_owner+rax],byte 1


                                       ; RESERVE PORTS
        mov     rdi,1                    ; 0x00-0xff
        mov     [0x2D0000],rdi
        shl     rdi,4
        mov     [0x2D0000+rdi+0],dword 1
        mov     [0x2D0000+rdi+8],dword 0x0
        mov     [0x2D0000+rdi+16],dword 0xFF
        cmp     [0xF604],byte 2          ; com1 mouse -> 0x3f0-0x3ff
        jne     ripl1
        inc     qword [0x2D0000]
        mov     rdi,[0x2D0000]
        shl     rdi,4
        mov     [0x2D0000+rdi+0],dword 1
        mov     [0x2D0000+rdi+4],dword 0x3F0
        mov     [0x2D0000+rdi+8],dword 0x3FF
      ripl1:
        cmp     [0xF604],byte 3          ; com2 mouse -> 0x2f0-0x2ff
        jne     ripl2
        inc     qword [0x2D0000]
        mov     rdi,[0x2D0000]
        shl     rdi,4
        mov     [0x2D0000+rdi+0],dword 1
        mov     [0x2D0000+rdi+4],dword 0x2F0
        mov     [0x2D0000+rdi+8],dword 0x2FF
      ripl2:

        popdu
        ret

mouseirqtable   db  12    ; ps2
                db  4     ; com1
                db  3     ; com2

setirqreadports:

        mov     r9,0
        mov     r8,0x60 + 0x01000000
        mov     [irq12read+0],r8                        ; read port 0x60 , byte
        mov     [irq12read+8],r9                        ; end of port list
        mov     r8,0x3F8 + 0x01000000
        mov     [irq04read+0],r8                        ; read port 0x3f8 , byte
        mov     [irq04read+8],r9                        ; end of port list
        mov     r8,0x2F8 + 0x01000000
        mov     [irq03read+0],r8                        ; read port 0x2f8 , byte
        mov     [irq03read+8],r9                        ; end of port list

        retq

process_number dq 0x1
novesachecksum dq 0x0


set_variables:

        mov     rcx,0x100                       ; flush port 0x60
.fl60:  in      al,0x60
        loop    .fl60
        mov     [0xFCFF],byte 0                 ; mouse buffer
        mov     [0xF400],byte 0                 ; keyboard buffer
        mov     [0xF500],byte 0                 ; button buffer
        mov     [0xFB0A],dword 100*65536+100    ; mouse x/y
        mov     byte [SB16_Status],0
        mov     [0x400000-12],dword 1           ; tiled background
        mov     [0xFE88],dword 0x2C0000         ; address of button list

        retq

align 8

sys_outport:
        mov     rdi,rbx          ; separate flag for read / write
        and     rbx,65535

        mov     rcx,[0x2d0000]
        test    rcx,rcx
        jne     sopl8
        mov     r8,1
        mov     [rsp+72],r8
        retq

  sopl8:
        mov     rdx,[0x6020]
        mov     rdx,[rdx+0x8]
        and     rbx,65535
        cld
  sopl1:
        mov     r8,1
        mov     rsi,rcx
        shl     rsi,5
        add     rsi,0x2D0000
        cmp     rdx,[rsi+0]
        jne     sopl2
        cmp     rbx,[rsi+8]
        jb      sopl2
        cmp     rbx,[rsi+16]
        jg      sopl2
        jmp     sopl3

  sopl2:
        dec     rcx
        jnz     sopl1
        mov     [rsp+72],r8
        retq

  sopl3:
        mov     r8,0
        mov     r9,0x80000000
        test    rdi,r9 ; read ?
        jnz     sopl4

        mov     dx,bx          ; write
        out     dx,al
        mov     [rsp+72],r8
        retq

  sopl4:
        mov     r8,0
        mov     dx,bx          ; read
        in      al,dx
        and     rax,0xFF
        mov     [rsp+72],r8
        mov     [rsp+48],rax
        retq


rerouteirqs:

        cli

        mov     al,0x11         ;  icw4, edge triggered
        out     0x20,al
        call    pic_delay
        out     0xA0,al
        call    pic_delay

        mov     al,0x20         ;  generate 0x20 +
        out     0x21,al
        call    pic_delay
        mov     al,0x28         ;  generate 0x28 +
        out     0xA1,al
        call    pic_delay

        mov     al,0x04         ;  slave at irq2
        out     0x21,al
        call    pic_delay
        mov     al,0x02         ;  at irq9
        out     0xA1,al
        call    pic_delay

        mov     al,0x01         ;  8086 mode
        out     0x21,al
        call    pic_delay
        out     0xA1,al
        call    pic_delay

        mov     al,255          ; mask all irq's
        out     0xA1,al
        call    pic_delay
        out     0x21,al
        call    pic_delay

        mov     rcx,0x1000
        cld
picl1:  call    pic_delay
        loop    picl1

        mov     al,255          ; mask all irq's
        out     0xA1,al
        call    pic_delay
        out     0x21,al
        call    pic_delay

        cli

        retq


pic_delay:

        jmp     pdl1
pdl1:   retq


checkscreenpixel:

        mov     rsi,[0x6008]
        inc     rsi

  sciloop:

        cmp     rsi,2
        jbe     scic3

        dec     rsi

        movzx   rdi,word [rsi*2+0xc400]
        shl     rdi,5
        add     rdi,window_data

        cmp     [rdi+8],rbx ; y start
        jbe     sci2
        jmp     sciloop
  sci2:

        cmp     [rdi+0],rax ; x start
        jbe     sci1
        jmp     sciloop
  sci1:
        mov     rcx,[rdi+0]
        mov     rdx,[rdi+8]
        add     rcx,[rdi+16]
        add     rdx,[rdi+24]
        cmp     rax,rcx
        jbe     sci3
        jmp     sciloop
  sci3:
        cmp     rbx,rdx
        jbe     sci4
        jmp     sciloop
  sci4:

        movzx   rcx,word [rsi*2+0xc400]       ; process of pixel

        ; check that the process has a rectangle window

        mov     rdx,rcx
        shl     rdx,8
        add     rdx,0x80000+0x80
        cmp     [rdx],dword 0
        je      rect_shaped

  rand_shaped:

        pushdu

        sub     rax,[rdi+0]
        sub     rbx,[rdi+8]
        push    rcx
        mov     rcx,[rdx+8]
        shr     rax,cl
        shr     rbx,cl
        mov     rsi,[rdi+16]
        add     rsi,1
        shr     rsi,cl
        imul    rbx,rsi
        add     rax,rbx
        add     rax,[rdx]
        pop     rcx
        mov     rdx,rcx
        shl     rdx,5
        add     rax,[rdx+0x6000+0x20]
        cmp     [rax],byte 1
        je      rand_window_pixel

        popdu
        jmp     sciloop

  rand_window_pixel:

        popdu

  rect_shaped:

        shl     rcx,5
        add     rcx,0x6000
        movzx   rcx,byte [rcx+0x1C]   ; screen id of process

        ret

  scic3:

        mov     rcx,1          ; os pixel
        ret




calculatescreen:

;  rax  x start
;  rbx  y start
;  rcx  x end
;  rdx  y end

        pushdu
        push    rax

   csp1:

        push    rcx
        push    rdx
        push    rax
        push    rbx

        call    checkscreenpixel

        mov     rax,[0xfe00]
        inc     rax
        imul    rax,qword [rsp+0]
        add     rax,[rsp+8]
        mov     [rax+0x400000],cl

        pop     rbx
        pop     rax
        pop     rdx
        pop     rcx

        inc     rax
        cmp     rax,rcx
        jbe     csp1
        mov     rax,[rsp]
        inc     rbx
        cmp     rbx,rdx
        jbe     csp1

        pop     rax
        popdu
        ret


setscreen:

;  rax  x start
;  rbx  y start
;  rcx  x end
;  rdx  y end

        pushdu

        push    rsi
        push    rax

   csp11:

        push    rax
        push    rbx
        push    rcx
        push    rdx

        mov     rsi,rax
        mov     rax,[0xfe00]
        inc     rax
        mul     rbx
        add     rax,rsi
        add     rax,0x400000

        mov     cl,[rsp+40]
        mov     [rax],cl

        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax
        inc     rax
        cmp     rax,rcx
        jbe     csp11
        mov     rax,[rsp]
        inc     rbx
        cmp     rbx,rdx
        jbe     csp11
        add     rsp,16

        popdu
        ret

;
;align 4
;
;sys_sb16:
;
;     cmp  word [sb16],word 0
;     jnz  sb16l1
;     mov  [esp+36],dword 1
;     ret
;   sb16l1:
;     mov  [esp+36],dword 0
;     cmp  eax,1    ; set volume - main
;     jnz  sb16l2
;     mov  dx,word [sb16]
;     add  dx,4
;     mov  al,0x22
;     out  dx,al
;     mov  esi,1
;     call delay_ms
;     mov  eax,ebx
;     inc  edx
;     out  dx,al
;     ret
;   sb16l2:
;
;     cmp  eax,2    ; set volume - cd
;     jnz  sb16l3
;     mov  dx,word [sb16]
;     add  dx,4
;     mov  al,0x28
;     out  dx,al
;     mov  esi,1
;     call delay_ms
;     mov  eax,ebx
;     add  edx,1
;     out  dx,al
;     ret
;   sb16l3:
;      mov  [esp+36],dword 2
;      ret
;
;
;align 4
;
;sys_sb16II:
;
;     cmp  word [sb16],word 0
;     jnz  IIsb16l1
;     mov  [esp+36],dword 1
;     ret
;   IIsb16l1:
;
;     cmp  eax,1    ; set volume - main
;     jnz  IIsb16l2
;     ; L
;     mov  dx,word [sb16]
;     add  dx,4
;     mov  al,0x30
;     out  dx,al
;     mov  eax,ebx
;     inc  edx
;     out  dx,al
;     ; R
;     mov  dx,word [sb16]
;     add  dx,4
;     mov  al,0x31
;     out  dx,al
;     mov  eax,ebx
;     inc  edx
;     out  dx,al
;     mov  [esp+36],dword 0
;     ret
;   IIsb16l2:
;
;     cmp  eax,2    ; set volume - cd
;     jnz  IIsb16l3
;     ; L
;     mov  dx,word [sb16]
;     add  dx,4
;     mov  al,0x36
;     out  dx,al
;     mov  eax,ebx
;     inc  edx
;     out  dx,al
;     ; R
;     mov  dx,word [sb16]
;     add  dx,4
;     mov  al,0x37
;     out  dx,al
;     mov  eax,ebx
;     inc  edx
;     out  dx,al
;     mov  [esp+36],dword 0
;     ret
;   IIsb16l3:
;
;     mov  [esp+36],dword 2
;     ret
;
;
;align 4
;
;sys_wss:
;
;     cmp  word [wss],word 0
;     jnz  wssl1
;     mov  [esp+36],dword 1
;     ret
;   wssl1:
;
;     cmp  eax,1    ; set volume - main
;     jnz  wssl2
;     mov  [esp+36],dword 0
;     ret
;   wssl2:
;
;     cmp  eax,2    ; set volume - cd
;     jnz  wssl3
;     ; L
;     mov  dx,word [wss]
;     add  dx,4
;     mov  al,0x2
;     out  dx,al
;     mov  esi,1
;     call delay_ms
;     mov  eax,ebx
;     inc  edx
;     out  dx,al
;     ; R
;     mov  dx,word [wss]
;     add  dx,4
;     mov  al,0x3
;     out  dx,al
;     mov  esi,1
;     call delay_ms
;     mov  eax,ebx
;     inc  edx
;     out  dx,al
;     mov  [esp+36],dword 0
;     ret
;   wssl3:
;     mov   [esp+36],dword 2
;     ret
;
;display_number:
;
;; eax = print type, al=0 -> ebx is number
;;                   al=1 -> ebx is pointer
;;                   ah=0 -> display decimal
;;                   ah=1 -> display hexadecimal
;;                   ah=2 -> display binary
;;                   eax bits 16-21 = number of digits to display (0-32)
;;                   eax bits 22-31 = reserved
;;
;; ebx = number or pointer
;; ecx = x shl 16 + y
;; edx = color
;
;     cmp   eax,0xffff            ; length > 0 ?
;     jge   cont_displ
;     ret
;   cont_displ:
;
;     cmp   eax,60*0x10000        ; length <= 60 ?
;     jbe   cont_displ2
;     ret
;   cont_displ2:
;
;     pusha
;
;     cmp   al,1                  ; ecx is a pointer ?
;     jne   displnl1
;     mov   edi,[0x3010]
;     mov   edi,[edi+0x10]
;     mov   ebx,[edi+ebx]
;   displnl1:
;     sub   esp,64
;
;     cmp   ah,0                  ; DESIMAL
;     jne   no_display_desnum
;     shr   eax,16
;     and   eax,0x2f
;     push  eax
;     ;mov   edi,[0x3010]
;     ;mov   edi,[edi+0x10]
;     mov   edi,esp
;     add   edi,4+64
;     mov   ecx,eax
;     mov   eax,ebx
;     mov   ebx,10
;   d_desnum:
;     xor   edx,edx
;     div   ebx
;     add   dl,48
;     mov   [edi],dl
;     dec   edi
;     loop  d_desnum
;     pop   eax
;     call  draw_num_text
;     add   esp,64
;     popa
;     ret
;   no_display_desnum:
;
;     cmp   ah,0x01               ; HEXADECIMAL
;     jne   no_display_hexnum
;     shr   eax,16
;     and   eax,0x2f
;     push  eax
;     ;mov   edi,[0x3010]
;     ;mov   edi,[edi+0x10]
;     mov   edi,esp
;     add   edi,4+64
;     mov   ecx,eax
;     mov   eax,ebx
;     mov   ebx,16
;   d_hexnum:
;     xor   edx,edx
;     div   ebx
;     add   edx,hexletters
;     mov   dl,[edx]
;     mov   [edi],dl
;     dec   edi
;     loop  d_hexnum
;     pop   eax
;     call  draw_num_text
;     add   esp,64
;     popa
;     ret
;   no_display_hexnum:
;
;     cmp   ah,0x02               ; BINARY
;     jne   no_display_binnum
;     shr   eax,16
;     and   eax,0x2f
;     push  eax
;     ;mov   edi,[0x3010]
;     ;mov   edi,[edi+0x10]
;     mov   edi,esp
;     add   edi,4+64
;     mov   ecx,eax
;     mov   eax,ebx
;     mov   ebx,2
;   d_binnum:
;     xor   edx,edx
;     div   ebx
;     add   dl,48
;     mov   [edi],dl
;     dec   edi
;     loop  d_binnum
;     pop   eax
;     call  draw_num_text
;     add   esp,64
;     popa
;     ret
;   no_display_binnum:
;
;     add   esp,64
;     popa
;     ret
;
;
;draw_num_text:
;
;     ; dtext
;     ;
;     ; eax x & y
;     ; ebx color
;     ; ecx start of text
;     ; edx length
;     ; edi 1 force
;
;     mov   edx,eax
;     mov   ecx,65
;     sub   ecx,eax
;     add   ecx,esp
;     add   ecx,4
;     mov   eax,[esp+64+32-8+4]
;     mov   ebx,[esp+64+32-12+4]
;     push  edx                       ; add window start x & y
;     push  ebx
;     mov   edx,[0x3010]
;     mov   ebx,[edx-twdw]
;     shl   ebx,16
;     add   ebx,[edx-twdw+4]
;     add   eax,ebx
;     pop   ebx
;     pop   edx
;     mov   edi,0
;     call  dtext
;
;     ret
;
;display_settings:
;
;;    eax = 0         ; DISPLAY redraw
;;          ebx = 0   ; all
;;
;;    eax = 1         ; BUTTON type
;;          ebx = 0   ; flat
;;          ebx = 1   ; 3D
;;    eax = 2         ; set WINDOW colours
;;          ebx = pointer to table
;;          ecx = number of bytes define
;;    eax = 3         ; get WINDOW colours
;;          ebx = pointer to table
;;          ecx = number of bytes wanted
;;    eax = 4         ; get skin height
;;          input  : nothing
;;          output : eax = skin height in pixel
;
;
;     pusha
;
;     cmp  eax,0       ; redraw display
;     jne  dspl0
;     cmp  ebx,0
;     jne  dspl0
;     cmp  [windowtypechanged],dword 1
;     jne  dspl00
;     mov  [windowtypechanged],dword 0
;     mov  [dlx],dword 0
;     mov  [dly],dword 0
;     mov  eax,[0xfe00]
;     mov  [dlxe],eax
;     mov  eax,[0xfe04]
;     mov  [dlye],eax
;     mov  eax,window_data
;     call redrawscreen
;   dspl00:
;     popa
;     ret
;   dspl0:
;
;     cmp  eax,1       ; button type
;     jne  dspl1
;     and  ebx,1
;     cmp  ebx,[buttontype]
;     je   dspl9
;     mov  [buttontype],ebx
;     mov  [windowtypechanged],dword 1
;    dspl9:
;     popa
;     ret
;   dspl1:
;
;     cmp  eax,2       ; set common window colours
;     jne  no_com_colours
;     mov  [windowtypechanged],dword 1
;     mov  esi,[0x3010]
;     add  esi,0x10
;     add  ebx,[esi]
;     mov  esi,ebx
;     mov  edi,common_colours
;     and  ecx,127
;     cld
;     rep  movsb
;     popa
;     ret
;   no_com_colours:
;
;     cmp  eax,3       ; get common window colours
;     jne  no_get_com
;     mov  esi,[0x3010]
;     add  esi,0x10
;     add  ebx,[esi]
;     mov  edi,ebx
;     mov  esi,common_colours
;     and  ecx,127
;     cld
;     rep  movsb
;     popa
;     ret
;   no_get_com:
;
;     cmp  eax,4       ; get skin height
;     jne  no_skin_height
;     popa
;     mov  eax,[_skinh]
;     mov  [esp+36],eax
;     ret
;   no_skin_height:
;
;     popa
;     ret
;
;
;common_colours:
;
;     times 128 db 0x0
;
;
;read_string:
;
;    ; eax  read_area
;    ; ebx  color of letter
;    ; ecx  color of background
;    ; edx  number of letters to read
;    ; esi  [x start]*65536 + [y_start]
;
;    ret
;
;
;align 4
;
;sys_setup:
;
;; 1=roland mpu midi base , base io address
;; 2=keyboard   1, base kaybap 2, shift keymap, 9 country 1eng 2fi 3ger 4rus
;; 3=cd base    1, pri.master 2, pri slave 3 sec master, 4 sec slave
;; 4=sb16 base , base io address
;; 5=system language, 1eng 2fi 3ger 4rus
;; 6=wss base , base io address
;; 7=hd base    1, pri.master 2, pri slave 3 sec master, 4 sec slave
;; 8=fat32 partition in hd
;; 9
;; 10 = sound dma channel
;; 11 = enable lba read
;; 12 = enable pci access
;
;
;     mov  [esp+36],dword 0
;     cmp  eax,1                      ; MIDI
;     jnz  nsyse1
;     cmp  ebx,0x100
;     jb   nsyse1
;     mov  edx,65535
;     cmp  edx,ebx
;     jb   nsyse1
;     mov  word [mididp],bx
;     inc  bx
;     mov  word [midisp],bx
;     ret
;   nsyse1:
;
;     cmp  eax,2                      ; KEYBOARD
;     jnz  nsyse2
;     cmp  ebx,1
;     jnz  kbnobase
;     mov  edi,[0x3010]
;     add  ecx,[edi+0x10]
;     mov  eax,ecx
;     mov  ebx,keymap
;     mov  ecx,128
;     call memmove
;     ret
;   kbnobase:
;     cmp  ebx,2
;     jnz  kbnoshift
;     mov  edi,[0x3010]
;     add  ecx,[edi+0x10]
;     mov  eax,ecx
;     mov  ebx,keymap_shift
;     mov  ecx,128
;     call memmove
;     ret
;   kbnoshift:
;     cmp  ebx,3
;     jne  kbnoalt
;     mov  edi,[0x3010]
;     add  ecx,[edi+0x10]
;     mov  eax,ecx
;     mov  ebx,keymap_alt
;     mov  ecx,128
;     call memmove
;     ret
;   kbnoalt:
;     cmp  ebx,9
;     jnz  kbnocountry
;     mov  word [keyboard],cx
;     ret
;   kbnocountry:
;     mov  [esp+36],dword 1
;     ret
;   nsyse2:
;
;     cmp  eax,3                      ; CD
;     jnz  nsyse3
;     cmp  ebx,1
;     jnz  noprma
;     mov  [cdbase],0x1f0
;     mov  [cdid],0xa0
;   noprma:
;     cmp  ebx,2
;     jnz  noprsl
;     mov  [cdbase],0x1f0
;     mov  [cdid],0xb0
;   noprsl:
;     cmp  ebx,3
;     jnz  nosema
;     mov  [cdbase],0x170
;     mov  [cdid],0xa0
;   nosema:
;     cmp  ebx,4
;     jnz  nosesl
;     mov  [cdbase],0x170
;     mov  [cdid],0xb0
;   nosesl:
;     ret
;   nsyse3:
;
;     cmp  eax,4                      ; SB
;     jnz  nsyse4
;     cmp  ebx,0x100
;     jb   nsyse4
;     mov  edx,65535
;     cmp  edx,ebx
;     jb   nsyse4
;     mov  word [sb16],bx
;     ret
;   nsyse4:
;
;     cmp  eax,5                      ; SYSTEM LANGUAGE
;     jnz  nsyse5
;     mov  [syslang],ebx
;     ret
;   nsyse5:
;
;     cmp  eax,6                      ; WSS
;     jnz  nsyse6
;     cmp  ebx,0x100
;     jb   nsyse6
;     mov  [wss],ebx
;     ret
;   nsyse6:
;
;     cmp  eax,7                      ; HD BASE
;     jne  nsyse7
;     cmp  ebx,1
;     jnz  noprmahd
;     mov  [hdbase],0x1f0
;     mov  [hdid],0x0
;     mov  [hdpos],1
;     call set_FAT32_variables
;   noprmahd:
;     cmp  ebx,2
;     jnz  noprslhd
;     mov  [hdbase],0x1f0
;     mov  [hdid],0x10
;     mov  [hdpos],2
;     call set_FAT32_variables
;   noprslhd:
;     cmp  ebx,3
;     jnz  nosemahd
;     mov  [hdbase],0x170
;     mov  [hdid],0x0
;     mov  [hdpos],3
;     call set_FAT32_variables
;   nosemahd:
;     cmp  ebx,4
;     jnz  noseslhd
;     mov  [hdbase],0x170
;     mov  [hdid],0x10
;     mov  [hdpos],4
;     call set_FAT32_variables
;   noseslhd:
;     ret
;   nsyse7:
;
;     cmp  eax,8                      ; HD PARTITION
;     jne  nsyse8
;     mov  [fat32part],ebx
;     call set_FAT32_variables
;     ret
;   nsyse8:
;
;     cmp  eax,10                     ; SOUND DMA CHANNEL
;     jne  no_set_sound_dma
;     mov  [sound_dma],ebx
;     ret
;   no_set_sound_dma:
;
;     cmp  eax,11                     ; ENABLE LBA READ
;     jne  no_set_lba_read
;     and  ebx,1
;     mov  [lba_read_enabled],ebx
;     ret
;   no_set_lba_read:
;
;     cmp  eax,12                     ; ENABLE PCI ACCESS
;     jne  no_set_pci_access
;     and  ebx,1
;     mov  [pci_access_enabled],ebx
;     ret
;   no_set_pci_access:
;
;     mov  [esp+36],dword -1
;     ret
;
;
;align 4
;
;sys_getsetup:
;
;; 1=roland mpu midi base , base io address
;; 2=keyboard   1, base kaybap 2, shift keymap, 9 country 1eng 2fi 3ger 4rus
;; 3=cd base    1, pri.master 2, pri slave 3 sec master, 4 sec slave
;; 4=sb16 base , base io address
;; 5=system language, 1eng 2fi 3ger 4rus
;; 6=wss base
;; 7=hd base    1, pri.master 2, pri slave 3 sec master, 4 sec slave
;; 8=fat32 partition in hd
;; 9=get hs timer tic
;
;     cmp  eax,1
;     jne  ngsyse1
;     mov  [esp+36],dword 0
;     ret
;   ngsyse1:
;
;     cmp  eax,2
;     jne  ngsyse2
;     cmp  ebx,1
;     jnz  kbnobaseret
;     mov  edi,[0x3010]
;     add  ecx,[edi+0x10]
;     mov  ebx,ecx
;     mov  eax,keymap
;     mov  ecx,128
;     call memmove
;     ret
;   kbnobaseret:
;     cmp  ebx,2
;     jnz  kbnoshiftret
;     mov  edi,[0x3010]
;     add  ecx,[edi+0x10]
;     mov  ebx,ecx
;     mov  eax,keymap_shift
;     mov  ecx,128
;     call memmove
;     ret
;   kbnoshiftret:
;     cmp  ebx,3
;     jne  kbnoaltret
;     mov  edi,[0x3010]
;     add  ecx,[edi+0x10]
;     mov  ebx,ecx
;     mov  eax,keymap_alt
;     mov  ecx,128
;     call memmove
;     ret
;   kbnoaltret:
;     cmp  ebx,9
;     jnz  ngsyse2
;     movzx eax,word [keyboard]
;     mov  [esp+36],eax
;     ret
;   ngsyse2:
;
;     cmp  eax,3
;     jnz  ngsyse3
;     mov  [esp+36],dword 0
;     ret
;   ngsyse3:
;
;     cmp  eax,4
;     jne  ngsyse4
;     mov  [esp+36],dword 0
;     ret
;   ngsyse4:
;
;     cmp  eax,5
;     jnz  ngsyse5
;     mov  eax,[syslang]
;     mov  [esp+36],eax
;     ret
;   ngsyse5:
;
;     cmp  eax,9
;     jne  ngsyse9
;     mov  eax,[0xfdf0]
;     mov  edi,[0x3000]
;     imul edi,256
;     add  edi,0x80000
;     cmp  [edi],dword 0x4B415551
;     je   ftl1
;     cmp  [edi+3],dword 0x4B415551
;     je   ftl1
;     jmp  ftl2
;   ftl1:
;     imul eax , 2
;   ftl2:
;     mov  [esp+36],eax
;     ret
;   ngsyse9:
;
;     mov  [esp+36],dword 1
;     ret
;
scroll_step  equ 32
scroll_base  equ 0xDA0000
scroll_max   equ 2000

scroll_top:  dd  0x0

scrolling: dd 0x0

align 8

check_scrolls:

        mov     [scrolling],byte 0

  chcsl9:

        cmp     [0xfb40],byte 0
        je      chcsl1

        mov     rsi , [0x6008]
        shl     rsi , 1
        add     rsi , 0xc400
        movzx   rsi , word [rsi]

        mov     rbp , 0
        mov     rdi , scroll_base

        mov     [esisave],esi

  check_scroll_list:

        ; Process slot

        mov     rsi , [esisave]

        mov     [edisave],edi

        cmp     [rdi+20],esi
        jne     new_scroll_check ; chcsl1

        shl     rsi , 5
        add     rsi , window_data

        ; 0xfb40 - buttons
        ; 0xfb0a - word x
        ; 0xfb0c - word y

        ; X

     ;   mov   rax , [rdi+0]
     ;   and   rax , 0xffff
     ;   add   rax , [rsi+0]

        mov     rax , [rsi+0]
        cmp     [rdi+2],byte 2
        jne     chscrl12
        mov     rax , [rsi+4]
  chscrl12:
        add     rax , [rdi+0]
        and     rax , 0xffff
        mov     rbx , rax
        add     rbx , 13
        movzx   rcx , word [0xfb0a]
        cmp     [rdi+2],byte 2
        jne     chscrl1
        movzx   rcx , word [0xfb0c]
  chscrl1:
        cmp     [scrolling],byte 1
        je      chcsl95
        cmp     rcx , rax
        jb      new_scroll_check ; chcsl1
        cmp     rcx , rbx
        ja      new_scroll_check ; chcsl1
  chcsl95:

    ; Y

 ;      mov     rax , [rdi+4+2]
 ;      and     rax , 0xffff
 ;      add     rax , [rsi+4]

        mov     rax , [rsi+4]
        cmp     [rdi+2],byte 2
        jne     chscrl14
        mov     rax , [rsi+0]
  chscrl14:
        add     rax , [rdi+4+2]
        and     rax , 0xffff

        mov     rbx , [rdi+4]
        and     rbx , 0xffff
        add     rbx , rax
        movzx   rcx , word [0xfb0c]
        cmp     [rdi+2],byte 2
        jne     chscrl2
        movzx   rcx , word [0xfb0a]
  chscrl2:
        cmp     [scrolling],byte 1
        je      chcsl96
        cmp     rcx , rax
        jb      new_scroll_check ; chcsl1
        cmp     rcx , rbx
        ja      new_scroll_check ; chcsl1
  chcsl96:

        jmp     scrfoundl1

    esisave: dd 0x0
    edisave: dd 0x0

  new_scroll_check:

        add     rdi , scroll_step

        inc     rbp
        cmp     rbp , [scroll_top]
        jbe     check_scroll_list

        jmp     chcsl1 ; no scrolls found

  scrfoundl1:

  more_scroll:

        mov     rdi , [edisave]

        mov     rsi , [esisave]
        shl     rsi , 5
        add     rsi , window_data

;       mov     rax , [rdi+4+2]
;       and     rax , 0xffff
;       add     rax , [rsi+4]
;       mov     rbx , [rdi+4]
;       and     rbx , 0xffff
;       add     rbx , rax

        mov     rax , [rsi+4]
        cmp     [rdi+2],byte 2
        jne     chscrl142
        mov     rax , [rsi+0]
  chscrl142:
        add     rax , [rdi+4+2]
        and     rax , 0xffff

        mov     rbx , [rdi+4]
        and     rbx , 0xffff
        add     rbx , rax

        movzx   rcx , word [0xfb0c]
        cmp     [rdi+2],byte 2
        jne     chscrl22
        movzx   rcx , word [0xfb0a]
  chscrl22:

        mov     rsi , [esisave]

        mov     rbp , rdi

        ;;;;;

        add     rax , 15
        sub     rbx , 15

        cmp     [scrolling],byte 1
        je      chcsl97

        ; Top button

        cmp     rcx , rax
        ja      chcsl32
        mov     rax , [rbp+16]
        dec     rax
        jmp     chcsl3
  chcsl32:

        ; Low button

        cmp     rcx , rbx
        jb      chcsl33
        mov     rax , [rbp+16]
        inc     rax
        jmp     chcsl3
  chcsl33:

  chcsl97:

        ; Read glider

        movzx   rcx , word [0xfb0c]
        cmp     [rdi+2],byte 2
        jne     chscrl3
        movzx   rcx , word [0xfb0a]
  chscrl3:

        cmp     rcx , rax     ; Below lowest point
        jae     scroll_fine_1
        mov     rax , rcx
        jmp     scrolljmp
  scroll_fine_1:
        push    rax
        mov     rax , [rdi+24]
        shr     rax , 1
        sub     rcx , rax  ; glider size / 2
        pop     rax
  scrolljmp:

        cmp     rcx , rax     ; Below lowest point
        jae     scroll_fine_11
        mov     rax , rcx
  scroll_fine_11:

        sub     rcx , rax
        mov     rax , rcx

        mov     rdi , [rbp+4]
        and     rdi , 0xffff
        sub     rdi , 15+15                 ; Heads
        sub     rdi , [rbp+24]              ; Glider size
                                          ; edi = Glide area from:to

        imul    rax , [rbp+12]
        xor     rdx , rdx
        cmp     rdi , 0
        jne     noediz
        mov     rdi , 1
  noediz:
        div     rdi
        add     rax , [rbp+8]

        push    rbx  ; Above highest point
        mov     rbx , [rbp+8]
        add     rbx , [rbp+12]
        dec     rbx
        cmp     rax , rbx
        jbe     scroll_fine_2
        mov     rax , rbx
  scroll_fine_2:
        pop     rbx

        cmp     rax , [rbp+16]
        je      chcsl4
        mov     [0xf500],byte 1
        mov     [0xf501],eax
  chcsl4:

        mov     rax , 1
        call    delay_hs
        call    osloop_without_gui_response

        mov     [scrolling],byte 1

        cmp     [0xfb40],byte 0
        jne     more_scroll ; chcsl9

        mov     [scrolling],byte 0

        jmp     chcsl1

  chcsl3:

        mov     [0xf500],byte 1
        mov     [0xf501],eax

  chcsl2:
        mov     rax , 1
        call    delay_hs
        call    osloop_without_gui_response
        cmp     [0xfb40],byte 0
        jne     chcsl2

  chcsl1:

        ret

remove_scrolls:

; In : rdx = Process slot

    pushdu

        mov     rdi,scroll_base
        mov     rbp,scroll_max
  remscrl1:
        cmp     [rdi+20],rdx
        jne     remscrl2
        mov     [rdi+20],dword 0
  remscrl2:

        add     rdi,scroll_step
        dec     rbp
        jnz     remscrl1

        popdu

        ret


;check_scroll_table:
;
;; Out : free address or current address
;
;    push  ecx ebp
;
;    mov   edi , scroll_base
;    mov   ecx , [0x3000]
;    mov   ebp , [scroll_top]
;    cmp   ebp , 0
;    je    cstl21
;  cstl1:
;    cmp  [edi+0],eax
;    jne  cstl2
;    cmp  [edi+4],ebx
;    jne  cstl2
;    cmp  [edi+20],ecx
;    jne  cstl2
;
;    ; Scroll position found
;
;    pop  ebp ecx
;    ret
;  cstl2:
;    add   edi , scroll_step
;    dec   ebp
;    jnz   cstl1
;
;  cstl21:
;
;    ; Not found or list empty
;
;    ; Find empty slot
;
;    mov   edi , scroll_base
;    mov   ecx , [0x3000]
;    mov   ebp , 0
;  cstl51:
;    cmp  [edi+20],dword 0
;    jne  cstl52
;
;    ; Empty slot found - check scroll_top
;
;    inc   ebp
;    cmp   ebp , [scroll_top]
;    jbe   cstl53
;    mov   [scroll_top], ebp
;  cstl53:
;
;    pop   ebp ecx
;    ret
;  cstl52:
;    add   edi , scroll_step
;    inc   ebp
;    cmp   ebp , scroll_max
;    jb    cstl51
;
;    pop  ebp ecx
;
;    ; No empty slots
;
;    sub   edi , scroll_step
;
;    ret
;
;
;align 4
;
;sys_scroll:
;
;;        mov   eax , 113
;;  eax   mov   ebx , 001 shl 16 + 260
;;  ebx   mov   ecx , 060 shl 16 + 180
;;  ecx   mov   edx , 1000
;;  edx   mov   esi , 200
;;  esi   mov   edi , [scroll_value]
;
;    call check_scroll_table ; edi = free address or current address
;
;    cmp  edx , 1
;    jge  noscrollsizesml
;    mov  edx , 1
;  noscrollsizesml:
;
;    ; Value within limits
;
;    cmp  esi , ecx
;    jae  scrl1
;    mov  esi , ecx
;  scrl1:
;    push ecx
;    add  ecx , edx
;    dec  ecx
;    cmp  esi , ecx
;    jbe  scrl2
;    mov  esi , ecx
;  scrl2:
;    pop  ecx
;
;    ; Save values
;
;    mov  [edi+0],eax
;    mov  [edi+4],ebx
;    mov  [edi+8],ecx
;    mov  [edi+12],edx
;    mov  [edi+16],esi
;    push  eax
;    mov   eax , [0x3000]
;    mov  [edi+20],eax      ; Process slot - If Zero = slot no used
;    pop   eax
;
;    pusha
;    and   ebx , 0xffff
;    sub   ebx , 15+15
;    mov   eax , ebx
;    mov   ebx , edx
;    xor   edx , edx
;    div   ebx
;    cmp   eax , 23
;    jge   scsizefine
;    mov   eax , 23
;  scsizefine:
;    mov  [edi+24],eax ; Scroll size
;    popa
;
;    mov   ebp , eax
;    shr   ebp , 16  ; save for vertical / horizontal draw
;
;    ; Transparent head 1
;
;    pusha
;    push  ebp
;    mov   esi , scroll_grey_table
;    mov   ebp , 15
;    pop   edi
;    call  scroll_transparent
;    popa
;
;    ; Transparent head 2
;
;    pusha
;    push  ebp
;    mov   ecx , ebx
;    shl   ecx , 16
;    add   ebx , ecx
;    mov   ecx , 15 shl 16
;    sub   ebx , ecx
;    mov   esi , scroll_grey_table
;    mov   ebp , 15
;    pop   edi
;    call  scroll_transparent
;    popa
;
;    ; Draw glider
;
;    mov   ebp , ebx ; Save for background
;
;    add   ebx , 15 * 65536
;
;    ;  ecx   mov   edx , 1000
;    ;  edx   mov   esi , 200
;    ;  esi   mov   edi , [scroll_value]
;
;    push  eax ecx edx edi
;
;    mov   eax , edi
;    mov   edi , ebx
;    and   edi , 0xffff
;    sub   edi , 15+15                 ; Heads
;    sub   edi , [eax+24]              ; Glider size
;                                      ; edi = Glide area from:to
;    sub   esi , ecx
;    imul  esi , edi
;    mov   eax , esi
;    mov   ecx , edx
;    cmp   ecx , 1
;    je    nodece
;    dec   ecx
;  nodece:
;    mov   edx , 0
;    div   ecx
;
;    shl   eax , 16
;    add   ebx , eax
;
;    pop   edi edx ecx eax
;
;    ; Gray scroll area 1
;
;    pusha
;    push  eax
;    mov   ecx , ebx
;    mov   ebx , ebp
;    shr   ecx , 16
;    mov   bx  , cx
;;    sub   bx  , 2
;    mov   edi , [0x3010]
;    mov   ecx , [edi-twdw]
;    cmp   [esp+2],byte 2
;    jne   nosxys23
;    mov   ecx , [edi-twdw+4]
;  nosxys23:
;    add   ax  , cx
;    mov   dx  , ax
;    shl   eax , 16
;    mov   ax  , dx
;;    mov   ecx , ebx
;;    shr   ecx , 16
;;    add   bx  , cx
;;    dec   bx
;    add   ebx , 15*65536
;;    sub   bx  , 15
;    mov   ecx , [edi-twdw+4]
;    cmp   [esp+2],byte 2
;    jne   nosxys24
;    mov   ecx , [edi-twdw+0]
;  nosxys24:
;    mov   dx  , cx
;    shl   ecx , 16
;    mov   cx  , dx
;    add   ebx , ecx
;    mov   edi , 0
;    mov   ecx , 0xc0c0c0
;    mov   ebp , 13
;  sysscrl1:
;    pusha
;    cmp   [esp+32+2],byte 2
;    jne   nosxys2
;    xchg  eax , ebx
;  nosxys2:
;    call  draw_line
;    popa
;    add   ecx , 0x030303
;    add   eax , 0x00010001
;    dec   ebp
;    jnz   sysscrl1
;    pop   eax
;    popa
;
;
;    ; Gray scroll area 2
;
;    pusha
;    push  eax
;    shr   ebx , 16
;    add   ebx , [edi+24]
;    shl   ebx , 16
;    mov   ecx , [edi+4+2]
;    mov   bx  , cx
;    mov   ecx , [edi+4]
;    add   bx  , cx
;    sub   bx  , 15
;    sub   ebx , 0x00010001
;
;;    mov   ecx , ebx
;;    mov   ebx , ebp
;;    shr   ecx , 16
;;    mov   bx  , cx
;
;    mov   edi , [0x3010]
;    mov   ecx , [edi-twdw]
;    cmp   [esp+2],byte 2
;    jne   nosxys25
;    mov   ecx , [edi-twdw+4]
;  nosxys25:
;    add   ax  , cx
;    mov   dx  , ax
;    shl   eax , 16
;    mov   ax  , dx
;;    mov   ecx , ebx
;;    shr   ecx , 16
;;    add   bx  , cx
;;    dec   bx
;;;    add   ebx , 15*65536
;;    sub   bx  , 15
;    mov   ecx , [edi-twdw+4]
;    cmp   [esp+2],byte 2
;    jne   nosxys26
;    mov   ecx , [edi-twdw+0]
;  nosxys26:
;    mov   dx  , cx
;    shl   ecx , 16
;    mov   cx  , dx
;    add   ebx , ecx
;    mov   edi , 0
;    mov   ecx , 0xc0c0c0
;    mov   ebp , 13
;  sysscrl21:
;    pusha
;    cmp   [esp+32+2],byte 2
;    jne   nosxys3
;    xchg  eax , ebx
;  nosxys3:
;    call  draw_line
;    popa
;    add   ecx , 0x030303
;    add   eax , 0x00010001
;    dec   ebp
;    jnz   sysscrl21
;    pop   eax
;    popa
;
;
;
;    ; Gray 1
;
;    pusha
;    push  eax
;    mov   esi , scroll_grey_table
;    mov   ebp , 3
;    pop   edi
;    shr   edi , 16
;    call  scroll_transparent
;    popa
;    add   ebx , 3 * 65536
;
;    ; Blue middle
;
;    pusha
;    push  eax
;    mov   esi , scroll_color_table
;    mov   ebp , [edi+24]
;    sub   ebp , 6
;    pop   edi
;    shr   edi , 16
;    call  scroll_transparent
;    popa
;
;    push  eax
;    mov   eax , [edi+24]
;    sub   eax , 6
;    shl   eax , 16
;    add   ebx , eax
;    pop   eax
;
;    ; Gray 2
;
;    pusha
;    push  eax
;    mov   esi , scroll_grey_table
;    mov   ebp , 3
;    pop   edi
;    shr   edi , 16
;    call  scroll_transparent
;    popa
;
;    ret
;
;
;scroll_transparent:
;
;; In : ebp = size - esi = pointer to colour table - edi = vertic/horizont
;
;    pusha
;    push  edi
;
;    mov   edi , [0x3010]
;    mov   ecx , [edi-twdw]
;    cmp   [esp],byte 2
;    jne   noscxysw3
;    mov   ecx , [edi-twdw+4]
;  noscxysw3:
;    add   ax  , cx
;    mov   dx  , ax
;    shl   eax , 16
;    mov   ax  , dx
;
;    mov   ecx , ebx
;    shr   ecx , 16
;    add   ecx , ebp
;    dec   ecx
;    mov   bx  , cx
;    mov   ecx , [edi-twdw+4]
;    cmp   [esp],byte 2
;    jne   noscxysw2
;    mov   ecx , [edi-twdw+0]
;  noscxysw2:
;    mov   dx  , cx
;    shl   ecx , 16
;    mov   cx  , dx
;    add   ebx , ecx
;    mov   ebp , 13
;  sysscrl2:
;    mov   ecx , [esi]
;    and   ecx , 0xffffff
;    pusha
;    cmp   [esp+32],byte 2
;    jne   noscxysw
;    xchg  eax,ebx
;  noscxysw:
;    call  draw_line
;    popa
;    add   esi , 3
;    add   eax , 0x00010001
;    dec   ebp
;    jnz   sysscrl2
;
;    pop   edi
;    popa
;
;    ret
;
;
;scroll_color_table:
;
;    db   0xA6 ,0xA6 ,0xA6 ,0xB0 ,0x9B ,0x7F ,0xAE ,0x98
;    db   0x7D ,0xAB ,0x97 ,0x7B ,0xA6 ,0x94 ,0x78 ,0xA3
;    db   0x90 ,0x76 ,0xA0 ,0x8D ,0x73 ,0x9C ,0x8B ,0x71
;    db   0x9A ,0x87 ,0x70 ,0x97 ,0x85 ,0x6D ,0x94 ,0x84
;    db   0x6B ,0x8F ,0x80 ,0x68 ,0x4F ,0x4F ,0x4F
;
;
;scroll_grey_table:
;
;    db   0xA1 ,0xA1 ,0xA1 ,0xF1 ,0xF1 ,0xF1 ,0xE6 ,0xE6
;    db   0xE6 ,0xDE ,0xDE ,0xDE ,0xDD ,0xDD ,0xDD ,0xDC
;    db   0xDC ,0xDC ,0xCB ,0xCB ,0xCB ,0xD6 ,0xD6 ,0xD6
;    db   0xDB ,0xDB ,0xDB ,0xE4 ,0xE4 ,0xE4 ,0xEC ,0xEC
;    db   0xEC ,0xE9 ,0xE9 ,0xE9 ,0x9E ,0x9E ,0x9E
;
;
;;scroll_color_table:
;;    db    149  ,087  ,33
;;    db    229  ,210  ,156
;;    db    218  ,191  ,142
;;    db    210  ,176  ,124
;;    db    209  ,168  ,111
;;    db    208  ,170  ,100
;;    db    191  ,120  ,29
;;    db    202  ,148  ,36
;;    db    207  ,161  ,56
;;    db    216  ,173  ,68
;;    db    224  ,190  ,83
;;    db    221  ,189  ,80
;;    db    146  ,085  ,33
;;scroll_grey_table:
;;    db    159,159,159
;;    db    239,239,239
;;    db    228,228,228
;;    db    220,220,220
;;    db    219,219,219
;;    db    218,218,218
;;    db    201,201,201
;;    db    212,212,212
;;    db    217,217,217
;;    db    226,226,226
;;    db    234,234,234
;;    db    231,231,231
;;    db    156,156,156
;
;
;
;
;align 4
;
;readmousepos:
;
;; eax=0 screen relative
;; eax=1 window relative
;; eax=2 buttons pressed
;
;    test eax,eax
;    jnz  nosr
;    mov  eax,[0xfb0a]
;    shl  eax,16
;    mov  ax,[0xfb0c]
;    mov  [esp+36],eax
;    ret
;  nosr:
;
;    cmp  eax,1
;    jnz  nowr
;    mov  eax,[0xfb0a]
;    shl  eax,16
;    mov  ax,[0xfb0c]
;    mov  esi,[0x3010]
;    sub  esi,twdw
;    mov  bx,[esi]
;    shl  ebx,16
;    mov  bx,[esi+4]
;    sub  eax,ebx
;    mov  [esp+36],eax
;    ret
;  nowr:
;
;    cmp   eax,2
;    jnz   nomb
;    movzx eax,byte [0xfb40]
;  nomb:
;    mov   [esp+36],eax
;
;    ret
;
;
detect_devices:

    ret


;sys_end:
;
;     mov   eax,[0x3010]
;     add   eax,0xa
;     mov   [eax],byte 3  ; terminate this program
;    waitterm:            ; wait here for termination
;     mov   eax,5
;     call  delay_hs
;     jmp   waitterm
;
;
;
;sys_system:
;
;     cmp  eax,1                              ; BOOT
;     jnz  nosystemboot
;     mov  eax,[0x3004]
;     add  eax,2
;     mov  [shutdown_processes],eax
;     mov  [0xFF00],al
;     mov  eax,0
;     ret
;   shutdown_processes: dd 0x0
;   nosystemboot:
;
;     cmp  eax,2                              ; TERMINATE
;     jnz  noprocessterminate
;     cmp  ebx,2
;     jb   noprocessterminate
;     mov  edx,[0x3004]
;     cmp  ebx,edx
;     jg   noprocessterminate
;     mov  eax,[0x3004]
;     shl  ebx,5
;     mov  edx,[ebx+0x3000+4]
;     add  ebx,0x3000+0xa
;     mov  [ebx],byte 3       ; clear possible i40's
;
;     cmp  edx,[application_table_status]    ; clear app table stat
;     jne  noatsc
;     mov  [application_table_status],0
;   noatsc:
;
;     ret
;   noprocessterminate:
;
;     cmp  eax,3                              ; ACTIVATE WINDOW
;     jnz  nowindowactivate
;     cmp  ebx,2
;     jb   nowindowactivate
;     cmp  ebx,[0x3004]
;     jg   nowindowactivate
;     ; edi = position at window_data+
;     mov  edi,ebx
;     shl  ebx,1
;     add  ebx,0xc000
;     mov  esi,[ebx]
;     and  esi,0xffff
;     movzx edx,word [0x3004]
;     cmp  esi,edx
;     jz   nowindowactivate
;     mov  [0xff01],edi
;     mov  eax,0
;     ret
;   nowindowactivate:
;
;     cmp  eax,4                              ; GET IDLETIME
;     jnz  nogetidletime
;     mov  eax,[idleusesec]
;     ret
;   nogetidletime:
;
;     cmp  eax,5                              ; GET TSC/SEC
;     jnz  nogettscsec
;     mov  eax,[0xf600]
;     ret
;   nogettscsec:
;
;     ret
;
;sys_cd_audio:
;
;     cmp  word [cdbase],word 0
;     jnz  cdcon
;     mov  eax,1
;     ret
;   cdcon:
;
;     ; eax=1 cdplay at ebx 0x00FFSSMM
;     ; eax=2 get tracklist size of ecx to [ebx]
;     ; eax=3 stop/pause playing
;
;     cmp  eax,1
;     jnz  nocdp
;     call sys_cdplay
;     ret
;   nocdp:
;
;     cmp eax,2
;     jnz nocdtl
;     mov edi,[0x3010]
;     add edi,0x10
;     add ebx,[edi]
;     call sys_cdtracklist
;     ret
;   nocdtl:
;
;     cmp eax,3
;     jnz nocdpause
;     call sys_cdpause
;     ret
;   nocdpause:
;
;     mov eax,0xffffff01
;     ret
;
;
;
;sys_cd_atapi_command:
;
;     pusha
;
;     mov  dx,word [cdbase]
;     add  dx,6
;     mov  ax,word [cdid]
;     out  dx,al
;     mov  esi,10
;     call delay_ms
;     mov  dx,word [cdbase]
;     add  dx,7
;     in   al,dx
;     and  al,0x80
;     cmp  al,0
;     jnz  res
;     jmp  cdl6
;   res:
;     mov dx,word [cdbase]
;     add dx,7
;     mov al,0x8
;     out dx,al
;     mov dx,word [cdbase]
;     add dx,0x206
;     mov al,0xe
;     out dx,al
;     mov  esi,1
;     call delay_ms
;     mov dx,word [cdbase]
;     add dx,0x206
;     mov al,0x8
;     out dx,al
;     mov  esi,30
;     call delay_ms
;     xor  cx,cx
;   cdl5:
;     inc  cx
;     cmp  cx,10
;     jz   cdl6
;     mov  dx,word [cdbase]
;     add  dx,7
;     in   al,dx
;     and  al,0x88
;     cmp  al,0x00
;     jz   cdl5
;     mov  esi,100
;     call delay_ms
;     jmp  cdl5
;   cdl6:
;     mov dx,word [cdbase]
;     add dx,4
;     mov al,0
;     out dx,al
;     mov dx,word [cdbase]
;     add dx,5
;     mov al,0
;     out dx,al
;     mov dx,word [cdbase]
;     add dx,7
;     mov al,0xec
;     out dx,al
;     mov  esi,5
;     call delay_ms
;     mov dx,word [cdbase]
;     add dx,1
;     mov al,0
;     out dx,al
;     add dx,1
;     mov al,0
;     out dx,al
;     add dx,1
;     mov al,0
;     out dx,al
;     add dx,1
;     mov al,0
;     out dx,al
;     add dx,1
;     mov al,128
;     out dx,al
;     add dx,2
;     mov al,0xa0
;     out dx,al
;     xor  cx,cx
;     mov  dx,word [cdbase]
;     add  dx,7
;   cdl1:
;     inc  cx
;     cmp  cx,100
;     jz   cdl2
;     in   al,dx
;     and  ax,0x88
;     cmp  al,0x8
;     jz   cdl2
;     mov  esi,2
;     call delay_ms
;     jmp  cdl1
;   cdl2:
;
;     popa
;     ret
;
;
;sys_cdplay:
;
;     mov  ax,5
;     push ax
;     push ebx
;   cdplay:
;     call sys_cd_atapi_command
;     cli
;     mov  dx,word [cdbase]
;     mov  ax,0x0047
;     out  dx,ax
;     mov  al,1
;     mov  ah,[esp+0] ; min xx
;     out  dx,ax
;     mov  ax,[esp+1] ; fr sec
;     out  dx,ax
;     mov  ax,256+99
;     out  dx,ax
;     mov  ax,0x0001
;     out  dx,ax
;     mov  ax,0x0000
;     out  dx,ax
;     mov  esi,10
;     call delay_ms
;     sti
;     add  dx,7
;     in   al,dx
;     test al,1
;     jz   cdplayok
;     mov  ax,[esp+4]
;     dec  ax
;     mov  [esp+4],ax
;     cmp  ax,0
;     jz   cdplayfail
;     jmp  cdplay
;   cdplayfail:
;   cdplayok:
;     pop  ebx
;     pop  ax
;     mov  eax,0
;     ret
;
;
;sys_cdtracklist:
;
;     push ebx
;   tcdplay:
;     call sys_cd_atapi_command
;     mov  dx,word [cdbase]
;     mov  ax,0x43+2*256
;     out  dx,ax
;     mov  ax,0x0
;     out  dx,ax
;     mov  ax,0x0
;     out  dx,ax
;     mov  ax,0x0
;     out  dx,ax
;     mov  ax,200
;     out  dx,ax
;     mov  ax,0x0
;     out  dx,ax
;     in   al,dx
;     mov  cx,1000
;     mov  dx,word [cdbase]
;     add  dx,7
;     cld
;   cdtrnwewait:
;     mov  esi,10
;     call delay_ms
;     in   al,dx
;     and  al,128
;     cmp  al,0
;     jz   cdtrl1
;     loop cdtrnwewait
;   cdtrl1:
;     ; read the result
;     mov  ecx,[esp+0]
;     mov  dx,word [cdbase]
;   cdtrread:
;     add  dx,7
;     in   al,dx
;     and  al,8
;     cmp  al,8
;     jnz  cdtrdone
;     sub  dx,7
;     in   ax,dx
;     mov  [ecx],ax
;     add  ecx,2
;     jmp  cdtrread
;   cdtrdone:
;     pop  ecx
;     mov  eax,0
;     ret
;
;
;sys_cdpause:
;
;     call sys_cd_atapi_command
;
;     mov  dx,word [cdbase]
;     mov  ax,0x004B
;     out  dx,ax
;     mov  ax,0
;     out  dx,ax
;     mov  ax,0
;     out  dx,ax
;     mov  ax,0
;     out  dx,ax
;     mov  ax,0
;     out  dx,ax
;     mov  ax,0
;     out  dx,ax
;
;     mov  esi,10
;     call delay_ms
;     add  dx,7
;     in   al,dx
;
;     mov  eax,0
;     ret
;
;
;sys_cachetodiskette:
;    pusha
;    cmp  eax,1
;    jne  no_write_all_of_ramdisk
;    call fdc_writeramdisk
;    popa
;    ret
;  no_write_all_of_ramdisk:
;    cmp eax,2
;    jne no_write_part_of_ramdisk
;    call fdc_commitflush
;    popa
;    ret
;  no_write_part_of_ramdisk:
;    cmp  eax,3
;    jne  no_set_fdc
;    call fdc_set
;    popa
;    ret
;  no_set_fdc:
;    cmp  eax,4
;    jne  no_get_fdc
;    popa
;    call fdc_get
;    mov    [esp+36],ecx
;    ret
;   no_get_fdc:
;                                popa
;    ret
;
;
;bgrchanged  dd  0x0
;
;sys_background:
;
;    cmp   eax,1                            ; BACKGROUND SIZE
;    jnz   nosb1
;    cmp   ebx,0
;    je    sbgrr
;    cmp   ecx,0
;    je    sbgrr
;    mov   [0x400000-8],ebx
;    mov   [0x400000-4],ecx
;    mov   [bgrchanged],1
;  sbgrr:
;    ret
;  nosb1:
;
;    cmp   eax,2                            ; SET PIXEL
;    jnz   nosb2
;    mov   edx,0x100000-16
;    cmp   edx,ebx
;    jbe   nosb2
;    mov   edx,[ebx]
;    and   edx,255*256*256*256
;    and   ecx,255*256*256+255*256+255
;    add   edx,ecx
;    mov   [ebx+0x300000],edx
;    mov   [bgrchanged],1
;    ret
;  nosb2:
;
;    cmp   eax,3                            ; DRAW BACKGROUND
;    jnz   nosb3
;    cmp   [bgrchanged],0
;    je    nosb31
;    mov   [bgrchanged],0
;    mov   [0xfff0],byte 1
;   nosb31:
;    ret
;  nosb3:
;
;    cmp   eax,4                            ; TILED / STRETCHED
;    jnz   nosb4
;    cmp   ebx,[0x400000-12]
;    je    nosb41
;    mov   [0x400000-12],ebx
;    mov   [bgrchanged],1
;   nosb41:
;    ret
;  nosb4:
;
;    cmp   eax,5                            ; BLOCK MOVE TO BGR
;    jnz   nosb5
;    mov   edi,[0x3010]
;    add   ebx,[edi+0x10]
;    mov   esi,ebx
;    mov   edi,ecx
;    add   ecx,edx
;    cmp   ecx,0x100000-16
;    jbe   nsb52
;    ret
;   nsb52:
;    add   edi,0x300000
;    mov   ecx,edx
;    cmp   ecx,0x100000-16
;    jbe   nsb51
;    ret
;   nsb51:
;    mov   [bgrchanged],1
;    cld
;    rep   movsb
;    ret
;  nosb5:
;
;    ret
;
;
;align 4
;
;sys_getbackground:
;
;    cmp   eax,1                                  ; SIZE
;    jnz   nogb1
;    mov   eax,[0x400000-8]
;    shl   eax,16
;    mov   ax,[0x400000-4]
;    mov   [esp+36],eax
;    ret
;  nogb1:
;
;    cmp   eax,2                                  ; PIXEL
;    jnz   nogb2
;    mov   edx,0x100000-16
;    cmp   edx,ebx
;    jbe   nogb2
;    mov   eax,[ebx+0x300000]
;    and   eax,255*256*256+255*256+255
;    mov   [esp+36],eax
;    ret
;  nogb2:
;
;    cmp   eax,4                                  ; TILED / STRETCHED
;    jnz   nogb4
;    mov   eax,[0x400000-12]
;  nogb4:
;    mov   [esp+36],eax
;    ret
;
;
;align 4
;
;sys_getkey:
;
;    movzx ebx,word [0x3000]                      ; TOP OF WINDOW STACK
;    shl   ebx,1
;    mov   [esp+36],dword 1
;    add   ebx,0xc000
;    movzx ecx,word [ebx]
;    mov   edx,[0x3004]
;    cmp   ecx,edx
;    je    sysgkl1
;    ret
;  sysgkl1:
;    cmp   [0xf400],byte 0
;    jne   gkc1
;    ret
;  gkc1:
;    movzx eax,byte [0xf401]
;    shl   eax,8
;    dec   byte [0xf400]
;    and   byte [0xf400],127
;    movzx ecx,byte [0xf400]
;    add   ecx,2
;    mov   esi,0xf402
;    mov   edi,0xf401
;    cld
;    rep   movsb
;
;    mov   [esp+36],eax
;    ret
;
;
;align 4
;
;sys_getbutton:
;
;    movzx ebx,word [0x3000]                      ; TOP OF WINDOW STACK
;    mov   [esp+36],dword 1
;    shl   ebx,1
;    add   ebx,0xc000
;    movzx ecx,word[ebx]
;    movzx edx,word[0x3004]
;    cmp   ecx,edx
;    je    gbot
;    ret
;  gbot:
;    movzx eax,byte [0xf500]
;    test  eax,eax
;    jnz   gbc1
;    ret
;  gbc1:
;    mov   eax,[0xf501]
;    shl   eax,8
;    mov   [0xf500],byte 0
;    mov   [esp+36],eax
;    ret
;
;
;align 4
;
;sys_cpuusage:
;
;;  RETURN:
;;
;;  +00 dword     process cpu usage
;;  +04  word     position in windowing stack
;;  +06  word     windowing stack value at current position (cpu nro)
;;  +10 12 bytes  name
;;  +22 dword     start in mem
;;  +26 dword     used mem
;;  +30 dword     PID , process idenfification number
;;
;
;    mov  edi,[0x3010]   ; eax = return area
;    add  edi,0x10
;    add  eax,[edi]
;
;    cmp  ebx,-1         ; who am I ?
;    jne  no_who_am_i
;    mov  ebx,[0x3000]
;  no_who_am_i:
;
;    push eax            ; return area
;    push ebx            ; process number
;
;    push ebx
;    push ebx
;    push eax
;
;    ; return memory usage
;
;    xor  edx,edx
;    mov  eax,0x20
;    mul  ebx
;    add  eax,0x3000+0x1c
;    mov  ebx,eax
;    pop  eax
;    mov  ecx,[ebx]
;    mov  [eax],ecx
;    mov  ebx,[esp]
;    shl  ebx,1
;    add  ebx,0xc000
;    mov  cx,[ebx]
;    mov  [eax+4],cx
;    mov  ebx,[esp]
;    shl  ebx,1
;    add  ebx,0xc400
;    mov  cx,[ebx]
;    mov  [eax+6],cx
;    pop  ebx
;    push eax
;    mov  eax,ebx
;    shl  eax,8
;    add  eax,0x80000
;    pop  ebx
;    add  ebx,10
;    mov  ecx,11
;    call memmove
;
;    ; memory usage
;
;    xor    eax,eax
;    mov    edx,0x100000*16-4096
;    pop    ecx                                   ; get gdt of tss
;    cmp    ecx,1
;    je     os_mem
;    ;shl    ecx,8
;    ;add    ecx,0x80000+0x88
;    ;mov    ecx,[ecx]
;    shl    ecx,3
;    ; eax run base -> edx used memory
;    mov    al,[ecx+gdts+ app_code-3 +4]            ;  base  23:16
;    mov    ah,[ecx+gdts+ app_code-3 +7]            ;  base  31:24
;    shl    eax,16
;    mov    ax,[ecx+gdts+ app_code-3 +2]            ;  base  0:15
;    movzx  edx,word [ecx+gdts+ app_code-3 +0]
;    shl    edx,12
;  os_mem:
;    add    edx,4096 - 1 ; include 4 kb selector page size
;    mov    [ebx+12],eax
;    mov    [ebx+16],edx
;
;    ; PID (+30)
;
;    mov    eax,[esp]
;    shl    eax,5
;    add    eax,0x3000+0x4
;    mov    eax,[eax]
;    mov    [ebx+20],eax
;
;    ; window position and size
;
;    mov    esi,[esp]
;    shl    esi,5
;    add    esi,window_data
;    mov    edi,[esp+4]
;    add    edi,34
;    mov    ecx,4*4
;    cld
;    rep    movsb
;
;    ; Process state (+50)
;
;    mov    eax,[esp]
;    shl    eax,5
;    add    eax,0x3000+0xa
;    mov    eax,[eax]
;    mov    [ebx+40],ax
;
;
;    pop    ebx
;    pop    eax
;
;    ; return number of processes
;
;    mov    eax,[0x3004]
;    mov    [esp+36],eax
;    ret
;
;checkimage:
;
;        push  eax
;        push  ebx
;        push  ecx
;        push  edx
;        xor   edx,edx
;        xor   ecx,ecx
;        mov   cx,[0x3000]
;        shl   ecx,1
;        add   ecx,0xc000
;        mov   dx,[ecx]
;        mov   ax,[0x3004]
;        cmp   dx,ax
;        jz    imok
;        jmp   imcheckinside
;      imok:                  ; first in stack
;        pop   edx
;        pop   ecx
;        pop   ebx
;        pop   eax
;
;        mov   ecx,0
;        ret
;
;      imcheckinside:
;
;        mov   esi,edx        ; window of image -position in windowing stack
;        pop   edx
;        pop   ecx
;        pop   ebx
;        pop   eax
;        call  cilimit
;        ret
;
;cilimit:
;
;        push eax
;        push ebx
;        xor  eax,eax
;        xor  ebx,ebx
;        mov  al,[0xe000]
;        cmp  eax,1
;        jz   cilc
;        pop  ebx
;        pop  eax
;        mov  ecx,0
;        ret
;      cilc:
;        push  ecx
;        push  edx
;        mov   edi,[0x3010]
;        mov   ecx,draw_data-0x3000
;        add   edi,ecx
;        mov   eax,[esp+12]
;        mov   ebx,[esp+8]
;        mov   ecx,[esp+04]
;        mov   edx,[esp+00]
;        mov   ecx,[edi+12]
;        cmp   edx,ecx
;        jbe   cici1
;        jmp   cicino
;      cici1:
;        mov   ecx,[edi+4]
;        cmp   ecx,ebx
;        jbe   cici2
;        jmp   cicino
;      cici2:
;        mov   eax,[esp+12]
;        mov   ebx,[esp+8]
;        mov   ecx,[esp+04]
;        mov   edx,[esp+00]
;        mov   edx,[edi+8]
;        cmp   ecx,edx
;        jbe   cici3
;        jmp   cicino
;      cici3:
;        mov   edx,[edi+0]
;        cmp   edx,eax
;        jbe   cici4
;        jmp   cicino
;      cici4:
;        pop   edx
;        pop   ecx
;        pop   ebx
;        pop   eax
;        mov   ecx,0     ;inside of draw limits
;        ret
;      cicino:
;        pop   edx
;        pop   ecx
;        pop   ebx
;        pop   eax
;        mov   ecx,1     ;outside of draw limits
;        ret
;
;
;dececx:
;
;    push eax
;    push edx
;    push ecx
;
;    mov  edx,1
;
;  dececl:
;
;    movzx eax,byte [esp+edx]
;    cmp   eax,0x20
;    jge   deccl1
;    mov   [esp+edx],byte 0x20
;   deccl1:
;    sub   [esp+edx],byte 0x20
;
;    add  edx,1
;    cmp  edx,4
;    jbe  dececl
;
;    pop  ecx
;    pop  edx
;    pop  eax
;    ret
;
;drawbuttonframes2:
;
;        push  esi
;        push  edi
;        push  eax
;        push  ebx
;        push  ecx
;        push  edx
;
;        shr   eax,16
;        shr   ebx,16
;        mov   edx,[0x3010]
;
;        add   eax,[edx-twdw]
;        add   ebx,[edx-twdw+4]
;        mov   cx,ax
;        mov   dx,bx
;        shl   eax,16
;        shl   ebx,16
;        mov   ax,cx
;        mov   bx,dx
;        add   ax,word [esp+12]
;        mov   esi,ebx
;        mov   edi,0
;        mov   ecx,[esp+0]
;        add   ecx,0x202020 + 0x080808
;        call  draw_line
;
;        movzx edx,word [esp+8]
;        add   ebx,edx
;        shl   edx,16
;        add   ebx,edx
;        mov   ecx,[esp+0]
;;        call  dececx
;        sub   ecx,0x202020 + 0x080808
;        call  draw_line
;
;        mov   ebx,esi
;        push  edx
;        mov   edx,eax
;        shr   edx,16
;        mov   ax,dx
;        mov   edx,ebx
;        shr   edx,16
;        mov   bx,dx
;        mov   dx,[esp+8+4]
;        add   bx,dx
;        pop   edx
;        mov   edi,0
;        mov   ecx,[esp+0]
;        add   ecx,0x202020 + 0x080808
;        call  draw_line
;
;        mov   esi,edx
;        mov   dx,[esp+12]
;        add   ax,dx
;        shl   edx,16
;        add   eax,edx
;        add   ebx,1*65536
;        mov   edx,esi
;        mov   ecx,[esp+0]
;        sub   ecx,0x202020 + 0x080808
;
;;        call  dececx
;        call  draw_line
;
;        pop   edx
;        pop   ecx
;        pop   ebx
;        pop   eax
;        pop   edi
;        pop   esi
;
;        ret
;
;
;drawbuttonframes:
;
;        push  esi
;        push  edi
;        push  eax
;        push  ebx
;        push  ecx
;        push  edx
;
;        shr   eax,16
;        shr   ebx,16
;        mov   edx,[0x3010]
;
;        add   eax,[edx-twdw]
;        add   ebx,[edx-twdw+4]
;        mov   cx,ax
;        mov   dx,bx
;        shl   eax,16
;        shl   ebx,16
;        mov   ax,cx
;        mov   bx,dx
;        add   ax,word [esp+12]
;        mov   esi,ebx
;        mov   edi,0
;        mov   ecx,[esp+0]
;        add   ecx,0x202020
;        call  draw_line
;
;        movzx edx,word [esp+8]
;        add   ebx,edx
;        shl   edx,16
;        add   ebx,edx
;        mov   ecx,[esp+0]
;        call  dececx
;        call  draw_line
;
;        mov   ebx,esi
;        push  edx
;        mov   edx,eax
;        shr   edx,16
;        mov   ax,dx
;        mov   edx,ebx
;        shr   edx,16
;        mov   bx,dx
;        mov   dx,[esp+8+4]
;        add   bx,dx
;        pop   edx
;        mov   edi,0
;        mov   ecx,[esp+0]
;        add   ecx,0x202020
;        call  draw_line
;
;        mov   esi,edx
;        mov   dx,[esp+12]
;        add   ax,dx
;        shl   edx,16
;        add   eax,edx
;        add   ebx,1*65536
;        mov   edx,esi
;        mov   ecx,[esp+0]
;        call  dececx
;        call  draw_line
;
;        pop   edx
;        pop   ecx
;        pop   ebx
;        pop   eax
;        pop   edi
;        pop   esi
;
;        ret
;
;button_dececx:
;
;        cmp   [buttontype],dword 1
;        je    bdece
;        ret
;      bdece:
;        push  eax
;        mov   eax,0x01
;        cmp   edi,20
;        jg    bdl9
;        mov   eax,0x02
;      bdl9:
;        test  ecx,0xff
;        jz    bdl1
;        sub   ecx,eax
;      bdl1:
;        shl   eax,8
;        test  ecx,0xff00
;        jz    bdl2
;        sub   ecx,eax
;      bdl2:
;        shl   eax,8
;        test  ecx,0xff0000
;        jz    bdl3
;        sub   ecx,eax
;      bdl3:
;        pop    eax
;        ret
;
;
;sys_button:
;
;        push  ebx        ; No buttons to grab bar, if window type 2,3,..
;        mov   edi,[0x3000]
;        shl   edi,5
;        add   edi,window_data
;        mov   edi , [edi+16+3]
;        and   edi , 15
;        cmp   edi , 0
;        je    sysbl123
;        cmp   edi , 1
;        je    sysbl123
;        cmp   ax , 20
;        ja    sysbl123
;        cmp   bx , 20
;        ja    sysbl123
;        shr   ebx , 16
;        cmp   ebx , 10
;        ja    sysbl123
;        pop   ebx
;        ret
;      sysbl123:
;        pop   ebx
;
;        test  ecx,0x80000000
;        jnz   remove_button
;
;        ; 0 -> def color 0
;        ; 1 -> def color 1
;        ; 2 -> use own from application
;
;        mov   ebp , edx
;        and   ebp , 0xf0000000
;        cmp   ebp , 0x00000000
;        jne   no_def_but_1
;        and   edx , 0xff000000
;        add   edx , [defbuttoncolor]
;      no_def_but_1:
;        cmp   ebp , 0x10000000
;        jne   no_def_but_2
;        and   edx , 0xff000000
;        add   edx , [defbuttoncolor2]
;      no_def_but_2:
;
;        push  ebp
;
;        push  esi
;        push  edi
;        push  eax
;        push  ebx
;        push  ecx
;        push  edx
;
;        test  ecx,0x40000000
;        jnz   button_no_draw
;
;        pusha                       ; button body
;        push  ebx
;        shr   eax,16
;        shr   ebx,16
;        mov   edx,[0x3010]
;        mov   esi,[edx-twdw]
;        mov   edi,[edx-twdw+4]
;        add   eax,esi
;        add   ebx,edi
;        mov   cx,ax
;        mov   dx,bx
;        shl   eax,16
;        shl   ebx,16
;        mov   ax,cx
;        mov   bx,dx
;        movzx ecx,word [4+32+esp+12]
;        add   eax,ecx
;        mov   ecx,[4+32+esp+0]
;        cmp   [buttontype],dword 0
;        je    bdecel1
;        add   ecx,0x141414
;       bdecel1:
;        movzx edi,word [esp]
;       bnewline:
;        call  button_dececx
;        push  edi
;        mov   edi,0
;        call  draw_line
;        pop   edi
;        add   ebx,1*65536+1
;        dec   word [esp]
;        mov   dx,[esp]
;        cmp   dx,0
;        jnz   bnewline
;        pop   ebx
;        popa
;
;        call  drawbuttonframes2
;
;      button_no_draw:
;
;        and   ecx,0xffff
;
;        mov   edi,[0x3010]
;        sub   edi,twdw
;
;        mov   edi,[0xfe88]
;        movzx eax,word [edi]
;        cmp   eax,1000
;        jge   noaddbutt
;        inc   eax
;        mov   [edi],ax
;
;        shl   eax,4
;        add   eax,edi
;
;        mov   bx,[0x3000]
;        mov   [eax],bx
;
;        add   eax,2         ; save button id number
;        mov   ebx,[esp+4]
;        mov   [eax],bx      ; bits 0-15
;        shr   ebx,16
;        mov   [eax-2+0xc],bx; bits 16-31
;        add   eax,2         ; x start
;        mov   bx,[esp+12+2]
;        mov   [eax],bx
;        add   eax,2         ; x size
;        mov   bx,[esp+12+0]
;        mov   [eax],bx
;        add   eax,2         ; y start
;        mov   bx,[esp+8+2]
;        mov   [eax],bx
;        add   eax,2         ; y size
;        mov   bx,[esp+8+0]
;        mov   [eax],bx
;
;     noaddbutt:
;
;        pop   edx
;        pop   ecx
;        pop   ebx
;        pop   eax
;        pop   edi
;        pop   esi
;
;        ; Draw button text
;
;        pop   ebp
;        cmp   ebp , 0x10000000
;        jne   no_def_but_21
;
;        cmp   esi , 0
;        je    no_def_but_21
;
;        mov   ecx,eax
;        and   ecx,0xffff
;        shr   ecx,1
;        shr   eax,16
;        add   eax,ecx
;        add   eax,2
;        mov   ecx,ebx
;        and   ecx,0xffff
;        shr   ecx,1
;        shr   ebx,16
;        add   ebx,ecx
;        sub   ebx,3
;        mov   edx,[0x3010]
;        add   eax,[edx-twdw]
;        add   ebx,[edx-twdw+4]
;        shl   eax,16
;        mov   ax,bx
;
;        ; eax x & y
;        ; ebx font ( 0xX0000000 ) & color ( 0x00RRGGBB )
;        ; ecx start of text
;        ; edx length
;        ; edi 1 force
;
;        mov   ebx , 0x00000000
;        mov   ecx , esi
;        mov   edi , [0x3010]
;        add   ecx , [edi+0x10]
;        call  return_string_length
;        mov   edi , edx
;        imul  edi , 3
;        shl   edi , 16
;        sub   eax , edi
;        mov   edi , 0
;        call  dtext
;
;      no_def_but_21:
;
;        ret
;
;
;remove_button:
;
;    and  ecx,0x7fffffff
;
;  rnewba2:
;
;    mov   edi,[0xfe88]
;    mov   eax,edi
;    movzx ebx,word [edi]
;    inc   bx
;
;  rnewba:
;
;    dec   bx
;    jz    rnmba
;
;    add   eax,0x10
;
;    mov   dx,[0x3000]
;    cmp   dx,[eax]
;    jnz   rnewba
;
;    cmp   cx,[eax+2]
;    jnz   rnewba
;
;    pusha
;    mov   ecx,ebx
;    inc   ecx
;    shl   ecx,4
;    mov   ebx,eax
;    add   eax,0x10
;    call  memmove
;    dec   dword [edi]
;    popa
;
;    jmp   rnewba2
;
;  rnmba:
;
;    ret
;
;
;align 4
;
;sys_clock:
;
;        cli
;        xor   al,al           ; seconds
;        out   0x70,al
;        in    al,0x71
;        movzx ecx,al
;        mov   al,02           ; minutes
;        shl   ecx,16
;        out   0x70,al
;        in    al,0x71
;        movzx edx,al
;        mov   al,04           ; hours
;        shl   edx,8
;        out   0x70,al
;        in    al,0x71
;        add   ecx,edx
;        movzx edx,al
;        add   ecx,edx
;        sti
;        mov   [esp+36],ecx
;        ret
;
;
;align 4
;
;sys_date:
;
;        cli
;        mov     al,6            ; day of week
;        out     0x70,al
;        in      al,0x71
;        mov     ch,al
;        mov     al,7            ; date
;        out     0x70,al
;        in      al,0x71
;        mov     cl,al
;        mov     al,8            ; month
;        shl     ecx,16
;        out     0x70,al
;        in      al,0x71
;        mov     ch,al
;        mov     al,9            ; year
;        out     0x70,al
;        in      al,0x71
;        mov     cl,al
;        sti
;        mov     [esp+36],ecx
;        ret
;
;
;; redraw status
;
;sys_redrawstat:
;
;    cmp  eax,1
;    jne  no_widgets_away
;
;    ; buttons away
;
;    mov   ecx,[0x3000]
;
;  sys_newba2:
;
;    mov   edi,[0xfe88]
;    cmp   [edi],dword 0  ; empty button list ?
;    je    end_of_buttons_away
;
;    movzx ebx,word [edi]
;    inc   ebx
;
;    mov   eax,edi
;
;  sys_newba:
;
;    dec   ebx
;    jz    end_of_buttons_away
;
;    add   eax,0x10
;    cmp   cx,[eax]
;    jnz   sys_newba
;
;    pusha
;    mov   ecx,ebx
;    inc   ecx
;    shl   ecx,4
;    mov   ebx,eax
;    add   eax,0x10
;    call  memmove
;    dec   dword [edi]
;    popa
;
;    jmp   sys_newba2
;
;  end_of_buttons_away:
;
;    ret
;
;  no_widgets_away:
;
;    cmp   eax,2
;    jnz   srl1
;
;    mov   edx,[0x3010]      ; return whole screen draw area for this app
;    add   edx,draw_data-0x3000
;    mov   [edx+0],dword 0
;    mov   [edx+4],dword 0
;    mov   eax,[0xfe00]
;    mov   [edx+8],eax
;    mov   eax,[0xfe04]
;    mov   [edx+12],eax
;
;    mov   edi,[0x3010]
;    sub   edi,twdw
;    mov   [edi+30],byte 1   ; no new position & buttons from app
;
;    call  sys_window_mouse
;
;    ret
;
;  srl1:
;
;    ret
;
;
;sys_drawwindow:
;
;    mov   edi,ecx
;    shr   edi,16+8
;    and   edi,15
;
;    cmp   edi,0   ; type I    - original style
;    jne   nosyswI
;    call  sys_set_window
;    call  drawwindow_I
;    ret
;  nosyswI:
;
;    cmp   edi,1   ; type II   - only reserve area, no draw
;    jne   nosyswII
;    call  sys_set_window
;    call  sys_window_mouse
;    ret
;  nosyswII:
;
;    cmp   edi,2   ; type III  - new style
;    jne   nosyswIII
;    push  ebx
;    mov   ebx,[0x3000]
;    imul  ebx,256
;    add   ebx,0x80000
;    cmp   [ebx],dword 'FASM'
;    jne   nocbgr
;    mov   ebx , ecx
;    and   ebx , 0xffffff
;    cmp   ebx , 0x2030a0
;    jne   nocbgr
;    mov   cl , 0x80
;  nocbgr:
;    pop   ebx
;    call  sys_set_window
;    call  drawwindow_IV
;    ; call  drawwindow_III
;    ret
;  nosyswIII:
;
;    cmp   edi,3   ; type IV - skinned window
;    jne   nosyswIV
;    cmp   bx , 520
;    jne   no520y
;    mov   bx , 506
;  no520y:
;    call  sys_set_window
;    call  drawwindow_IV
;    ret
;  nosyswIV:
;
;    cmp   edi,4   ; type V - skinned, menu window
;    jne   nosyswV
;    cmp   esi , 0 ; No menu
;    je    no_menu_defined
;    pusha
;    call  sys_set_window
;    call  drawwindow_V
;    popa
;    call  drawwindow_menu
;    ret
;  nosyswV:
;
;    ret
;
;no_menu_defined:
;
;    pusha
;    call  sys_set_window
;    call  drawwindow_IV
;    popa
;
;    cmp   edx , 0
;    je    no_window_label_draw_2
;    pusha
;    mov   edi , [0x3010]
;    mov   ecx , edx
;    add   ecx , [edi+0x10]
;    call  return_string_length
;    mov   ebx , edx
;    imul  ebx , 8 / 2
;    mov   eax , [edi-twdw+8]
;    shr   eax , 1
;    sub   eax , ebx
;    and   eax , 0xffff
;    add   eax , [edi-twdw]
;    shl   eax , 16
;    add   eax , [edi-twdw+4]
;    add   eax , 8
;    mov   ebx , 0x10ffffff
;    mov   edi , 0
;    call  dtext
;    popa
;  no_window_label_draw_2:
;
;    ret
;
;
;drawwindow_menu:
;
;    ; Draw Window Label
;
;    cmp   edx , 0
;    je    no_window_label_draw
;    pusha
;    mov   edi , [0x3010]
;    mov   ecx , edx
;    add   ecx , [edi+0x10]
;    call  return_string_length
;    mov   ebx , edx
;    imul  ebx , 8 / 2
;    mov   eax , [edi-twdw+8]
;    shr   eax , 1
;    sub   eax , ebx
;    and   eax , 0xffff
;    add   eax , [edi-twdw]
;    shl   eax , 16
;    add   eax , [edi-twdw+4]
;    add   eax , 8
;    mov   ebx , 0x10ffffff
;    mov   edi , 0
;    call  dtext
;    popa
;  no_window_label_draw:
;
;    ; Draw Menu
;
;    cmp   esi , 0
;    je    no_window_menu_draw
;
;    mov   edi,[0x3010]
;
;    mov   eax,[edi-twdw]
;    add   eax,10
;    shl   eax,16
;    add   eax,[edi-twdw+4]
;    add   eax,27 -1
;
;    ; eax x & y
;    ; ebx font ( 0xX0000000 ) & color ( 0x00RRGGBB )
;    ; ecx start of text
;    ; edx length
;    ; edi 1 force
;
;    mov   ebx , 0x00000000
;    mov   ecx , esi
;    add   ecx , [edi+0x10]
;    add   ecx , 8*2
;
;    mov   ebp , 0 ; If window on top, menu Xpos counter.
;
;    cmp   [ecx],byte 0
;    jne   dwml1
;
;  dwml2:
;
;    inc   ecx
;
;    call  return_string_length
;
;    ;
;
;    push  eax ecx edx ebp
;    mov   edi , 0
;    call  dtext
;    pop   ebp edx ecx eax
;
;    add   ecx , edx
;    imul  edx , 6
;    add   edx , 15
;    shl   edx , 16
;    add   eax , edx
;
;  dwml3:
;
;    inc   ecx
;    cmp   [ecx],word 0+255*256
;    je    dwml1
;    cmp   [ecx],word 0+0*256
;    jne   dwml3
;    inc   ecx
;    jmp   dwml2
;
;  no_window_menu_draw:
;
;  dwml1:
;
;    ret
;
return_string_length: ; asciiz

; In : ecx = first letter : Out : edx = length

    push  rcx
    mov   rdx , 0
  rsll1:
    inc   rdx
    inc   rcx
    cmp   [rcx-1], byte 0
    jne   rsll1
    dec   rdx
    pop   rcx

    ret


get_menu_coordinates:

; In : [menu_slot]

    pushdu

    mov   [menu_id],dword 0

    ; Clear coordinates

    mov   rdi , menu_positions
    mov   rcx , 60
    mov   rax , 0xffffff
    cld
    rep   stosd

    ; Get menu abs address

    mov   rdi , [menu_slot]
    shl   rdi , 5
    mov   rcx , [rdi+window_data+0x30]   ; Menu address
    add   rcx , [rdi+0x6000+0x20]        ; App address
    add   rcx , 8*2

    mov   rdi , [menu_slot]
    shl   rdi , 5
    add   rdi , window_data
    mov   rax , [rdi]
    add   rax , 10  ; start X

    mov   rbp , 0   ; X-pos counter.

    cmp   [rcx],byte 0
    jne   gdwml1

  gdwml2:

    inc   rcx

    inc   dword [menu_id]

    call  return_string_length

    ; Save menu positions

    mov   [menu_positions+rbp*4],eax
    push  rcx rdi
    mov   rdi , [menu_slot]
    shl   rdi , 5
    sub   rcx , [rdi+0x6000+0x20]
    add   rcx , rdx
    inc   rcx
    mov   [menu_positions_text+rbp*4],ecx
    pop   rdi rcx
    push  rcx
    mov   rcx , [menu_id]
    mov   [menu_positions_id+rbp*4],ecx
    pop   rcx
    inc   dword [menu_id]

    add   rbp , 1
    and   rbp , 0x1f

    add   rcx , rdx
    imul  rdx , 6
    add   rdx , 15
    add   rax , rdx

  gdwml3:

    inc   rcx

    cmp   [rcx],word 0+1*256
    jne   gdwml11
    inc   dword [menu_id]
  gdwml11:
    cmp   [rcx],word 0+255*256
    je    gdwml1
    cmp   [rcx],word 0+0*256
    jne   gdwml3

    inc   rcx

    jmp   gdwml2

  gdwml1:

    popdu

    ret

menu_id: dd 0x0

menu_open: dd 0x0

check_menus_again:

    call  draw_menu_background

    call  check_no_selections

    jmp   chmel00


check_menus:

    mov   [menu_open],byte 0

    ; 0xfb40 - buttons

    cmp   [0xfb40],byte 0
    je    chmel1

  chmel00:

    mov   rdi , [0x6008] ; Get topmost window
    shl   rdi , 1
    add   rdi , 0xc400 ; 0xc000
    movzx rdi , word [rdi]

    mov   [menu_slot], edi

    shl   rdi , 5
    add   rdi , window_data

    mov   rax , [rdi+0x20] ; Type 4 ?
    and   rax , 0x0f000000
    cmp   rax , 0x04000000
    jne   chmel1

  ;   mov   eax , 100
  ;   call  delay_hs

    mov   rax , [rdi+0x30] ; Menu defined ?
    cmp   rax , 0x0
    je    chmel1

    ; 0xfb40 - buttons
    ; 0xfb0a - word x
    ; 0xfb0c - word y

    movzx rcx , word [0xfb0a]
    movzx rdx , word [0xfb0c]

    mov   rax , [rdi+0x0]
    mov   rbx , [rdi+0x4]
    add   rax , 5
    add   rbx , 20

    mov   [menu_start_y],ebx
    add   dword [menu_start_y],18

    cmp   rcx , rax
    jb    chmel1
    cmp   rdx , rbx
    jb    chmel1

    add   rax , [rdi+0x8]
    cmp   rcx , rax
    jg    chmel1

    add   rbx , 18
    cmp   rdx , rbx
    jg    chmel1

 ;   mov   eax , 100
 ;   call  delay_hs

    ; Get menu X coordinates

    call  get_menu_coordinates

    ; Which menu ?

    mov   rsi , menu_positions-4
    mov   rbx , rcx
    add   rbx , 6
    mov   rbp , 0

  chmel2:

    add   rsi , 4
    inc   rbp

    cmp   rsi , menu_positions+60*4
    ja    chmel1
    cmp   [rsi], dword 0xffff
    ja    chmel1

    cmp   rbx , [rsi+0]
    ja    chmel6
    jmp   chmel2
  chmel6:
    cmp   rbx , [rsi+4]
    jb    chmel61
    jmp   chmel2
  chmel61:
    mov   rax , [rsi+0]
    add   rax , 14 * 6
    cmp   rbx , rax
    jb    chmel62
    jmp   chmel2
  chmel62:

    mov   [selected_1_menu],ebp

    push  rbp
    dec   rbp
    mov   rax , [menu_positions_id+rbp*4]
    mov   [selected_menu],eax
    pop   rbp

    mov   rax , [rsi+0]
    sub   rax , 3
    mov   [menu_start_x],eax

    ; Get image & Draw menu
    ; Get image - D20000  -> DF0000   Menu under image ( 3 * 200 * 200 )

    mov   [menu_open],byte 1

    mov   rcx , [menu_slot]
    shl   rcx , 5
    mov   rcx , [rcx+0x6000+0x20]        ; App address
    mov   rbp , [selected_1_menu]
    dec   rbp
    add   rcx , [menu_positions_text+rbp*4]
    mov   [display_sub_menu],ecx

    ;

    mov   rbx , [menu_start_y]

  chmel70:

    mov   [menu_lines],dword 14

    ; Last menu ?

    push  rcx rdx
    mov   [lastmenu],byte 0
    mov   rcx , [display_sub_menu]
    inc   rcx
    call  return_string_length ; [ecx] -> edx
    add   rcx , rdx
    add   rcx , 1
    cmp   [rcx], byte 1
    je    chmel72
    mov   [lastmenu],byte 1
    mov   [menu_lines],dword 18
  chmel72:
    pop   rdx rcx

    ;

    mov   rax , [menu_start_x]
    mov   rdi , 1

    ;  Set window ID's for menu ( 0 ? )
    ;  rax  x start
    ;  rbx  y start
    ;  rcx  x end
    ;  rdx  y end
    ;  rsi

    pushdu
    mov   rcx , rax
    mov   rdx , rbx
    add   rcx , 139
    add   rdx , [menu_lines]
    dec   rdx
    mov   rsi , 0
    call  setscreen
    call  disable_mouse
    popdu

  chmel7:

    push  rax rbx rcx rdi
    ; in:
    ; eax = x coordinate
    ; ebx = y coordinate
    ; ret:
    ; ecx = 00 RR GG BB
    call  getpixel
    mov   rax , [rsp+24]
    mov   rbx , [rsp+16]
    sub   rax , [menu_start_y]
    sub   rbx , [menu_start_y]
    imul  rbx , 140
    add   rax , rbx
    imul  rax , 3
    add   rax , 0xD20000
    mov   [rax],rcx
    pop   rdi rcx rbx rax

    push  rax rbx rcx rdi
    ; rax = x coordinate
    ; rbx = y coordinate
    ; rcx = ?? RR GG BB    ; 0x01000000 negation
    ; rdi = 0x00000001 force
    mov   rcx , 0xffffff
    cmp   [lastmenu],dword 1
    jne   nomenuedge3
    cmp   [menu_lines], dword 1
    jne   nomenuedge3
    mov   rcx , 0x000000
  nomenuedge3:
    cmp   rax , [menu_start_x]
    jne   nomenuedge
    mov   rcx , 0x000000
  nomenuedge:
    mov   rdx , [menu_start_x]
    add   rdx , 139
    cmp   rax , rdx
    jne   nomenuedge2
    mov   rcx , 0x000000
  nomenuedge2:
    call  putpixel
    pop   rdi rcx rbx rax

    inc   rax
    mov   rdx , [menu_start_x]
    add   rdx , 140
    cmp   rax , rdx
    jb    chmel7
    mov   rax , [menu_start_x]

    inc   rbx

    dec   dword [menu_lines]
    jnz   chmel7

    ; Draw text

    ; dtext
    ;
    ; rax x & y
    ; rbx color
    ; rcx start of text
    ; rdx length
    ; rdi 1 force

    mov   [menu_reatched_y],ebx

    push  rbx
    mov   rax , [menu_start_x]
    add   rax , 10
    shl   rax , 16
    add   rax , rbx
    sub   rax , 10
    cmp   [lastmenu],byte 1
    jne   nolm
    sub   rax , 4
  nolm:
    mov   rbx , 0x000000
    mov   rdi , 1
    mov   rcx , [display_sub_menu]
    inc   rcx
    call  return_string_length ; [ecx] -> edx
    cmp   [rcx],byte '-'
    jne   dtextmenu
    ; draw a line
    ; rax = x1 x2
    ; rbx = y1 y2
    ; rcx = color
    ; rdi = force ?
    ; Draw line
    push  rax rbx rcx rdi
    add   rax , 4
    mov   rbx , rax
    shl   rbx , 16
    mov   bx , ax
    mov   rcx , rax
    shr   rcx , 16
    add   rcx , 116
    mov   ax , cx
    mov   rcx , 0x000000
    mov   rdi , 1
    call  draw_line
    pop   rdi rcx rbx rax
    jmp   nodtextmenu
  dtextmenu:
    call  dtext
  nodtextmenu:
    pop   rbx

    add   rcx , rdx
    add   rcx , 1
    mov   [display_sub_menu],ecx

    cmp   [rcx], byte 1
    je    chmel70

    ; Get entry & check events ( networking, .. )

       ; 0xfb40 - buttons
       ; 0xfb0a - word x
       ; 0xfb0c - word y

    ; Wait for mouse up

  chmel91:
    call  osloop_without_gui_response
    mov   rax , 1
    call  delay_hs
    cmp   [0xfb40],byte 0
    jne   chmel91

    ; Wait for mouse down

  chmel92:
    call  osloop_without_gui_response
    mov   rax , 1
    call  delay_hs
    cmp   [0xfb40],byte 0
    je    chmel92

    ; Inside drop menu ?

  ;  mov   eax , 100
  ;  call  delay_hs

    movzx rax , word [0xfb0a]
    cmp   rax , [menu_start_x]
    jb    check_menus_again
    mov   rbx , [menu_start_x]
    add   rbx , 140
    cmp   rax , rbx
    ja    check_menus_again
    movzx rbx , word [0xfb0c]
    cmp   rbx , [menu_start_y]
    jb    check_menus_again
    cmp   rbx , [menu_reatched_y]
    ja    check_menus_again

    ;

    movzx rax , word [0xfb0c]
    sub   rax , [menu_start_y]
    mov   rbx , 14
    xor   rdx , rdx
    div   rbx
    inc   rax
    add   [selected_menu],eax

    ; Wait for mouse up

  chmel93:
    call  osloop_without_gui_response
    mov   rax , 1
    call  delay_hs
    cmp   [0xfb40],byte 0
    jne   chmel93

    ; Draw menu background

    call  draw_menu_background

    ; Set button

    mov   rdi , [menu_slot]
    shl   rdi , 5
    add   rdi , window_data
    mov   rax , [rdi+0x30] ; Menu address
    mov   rdi , [menu_slot]
    shl   rdi , 5
    add   rdi , 0x6000
    add   rax , [rdi+0x20] ; App address
    mov   rbx , [rax+8]
    mov   [0xf500], byte 1 ; F500 byte - number of button presses in buffer
    add   rbx , [selected_menu]
    mov   [0xf501], rbx

    mov   [menu_open],byte 0

    ret

  chmel1:

    call  check_no_selections

    mov   [menu_open],byte 0

    ret


check_no_selections:

    cmp   [menu_open],byte 1
    jne   no_menu_closed

    ; User made no selections

    mov   edi , [menu_slot]
    shl   edi , 5
    add   edi , window_data
    mov   eax , [edi+0x18] ; Menu address
    mov   edi , [menu_slot]
    shl   edi , 5
    add   edi , 0x3000
    add   eax , [edi+0x10] ; App address
    mov   ebx , [eax+8]
    mov   [0xf500], byte 1
    mov   [0xf501], ebx

  no_menu_closed:

    ret


draw_menu_background:

    ; Draw background image

    call  disable_mouse

    mov   rax , [menu_start_x]
    mov   rbx , [menu_start_y]
    mov   rcx , 0xff0000
    mov   rdi , 1

  chmel75:

    push  rax rbx
    sub   rax , [menu_start_y]
    sub   rbx , [menu_start_y]
    imul  rbx , 140*3
    imul  rax , 3
    add   rax , rbx
    add   rax , 0xD20000
    mov   rcx , [rax]
    and   rcx , 0xffffff
    pop   rbx rax

    ; Draw pixel

    push  rax rbx rcx rdi
    ; rax = x coordinate
    ; rbx = y coordinate
    ; rcx = ?? RR GG BB    ; 0x01000000 negation
    ; rdi = 0x00000001 force
    call  putpixel
    pop   rdi rcx rbx rax

    inc   rax
    mov   rdx , [menu_start_x]
    add   rdx , 140
    cmp   rax , rdx
    jb    chmel75
    mov   rax , [menu_start_x]

    inc   rbx
    cmp   rbx , [menu_reatched_y]
    jb    chmel75

    ; Calculate window ID's

    ;  rax  x start
    ;  rbx  y start
    ;  rcx  x end
    ;  rdx  y end

    mov   rax , [menu_start_x]
    mov   rbx , [menu_start_y]
    mov   rcx , rax
    add   rcx , 140
    mov   rdx , [menu_reatched_y]
    inc   rdx
    call  calculatescreen

    ret


menu_slot:           dd  0x0
menu_add:            dd  0x0
menu_positions:      times 64 dd 0x0
menu_positions_text: times 64 dd 0x0
menu_positions_id:   times 64 dd 0x0

menu_start_x:        dd  0x0
menu_start_y:        dd  0x0

menu_lines:          dd  0x0

menuselection:       db  'Menu 1         '

menu_reatched_y:     dd  0x0

selected_1_menu:     dd  0x0

display_sub_menu:    dd  0x0

lastmenu:            dd  0x0

selected_menu:       dd  0x0
;
;sys_set_window:
;
;    mov   edi,[0x3000]
;    shl   edi,5
;    add   edi,window_data
;
;    mov   [edi+16],ecx
;    mov   [edi+20],edx
;    mov   [edi+24],esi
;
;    cmp   [edi+30],byte 1
;    jz    newd
;
;    push  eax
;    mov   eax,[0xfdf0]
;    add   eax,100
;    mov   [new_window_starting],eax
;    pop   eax
;
;    mov   [edi+8],ax
;    mov   [edi+12],bx
;    shr   eax,16
;    shr   ebx,16
;    mov   [edi+00],ax
;    mov   [edi+04],bx
;
;    call  check_window_position
;
;    pusha                   ; save for window fullscreen/resize
;    mov   esi,edi
;    sub   edi,window_data
;    shr   edi,5
;    shl   edi,8
;    add   edi,0x80000+0x90
;    mov   ecx,4
;    cld
;    rep   movsd
;    popa
;
;    pusha
;
;    mov   eax,1
;    call  delay_hs
;    movzx eax,word [edi+00]
;    movzx ebx,word [edi+04]
;    movzx ecx,word [edi+8]
;    movzx edx,word [edi+12]
;    add   cx,ax
;    add   dx,bx
;
;    call  calculatescreen
;
;    mov   [0xf400],byte 0           ; empty keyboard buffer
;    mov   [0xf500],byte 0           ; empty button buffer
;
;    popa
;
;  newd:
;    mov   [edi+31],byte 0   ; no redraw
;    mov   edx,edi
;
;    ret
;
;
;sys_window_move:
;
;        cmp  [window_move_pr],0
;        je   mwrl1
;
;        mov  [esp+36],dword 1         ; return queue error
;
;        ret
;
;     mwrl1:
;
;        mov   edi,[0x3000]            ; requestor process base
;        mov   [window_move_pr],edi
;
;        mov   [window_move_eax],eax
;        mov   [window_move_ebx],ebx
;        mov   [window_move_ecx],ecx
;        mov   [window_move_edx],edx
;
;        mov   [esp+36],dword 0        ; return success
;
;        ret
;
;

window_move_pr   dd  0x0
window_move_rax  dq  0x0
window_move_rbx  dq  0x0
window_move_rcx  dq  0x0
window_move_rdx  dq  0x0


check_window_move_request:

        pushdu

        mov     edi,[window_move_pr]    ; requestor process base

        cmp     rdi,0
        je      window_move_return

        shl     rdi,5
        add     rdi,window_data

        push    qword [rdi+0]           ; save old coordinates
        push    qword [rdi+8]
        push    qword [rdi+16]
        push    qword [rdi+24]

        mov     rax,[window_move_rax]
        mov     rbx,[window_move_rbx]
        mov     rcx,[window_move_rcx]
        mov     rdx,[window_move_rdx]

        cmp     rax,-1                  ; set new position and size
        je      no_x_reposition
        mov     [rdi+0],eax
      no_x_reposition:
        cmp     rbx,-1
        je      no_y_reposition
        mov     [rdi+4],ebx
      no_y_reposition:
        cmp     rcx,-1
        je      no_x_resizing
        mov     [rdi+8],ecx
      no_x_resizing:
        cmp     rdx,-1
        je      no_y_resizing
        mov     [rdi+12],edx
      no_y_resizing:

        call    check_window_position

        pushdu                       ; save for window fullscreen/resize
        mov     rsi,rdi
        sub     rdi,window_data
        shr     rdi,5
        shl     rdi,8
        add     rdi,0x80000+0x90
        mov     rcx,4
        cld
        rep     movsd
        popdu

        pushdu                       ; calculcate screen at new position
        mov     rax,[rdi+00]
        mov     rbx,[rdi+04]
        mov     rcx,[rdi+8]
        mov     rdx,[rdi+12]
        add     rcx,rax
        add     rdx,rbx
        call    calculatescreen
        popdu

        pop     rdx                   ; calculcate screen at old position
        pop     rcx
        pop     rbx
        pop     rax
        add     rcx,rax
        add     rdx,rbx
        mov     [dlx],eax             ; save for drawlimits
        mov     [dly],ebx
        mov     [dlxe],ecx
        mov     [dlye],edx
        call    calculatescreen

        mov     [rdi+31],byte 1       ; flag the process as redraw

        mov     rax,rdi               ; redraw screen at old position
        call    redrawscreen

        mov     [0xfff5],byte 0 ; mouse pointer
        mov     [0xfff4],byte 0 ; no mouse under
        mov     [0xfb44],byte 0 ; react to mouse up/down

        mov     rcx,10          ; wait 1/10 second
      wmrl3:
        call    check_mouse_data
        call    draw_pointer
        mov     rax,1
        call    delay_hs
        loop    wmrl3

        mov     [window_move_pr],0

      window_move_return:

        popdu

        ret



check_window_position:

    pushdu                           ; window inside screen ?

    movzx rax,word [rdi+0]
    movzx rbx,word [rdi+4]
    movzx rcx,word [rdi+8]
    movzx rdx,word [rdi+12]

    mov   rsi,rcx             ; check x pos
    add   rsi,rax
    cmp   rsi,[0xfe00]
    jbe   x_pos_ok
    mov   [rdi+0],dword 0
    mov   rax,0
  x_pos_ok:

    mov   rsi,rdx             ; check y pos
    add   rsi,rbx
    cmp   rsi,[0xfe04]
    jbe   y_pos_ok
    mov   [rdi+4],dword 0
    mov   rbx,0
  y_pos_ok:

    mov   rsi,rcx             ; check x size
    add   rsi,rax
    cmp   rsi,[0xfe00]
    jbe   x_size_ok
    mov   rcx,[0xfe00]
    mov   [rdi+8],ecx
  x_size_ok:

    mov   rsi,rdx             ; check y size
    add   rsi,rbx
    cmp   rsi,[0xfe04]
    jbe   y_size_ok
    mov   rdx,[0xfe04]
    mov   [rdi+12],edx
  y_size_ok:

    popdu

    ret


new_window_starting dd 0

;sys_window_mouse:
;
;    push  eax
;
;    mov   eax,[0xfdf0]
;    cmp   [new_window_starting],eax
;    jb    swml1
;
;    mov   [0xfff4],byte 0  ; no mouse background
;    mov   [0xfff5],byte 0  ; draw mouse
;
;    mov   [new_window_starting],eax
;
;  swml1:
;
;    pop   eax
;
;    ret
;
;drawwindow_I:
;
;        pusha
;
;        mov   esi,[edx+24]   ; rectangle
;        mov   eax,[edx+0]
;        shl   eax,16
;        add   eax,[edx+0]
;        add   eax,[edx+8]
;        mov   ebx,[edx+04]
;        shl   ebx,16
;        add   ebx,[edx+4]
;        add   ebx,[edx+12]
;        call  draw_rectangle
;
;        mov   ecx,[edx+20]   ; grab bar
;        push  ecx
;        mov   esi,edx
;        mov   edx,[esi+04]
;        add   edx,1
;        mov   ebx,[esi+04]
;        add   ebx,25
;        mov   eax,[esi+04]
;        add   eax,[esi+12]
;        cmp   ebx,eax
;        jb    wdsizeok
;        mov   ebx,eax
;      wdsizeok:
;        push  ebx
;      drwi:
;        mov   ebx,edx
;        shl   ebx,16
;        add   ebx,edx
;        mov   eax,[esi+00]
;        inc   eax
;        shl   eax,16
;        add   eax,[esi+00]
;        add   eax,[esi+8]
;        sub   eax,1
;        push  edx
;        mov   edx,0x80000000
;        mov   ecx,[esi+20]
;        and   ecx,edx
;        cmp   ecx,edx
;        jnz   nofa
;        mov   ecx,[esi+20]
;        sub   ecx,0x00040404
;        mov   [esi+20],ecx
;        and   ecx,0x00ffffff
;        jmp   faj
;      nofa:
;        mov   ecx,[esi+20]
;        and   ecx,0x00ffffff
;      faj:
;        pop   edx
;        mov   edi,0
;        call  draw_line
;        inc   edx
;        cmp   edx,[esp]
;        jb    drwi
;        add   esp,4
;        pop   ecx
;        mov   [esi+20],ecx
;
;        mov   edx,[esi+04]      ; inside work area
;        add   edx,21+5
;        mov   ebx,[esi+04]
;        add   ebx,[esi+12]
;        cmp   edx,ebx
;        jg    noinside
;        mov   eax,1
;        mov   ebx,21
;        mov   ecx,[esi+8]
;        mov   edx,[esi+12]
;        mov   edi,[esi+16]
;        call  drawbar
;      noinside:
;
;        popa
;
;        ret
;
;
draw_rectangle:

r_rax equ [rsp+28]   ; x start
r_ax  equ [rsp+30]   ; x end
r_rbx equ [rsp+16]   ; y start
r_bx  equ [rsp+18]   ; y end
;esi                 ; color

        pushdu

        mov   rcx,rsi          ; yb,xb -> yb,xe
        mov   rax,r_rax
        shl   rax,16
        mov   ax,r_ax
        mov   rbx,r_rbx
        shl   rbx,16
        mov   bx,r_rbx
        mov   rdi,0
        call  draw_line

        mov   rbx,r_bx         ; ye,xb -> ye,xe
        shl   rbx,16
        mov   bx,r_bx
        call  draw_line

        mov   rcx,rsi          ; ya,xa -> ye,xa
        mov   rax,r_rax
        shl   rax,16
        mov   ax,r_rax
        mov   rbx,r_rbx
        shl   rbx,16
        mov   bx,r_bx
        mov   rdi,0
        call  draw_line

        mov   rax,r_ax       ; ya,xe -> ye,xe
        shl   rax,16
        mov   ax,r_ax
        call  draw_line

        popdu
        ret


;drawwindow_III:
;
;        pusha
;
;        mov   edi,edx                              ; RECTANGLE
;        mov   eax,[edi+0]
;        shl   eax,16
;        mov   ax,[edi+0]
;        add   ax,[edi+8]
;        mov   ebx,[edi+4]
;        shl   ebx,16
;        mov   bx,[edi+4]
;        add   bx,[edi+12]
;        mov   esi,[edi+24]
;        shr   esi,1
;        and   esi,0x007f7f7f
;        push  esi
;        call  draw_rectangle
;        mov   ecx,3
;      dw3l:
;        add   eax,1*65536-1
;        add   ebx,1*65536-1
;        mov   esi,[edi+24]
;        call  draw_rectangle
;        dec   ecx
;        jnz   dw3l
;        pop   esi
;        add   eax,1*65536-1
;        add   ebx,1*65536-1
;        call  draw_rectangle
;
;        mov   ecx,[edx+20]                       ; GRAB BAR
;        push  ecx
;        mov   esi,edx
;        mov   edx,[esi+04]
;        add   edx,4
;        mov   ebx,[esi+04]
;        add   ebx,20
;        mov   eax,[esi+04]
;        add   eax,[esi+12]
;        cmp   ebx,eax
;        jb    wdsizeok2
;        mov   ebx,eax
;      wdsizeok2:
;        push  ebx
;      drwi2:
;        mov   ebx,edx
;        shl   ebx,16
;        add   ebx,edx
;        mov   eax,[esi+00]
;        shl   eax,16
;        add   eax,[esi+00]
;        add   eax,[esi+8]
;        add   eax,4*65536-4
;        mov   ecx,[esi+20]
;        test  ecx,0x40000000
;        jz    nofa3
;        add   ecx,0x040404
;      nofa3:
;        test  ecx,0x80000000
;        jz    nofa2
;        sub   ecx,0x040404
;      nofa2:
;        mov   [esi+20],ecx
;        and   ecx,0xffffff
;        mov   edi,0
;        call  draw_line
;        inc   edx
;        cmp   edx,[esp]
;        jb    drwi2
;        add   esp,4
;        pop   ecx
;        mov   [esi+20],ecx
;
;        mov   edx,[esi+04]                       ; WORK AREA
;        add   edx,21+5
;        mov   ebx,[esi+04]
;        add   ebx,[esi+12]
;        cmp   edx,ebx
;        jg    noinside2
;        mov   eax,5
;        mov   ebx,20
;        mov   ecx,[esi+8]
;        mov   edx,[esi+12]
;        sub   ecx,4
;        sub   edx,4
;        mov   edi,[esi+16]
;        call  drawbar
;      noinside2:
;
;        popa
;
;        ret
;
;
;sys_getevent:
;
;     call   get_event_for_app
;     mov    [esp+36],eax
;     ret
;
;
;align 4
;
;sys_wait_event_timeout:
;
;     mov   ebx,[0xfdf0]
;     add   ebx,eax
;     cmp   ebx,[0xfdf0]
;     jna   .swfet2
;   .swfet1:
;     call  get_event_for_app
;     test  eax,eax
;     jne   .eventoccur_time
;     call  change_task
;     cmp   ebx,[0xfdf0]
;     jg    .swfet1
;   .swfet2:
;     xor   eax,eax
;   .eventoccur_time:
;     mov   [esp+36],eax
;     ret
;
;
;align 4
;
;sys_waitforevent:
;
;     call  get_event_for_app
;     test  eax,eax
;     jne   eventoccur
;   newwait:
;
;     call  change_task
;     call  get_event_for_app
;     test  eax,eax
;     je    newwait
;
;   eventoccur:
;     mov   [esp+36],eax
;     ret
;
;
;get_event_for_app:
;
;     pusha
;
;     mov   edi,[0x3010]              ; WINDOW REDRAW
;     test  [edi],dword 1
;     jz    no_eventoccur1
;     mov   edi,[0x3010]
;     cmp   [edi-twdw+31],byte 0
;     je    no_eventoccur1
;     popa
;     mov   eax,1
;     ret
;   no_eventoccur1:
;
;     mov   edi,[0x3010]              ; KEY IN BUFFER
;     test  [edi],dword 2
;     jz    no_eventoccur2
;     movzx ecx,word [0x3000]
;     shl   ecx,1
;     add   ecx,0xc000
;     movzx edx,word [ecx]
;     movzx eax,word [0x3004]
;     cmp   eax,edx
;     jne   no_eventoccur2
;     cmp   [0xf400],byte 0
;     je    no_eventoccur2
;     popa
;     mov   eax,2
;     ret
;   no_eventoccur2:
;
;     mov   edi,[0x3010]              ; BUTTON IN BUFFER
;     test  [edi],dword 4
;     jz    no_eventoccur3
;     movzx ecx,word [0x3000]
;     shl   ecx,1
;     add   ecx,0xc000
;     movzx edx,word [ecx]
;     movzx eax,word [0x3004]
;     cmp   eax,edx
;     jnz   no_eventoccur3
;     cmp   [0xf500],byte 0
;     je    no_eventoccur3
;     popa
;     mov   eax,3
;     ret
;   no_eventoccur3:
;
;     mov   edi,[0x3010]              ; DESKTOP BACKGROUND REDRAW
;     test  [edi],dword 16
;     jz    no_eventoccur5
;     cmp   [0xfff0],byte 2
;     jnz   no_eventoccur5
;     popa
;     mov   eax,5
;     ret
;   no_eventoccur5:
;
;     mov   edi,[0x3010]              ; mouse event
;     test  [edi],dword 00100000b
;     jz    no_mouse_event
;     mov   edi,[0x3000]
;     shl   edi,8
;     test  [edi+0x80000+0xA8],dword 00100000b
;     jz    no_mouse_event
;     and   [edi+0x80000+0xA8],dword 0xffffffff-00100000b
;     popa
;     mov   eax,6
;     ret
;   no_mouse_event:
;
;     mov   edi,[0x3010]              ; IPC
;     test  [edi],dword 01000000b
;     jz    no_ipc
;     mov   edi,[0x3000]
;     shl   edi,8
;     test  [edi+0x80000+0xA8],dword 01000000b
;     jz    no_ipc
;     and   [edi+0x80000+0xA8],dword 0xffffffff-01000000b
;     popa
;     mov   eax,7
;     ret
;   no_ipc:
;
;
;     mov   edi,[0x3010]              ; STACK
;     test  [edi],dword 10000000b
;     jz    no_stack_event
;     mov   edi,[0x3000]
;     shl   edi,8
;     test  [edi+0x80000+0xA8],dword 10000000b
;     jz    no_stack_event
;     and   [edi+0x80000+0xA8],dword 0xffffffff-10000000b
;     popa
;     mov   eax,7
;     ret
;   no_stack_event:
;
;
;     mov   esi,0x2e0000              ; IRQ'S AND DATA
;     mov   ebx,0x00010000
;     mov   ecx,0
;   irq_event_test:
;     mov   edi,[0x3010]
;     test  [edi],ebx
;     jz    no_irq_event
;     mov   edi,ecx
;     shl   edi,2
;     add   edi,irq_owner
;     mov   edx,[edi]
;     mov   eax,[0x3010]
;     mov   eax,[eax+0x4]
;     cmp   edx,eax
;     jne   no_irq_event
;     cmp   [esi],dword 0
;     jz    no_irq_event
;     mov   eax,ecx
;     add   eax,16
;     mov   [esp+28],eax
;     popa
;     ret
;    no_irq_event:
;     add   esi,0x1000
;     shl   ebx,1
;     inc   ecx
;     cmp   ecx,16
;     jb    irq_event_test
;
;     popa
;     mov   eax,0
;     ret
;
;
dtext:

        ; rax x & y
        ; rbx font ( 0xX0000000 ) & color ( 0x00RRGGBB )
        ; rcx start of text
        ; rdx length
        ; rdi 1 force

        test    rbx,0x10000000
        jnz     dtext2

        pushdu

        mov     rsi,rdx
        and     rsi,0xff
        cmp     rsi,0    ; zero length ?
        jnz     dsok

        popdu
        ret

      dsok:

      letnew:

        push    rax
        push    rcx
        push    rdx
        movzx   rbx,ax
        shr     rax,16
        movzx   rdx,byte [rcx]
        mov     rcx,[rsp+6*4+64-32]
        call    drawletter
        pop     rdx
        pop     rcx
        pop     rax

        add     rax,6*65536

        add     rcx,1
        dec     dx
        jnz     letnew

        popdu
        ret


drawletter:

; rax  x
; rbx  y
; rcx  color
; rdx  letter
; rsi  shl size
; rdi  force

        pushdu

        mov     rax,0
        mov     rbx,0  ; 0x37000+rax+rbx*8
        inc     rsi

      chc:

        push    rax
        push    rbx

        mov     rdx,rbx
        shl     rdx,3
        add     rdx,rax
        add     rdx,0x37000+8
        mov     rcx,[rsp+64-24+16]
        imul    rcx,8*10
        add     rdx,rcx
        cmp     [rdx],byte 'o'
        jnz     nopix
        mov     rax,[rsp+8]
        mov     rbx,[rsp+0]
        add     rax,[rsp+64+4*4-8]
        add     rbx,[rsp+64+4*4-32]
        mov     rcx,[rsp+64+4*4-16]
        call    disable_mouse
        call    putpixel
      nopix:
        pop     rbx
        pop     rax

        add     rax,1
        cmp     rax,5 ; rbp
        jnz     chc

        mov     rax,0

        add     rbx,1
        cmp     rbx,9 ; rbp
        jnz     chc

        popdu
        ret


dtext2:

        ; rax x & y
        ; rbx color
        ; rcx start of text
        ; rdx length
        ; rdi 1 force

        pushdu

        mov     rsi,rdx
        and     rsi,0xff
        cmp     rsi,0    ; zero length ?
        jnz     dsok2

        popdu
        ret

      dsok2:
      letnew2:

        push    rax
        push    rcx
        push    rdx
        movzx   rbx,ax
        shr     rax,16
        movzx   rdx,byte [rcx]
        mov     rcx,[rsp+6*4+64-32]
        call    drawletter2
        pop     rdx
        pop     rcx
        pop     rax

        push    rdx
        movzx   rdx,byte [rcx]
        imul    rdx,10*10
        add     rdx,0x30000
        cmp     [rdx+6],byte ' '
        jne     nocharadd8
        add     rax,8*65536
        jmp     charaddok
      nocharadd8:
        movzx   rdx,byte [rdx+6]
        sub     rdx,47
        shl     rdx,16
        add     rax,rdx
      charaddok:
        pop     rdx

        add     rcx,1
        dec     dx
        jnz     letnew2

        popdu
        ret


drawletter2:

; rax  x
; rbx  y
; rcx  color
; rdx  letter
; rsi  shl size
; rdi  force

        pushdu

        mov     rax,0
        mov     rbx,0  ; +rax+rbx*8
        inc     rsi

      chc2:

        push    rax
        push    rbx

;        cmp   rsi,1
;        je    noldiv
;        xor   rdx,rdx
;        div   rsi
;        push  rax
;        xor   rdx,rdx
;        mov   rax,rbx
;        div   rsi
;        mov   rbx,rax
;        pop   rax
;      noldiv:

        mov     rdx,rbx
        ;shl   rdx,3
        imul    rdx,10
        add     rdx,rax
        add     rdx,0x30000+8+2
        mov     rcx,[rsp+64-24+16]
        ;shl   rcx,6
        imul    rcx,10*10
        add     rdx,rcx
        cmp     [rdx],byte 'o'
        jnz     nopix2
        mov     rax,[rsp+8]
        mov     rbx,[rsp+0]
        add     rax,[rsp+64+4*4-8]
        add     rbx,[rsp+64+4*4-32]
        mov     rcx,[rsp+64+4*4-16]
        call    disable_mouse
        call    putpixel
      nopix2:
        pop     rbx
        pop     rax

        ;mov   rbp,7
        ;imul  rbp,rsi

        add     rax,1
        cmp     rax,7 ;rbp
        jnz     chc2

        mov     rax,0

        ;mov   rbp,9
        ;imul  rbp,rsi

        add     rbx,1
        cmp     rbx,9 ; rbp
        jnz     chc2

        popdu
        ret




; check pixel limits

cplimit:
        push    rdi

        cmp     byte [0xe000], 1
        jnz     .ret0
        mov     rdi,[0x3010]
        add     rdi,draw_data-0x3000
        mov     rcx,1
        cmp     [rdi+0],eax  ; xs
        ja      .ret1
        cmp     [rdi+4],ebx  ; ys
        ja      .ret1
        cmp     rax,[rdi+8]   ; xe
        ja      .ret1
        cmp     rbx,[rdi+12] ; ye
        ja      .ret1

.ret0:
        xor     rcx,rcx
.ret1:
        pop     rdi
        ret


; check if pixel is allowed to be drawn

checkpixel:

        push    rax
        push    rbx
        push    rdx

        mov     rcx,[0x6000]
        shl     rcx,6
        add     rcx,0xc000
        mov     dx,word [rcx]

        cmp     dx,word [0x3004]
        jz      .ret0

        call    cplimit
        or      rcx,rcx    ; if (ecx == 0)
        jnz     .ret1

        mov     rdx,[0xfe00]     ; screen x size
        inc     rdx
        imul    rdx,rbx
        lea     rax,[rax+rdx+0x400000]
        mov     dl,[rax]

        mov     rax,[0x6000]
        shl     rax,5
        add     rax,0x6000+0x1C

        mov     rcx,1
        cmp     byte [rax], dl
        jnz     .ret1

.ret0:
        xor     rcx,rcx
.ret1:
        pop     rdx
        pop     rbx
        pop     rax
        ret


; activate window

windowactivate:

        ; rsi = abs mem position in stack 0xC400+

        pushdu

        push  rsi
        xor   eax,eax
        mov   ax,[rsi] ; ax <- process no
        shl   eax,1
        mov   ebx,eax
        add   ebx,0xc000
        xor   eax,eax
        mov   ax,[ebx] ; ax <- position in window stack

        mov   esi,0                 ; drop others
      waloop:
        cmp   esi,dword[0x6008]
        jb    waok
        jmp   wacont
      waok:

        inc   esi
        mov   edi,esi
        shl   edi,1
        add   edi,0xc000

        mov   bx,[rdi]
        cmp   bx,ax
        jbe   wanoc
        dec   bx
        mov   [rdi],bx

      wanoc:

        jmp   waloop

      wacont:
                            ; set to no 1
        pop   rsi

        xor   eax,eax
        mov   ax,[rsi]
        shl   eax,1
        add   eax,0xc000
        mov   bx,[0x6008]
        mov   [rax],bx

        ; update on screen -window stack

        mov   esi,0

      waloop2:

        mov   edi,[0x6008]
        cmp   esi,edi
        jb    waok2
        jmp   wacont2

      waok2:

        inc   esi

        mov   edi,esi
        shl   edi,1
        add   edi,0xc000
        movzx ebx,word [rdi]
        shl   ebx,1
        add   ebx,0xc400
        mov   ecx,esi
        mov   [rbx],cx
        jmp   waloop2

      wacont2:

        mov   [0xf400],byte 0           ; empty keyboard buffer
        mov   [0xf500],byte 0           ; empty button buffer

        popdu
        ret


mouse_active  db  0


; check misc

checkmisc:

        cmp     [ctrl_alt_del_long],1
        jne     nocpustart
    ;
  newcount:
        mov     rcx,0
        mov     rax,[application_table_status]
  newcadc:
        push    r8
        mov     r8,0
        cmp     [application_table_status],r8
        pop     r8
        je      contcad
        cmp     rax,[application_table_status]
        jne     newcount
        push    rax rbx rcx
        mov     rax , 1
        call    delay_hs
        pop     rcx rbx rax
        add     rcx , 1
        cmp     rcx , 200
        jb      newcadc
  contcad:
        push    r8
        mov     r8,0
        cmp     [application_table_status],r8
        pop     r8
        ;
        mov     rax,cpustring
        call    start_application_fl
        mov     [ctrl_alt_del_long],0
        jmp     nocpustart
  cpustring db 'CPU        '
  nocpustart:

        cmp     [mouse_active],1
        jne     mouse_not_active
        mov     [mouse_active],0
        mov     rdi,0
        mov     rcx,[0x6008]
   set_mouse_event:
        add     rdi,256
        or      [rdi+0x80000+0xA8],dword 00100000b
        loop    set_mouse_event
  mouse_not_active:


        cmp     [0xfff0],byte 0               ; background update ?
        jz      nobackgr
        mov     [0xfff0],byte 2
        call    change_task_long
        mov     [0xfff0],byte 0
        mov     [draw_data+32+0],dword 0
        mov     [draw_data+32+4],dword 0
        mov     rax,[0xfe00]
        mov     rbx,[0xfe04]
        mov     [draw_data+32+8],eax
        mov     [draw_data+32+12],ebx
        call    drawbackground
        mov     [0xfff4],byte 0
  nobackgr:


        ; system shutdown request

        cmp     [0xFF00],byte 0
        je      noshutdown

;        mov     rdx,[shutdown_processes]
;        sub     dl,2

;        cmp     [0xff00],dl
;        jne     no_mark_system_shutdown

;        mov     rdx,0x3040
;        movzx   rcx,byte [0xff00]
;        add     rcx,5
  markz:
        mov     [rdx+0xa],byte 3
        add     rdx,0x20
        loop    markz

  no_mark_system_shutdown:

        call    disable_mouse

        dec     byte [0xff00]

        cmp     [0xff00],byte 0
        jne     noshutdown  ;;  system_shutdown

 ;       jmp     system_shutdown ; shutdown.inc

        mov     rax , 200
        call    delay_hs

        ; Boot

        cli

        mov     rdx , 0x64 ; Boot with keyboard controller
        mov     rax , 0xfe
        out     dx,al

        cli
        jmp     $ ;  not propably needed


  noshutdown:


        mov     rax,[0x6008]                  ; termination
        mov     rbx,0x6040+0x14
        mov     rsi,1

  newct:
        mov     cl,[rbx]
        cmp     cl,byte 3
        jz      terminate_long
        cmp     cl,byte 4
        jz      terminate_long

        add     rbx,0x20
        inc     rsi
        dec     rax
        jnz     newct

        ret


find_pressed_button_frames:

        pushdu

        movzx   rbx,word [eax+0]
        shl     rbx,5
        add     rbx,window_data
        movzx   rcx,word [rbx+0]     ; window x start
        movzx   rdx,word [rax+4]     ; button x start
        add     rcx,rdx
        push    rcx

        mov     dx,[rax+6]     ; button x size
        add     cx,dx
        mov     rsi,rcx
        add     rsi,1
        mov     cx,[rbx+4]     ; window y start
        mov     dx,[rax+8]     ; button y start
        add     rcx,rdx
        mov     rbx,rcx
        mov     dx,[rax+10]    ; button y size
        add     dx,cx
        add     dx,1

        pop     rax

        ; rax x beginning
        ; rbx y beginning
        ; rsi x end
        ; rdx y end
        ; rcx color

        mov     [pressed_button_rax],rax
        mov     [pressed_button_rbx],rbx
        mov     [pressed_button_rcx],rcx
        mov     [pressed_button_rdx],rdx
        mov     [pressed_button_rsi],rsi

        popdu
        ret

pressed_button_rax  dq  0
pressed_button_rbx  dq  0
pressed_button_rcx  dq  0
pressed_button_rdx  dq  0
pressed_button_rsi  dq  0

; negative button image

negativebutton:
        ; If requested, do not display button
        ; boarder on press.
        test  rbx,0x20000000
        jz    draw_negative_button
        ret
      draw_negative_button:


        pushdu

        mov   rax,[pressed_button_rax]
        mov   rbx,[pressed_button_rbx]
        mov   rcx,[pressed_button_rcx]
        mov   rdx,[pressed_button_rdx]
        mov   rsi,[pressed_button_rsi]
        mov   rcx,0x01000000

        sub   rdx,1
        push  rdx
        add   rdx,1
        sub   rsi,1
        push  rsi
        add   rsi,1

        push  rax
        push  rbx
        push  rcx
        push  rdx
        push  rdi

      bdbnewline:
        mov   rdi,1    ; force
        cmp   rax,[rsp+32]
        jz    bneg
        cmp   rax,[rsp+40]
        jz    bneg
        cmp   rbx,[rsp+24]
        jz    bneg
        cmp   rbx,[rsp+48]
        jz    bneg
        jmp   nbneg

      bneg:

        call  disable_mouse
        call  putpixel

      nbneg:

        add   rax,1
        cmp   rax,rsi
        jnz   bdbnewline
        mov   rax,[rsp+32]
        add   rbx,1
        cmp   rbx,rdx
        jnz   bdbnewline

        add   rsp,56

        popdu

        ret

; check buttons


; 0000 word process number
; 0002 word button id number : bits 0-15
; 0004 word x start
; 0006 word x size
; 0008 word y start
; 000A word y size
; 000C word button id number : bits 16-31
;
; button table in 0x10 increments
;
; first at 0x10


checkbuttons:

        cmp     [0xfb40],byte 0    ; mouse buttons pressed
        jnz     check_buttons_continue
        ret

  check_buttons_continue:

        pushdu

        mov     rsi,0
        mov     rdi,[0xfe88]
        movzx   rdx,word [rdi]
        cmp     rdx,0
        jne     yesbuttoncheck
        popdu
        ret

  yesbuttoncheck:

        push    rsi
        inc     rdx
        push    rdx

  buttonnewcheck:

        pop     rdx
        pop     rsi
        inc     rsi
        cmp     rdx,rsi
        jge     bch

        popdu                 ; no button pressed
        ret

  bch:

        push    rsi
        push    rdx
        mov     rax,rsi
        shl     rax,4
        add     rax,rdi

        ; check that button is at top of windowing stack

        movzx   rbx,word [rax]
        shl     rbx,1
        add     rbx,0xc000
        movzx   rcx,word [rbx]
        cmp     rcx,[0x6008]
        jne     buttonnewcheck

        ; check that button start is inside window x/y end

        movzx   rbx,word [rax+0]
        shl     rbx,5
        add     rbx,window_data
        mov     rcx,[rbx+8]          ; window end X
        movzx   rdx,word [rax+4]     ; button start X
        cmp     rdx,rcx
        jge     buttonnewcheck

        mov     rcx,[rbx+12]         ; window end Y
        movzx   rdx,word [rax+8]     ; button start Y
        cmp     rdx,rcx
        jge     buttonnewcheck

        ; check coordinates
                                   ; mouse x >= button x ?
        movzx   rbx,word [rax+0]
        shl     rbx,5
        add     rbx,window_data
        movzx   rcx,word [rbx+0]     ; window x start
        movzx   rdx,word [rax+4]     ; button x start
        add     rdx,rcx
        mov     cx,[0xfb0a]
        cmp     rdx,rcx
        jg      buttonnewcheck

        movzx   rbx,word [rax+6]     ; button x size
        add     rdx,rbx
        cmp     rcx,rdx
        jg      buttonnewcheck

                                   ; mouse y >= button y ?
        movzx   rbx,word [rax+0]
        shl     rbx,5
        add     rbx,window_data
        movzx   rcx,word [rbx+4]     ; window y start
        movzx   rdx,word [rax+8]     ; button y start
        add     rdx,rcx
        mov     cx,[0xfb0c]
        cmp     rdx,rcx
        jg      buttonnewcheck

        movzx   rbx,word [eax+10]    ; button y size
        add     rdx,rbx
        cmp     rcx,rdx
        jg      buttonnewcheck

        ; mouse on button

        pop     rdx
        pop     rsi

        mov     bx,[rax+0xc]     ; button id : bits 16-31
        shl     rbx,16
        mov     bx,[rax+2]       ; button id : bits 00-16
        push    rbx

        mov     [0xfb44],byte 1  ; no mouse down checks
        call    find_pressed_button_frames
        call    negativebutton
        pushdu

  cbwaitmouseup:

        call    checkidle

        call    check_mouse_data
        call    draw_pointer

        pushdu
        call    stack_handler
        popdu

        cmp     [0xfb40],byte 0  ; mouse buttons pressed ?
        jnz     cbwaitmouseup
        popdu
        call    negativebutton
        mov     [0xfff4],byte 0  ; no mouse background
        mov     [0xfff5],byte 0  ; draw mouse
        mov     [0xf500],byte 1
        pop     rbx
        mov     [0xf501],ebx
        mov     [0xfb44],byte 0  ; mouse down checks
        popdu
        ret

; check if window is necessary to draw

checkwindowdraw:

        ; rdi = position in window_data+

        mov     esi,edi
        sub     esi,window_data
        shr     esi,5

        ; rsi = process nro

        mov     edx,esi
        shl     edx,1
        add     edx,0xc000

        xor     ecx,ecx
        mov     cx,[edx]
        shl     ecx,1
        add     ecx,0xc400 ; position in windowing stack, checks from here ->

        mov     esi,ecx

 ;       pushdu
 ;       xor   eax,eax
 ;       mov   ax,[rsi]
 ;       shl   eax,5
 ;       add   eax,window_data
 ;       mov   esi,eax
 ;       popdu

        push    rsi

      wdn0:

        pop     rsi
        add     esi,2
        push    rsi

        mov     eax,[0x6008]
        shl     eax,1
        add     eax,0xc400

        cmp     esi,eax
        jbe     wdn1

        pop     rsi

        mov     ecx,0       ; passed all windows to top
        ret

      wdn1:

        xor     eax,eax
        mov     ax,[esi]
        shl     eax,5
        add     eax,window_data
        mov     esi,eax

        mov     eax,[edi+0]
        mov     ebx,[edi+4]
        mov     ecx,[edi+8]
        mov     edx,[edi+12]
        add     ecx,eax
        add     edx,ebx

    ;    dec   eax ; 0.80
    ;    dec   ebx
    ;    inc   ecx ; 0.80
    ;    inc   edx

        mov     ecx,[esi+4]    ; y check
        cmp     ecx,edx
        jb      wici1
        jmp     wdn0
      wici1:
        mov     eax,[esi+12]
        add     ecx,eax
        cmp     ebx,ecx
        jbe     wici2
        jmp     wdn0
      wici2:

        mov     eax,[edi+0]
        mov     ebx,[edi+4]
        mov     ecx,[edi+8]
        mov     edx,[edi+12]
        add     ecx,eax
        add     edx,ebx

        sub     ecx , 2

    ;    dec   eax ; 0.80
    ;    dec   ebx
    ;    inc   ecx ; 0.80
    ;    inc   edx

        mov     edx,[esi+0]    ; x check
        cmp     edx,ecx
        jb      wici3
        jmp     wdn0
      wici3:
        mov     ecx,[esi+8]
        add     edx,ecx
        cmp     eax,edx
        jbe     wici4
        jmp     wdn0
      wici4:

        pop     rsi
        mov     ecx,1   ; overlap some window
        ret


waredraw:     ; if redraw necessary at activate

        pushdu

        call    checkwindowdraw      ; draw window on activation ?

        cmp     rcx,0
        jnz     wand2                ; yes
        jmp     wand                 ; no

      wand2:

        popdu
        mov     [0xfb44],byte 1
        call    windowactivate

        ; update screen info

        pushdu
        mov     rdi,[0x6008]
        shl     edi,1
        add     edi,0xc400
        mov     esi,[rdi]
        and     esi,65535
        shl     esi,5
        add     esi,window_data

        mov     eax,[rsi+00]
        mov     ebx,[rsi+04]
        mov     ecx,[rsi+8]
        mov     edx,[rsi+12]

        add     ecx,eax
        add     edx,ebx

        mov     edi,[0x6008]
        shl     edi,1
        add     edi,0xc400
        mov     esi,[rdi]
        and     esi,255
        shl     esi,5
        add     esi,0x6000+0x1C
        movzx   esi,byte[rsi]
        call    calculatescreen ; setscreen

        popdu

        cmp     [0xff01],dword 1
        jbe     wand5

        mov     eax,10            ; wait for putimages to finish
        call    delay_hs

        mov     [rdi+31],byte 1  ; redraw flag for app
        mov     [0xfb44],byte 0  ; mouse down checks

        ret

      wand5:

        mov     eax,5            ; wait for putimages to finish
        call    delay_hs

        mov     [rdi+31],byte 1  ; redraw flag for app
        mov     ecx,100

       cwwaitflagdown:

        dec     ecx
        jz      cwnowait

        mov     eax,2
        call    delay_hs
        cmp     [rdi+31],byte 0  ; wait flag to drop
        jnz     cwwaitflagdown

      cwnowait:

        mov     ecx,10
      cwwait:
        mov     eax,1           ; wait for draw to finish
        call    delay_hs
        loop    cwwait

        mov     [0xfb44],byte 0

        ret

      wand:

        popdu

        call    windowactivate
        mov     [0xfb44],byte 0  ; mouse down checks
        mov     [0xfff4],byte 0  ; no mouse background
        mov     [0xfff5],byte 0  ; draw mouse
        ret


window_moving   db 'Kernel : Window - move/resize',13,10,0
window_moved    db 'Kernel : Window - done',13,10,0

; check window touch

checkwindows:


        pushdu

        cmp     [0xff01],dword 1  ; activate request from app ?
        jbe     cwl1
        mov     rdi,[0xff01]
        shl     rdi,5
        add     rdi,window_data
        mov     rbx,[0xff01]
        shl     rbx,1
        add     rbx,0xc000
        mov     rsi,[rbx]
        and     rsi,65535
        shl     rsi,1
        add     rsi,0xc400
        call    waredraw
        mov     [0xff01],dword 0

        popdu
        ret

      cwl1:

        cmp     [0xfb40],byte 0    ; mouse buttons pressed ?
        jne     cwm
        popdu
        ret

      cwm:

        mov     rsi,[0x6008]
        inc     rsi

      cwloop:

        cmp     rsi,2
        jge     cwok
        popdu
        ret

      cwok:
        dec     rsi
        mov     rax,rsi
        shl     rax,1
        add     rax,0xc400
        xor     rbx,rbx
        mov     bx,[rax]
        shl     rbx,5
        add     rbx,window_data
        mov     rdi,rbx
        mov     ax,[0xfb0a]
        mov     bx,[0xfb0c]
        mov     cx,[rdi+0]
        mov     dx,[rdi+8]
        cmp     cx,ax
        jb      cw1
        jmp     cwloop
      cw1:
        cmp     dx,bx
        jb      cw2
        jmp     cwloop
      cw2:
        add     cx,[rdi+16]
        add     dx,[rdi+24]
        cmp     ax,cx
        jb      cw3
        jmp     cwloop
      cw3:
        cmp     bx,dx
        jb      cw4
        jmp     cwloop
      cw4:

        pushdu
        mov     rax,rsi
        mov     bx,[0x6008]
        cmp     ax,bx
        jnz     nomovwin
        jmp     movwin
       nomovwin:
        ; rax = position in windowing stack
        ; redraw must ?
        shl     rsi,1
        add     rsi,0xc400
        call    waredraw
        add     rsp,64

        popdu
        ret

      movwin:    ; MOVE OR RESIZE WINDOW

        popdu
        ; Check for user enabled fixed window
        mov      rdx,[rdi+0x28]
        and      rdx,0x0f000000
        cmp      rdx,0x01000000
        jne      window_move_enabled_for_user
        popdu
        ret
      window_move_enabled_for_user:


        mov     [do_resize_from_corner],byte 0   ; resize for skinned window
        mov     rdx,[rdi+0x20]
        and     rdx,0x0f000000
        cmp     rdx,0x02000000
        jb      no_resize_2
        mov     dx,[rdi+8]
        add     dx,[rdi+24]
        sub     dx,6
        cmp     bx,dx
        jb      no_resize_2
        mov     [do_resize_from_corner],byte 1
        jmp     cw5
      no_resize_2:

        mov     dx,[rdi+8] ; check if touch on bar
        add     dx,21
        cmp     bx,dx
        jb      cw5
        popdu
        ret

      cw5:

        push    rsi
        mov     rsi,window_moving
        call    sys_msg_board_str
        pop     rsi

        mov     rcx,[0xfdf0]    ; double-click ?
        mov     rdx,rcx
        sub     edx,[latest_window_touch]
        mov     [latest_window_touch],ecx
        mov     [latest_window_touch_delta],edx

        mov     cl,[0xfb40]     ; save for shade check
        mov     [do_resize],cl

        movzx   rcx,word [rdi+0]
        movzx   rdx,word [rdi+8]

        pushdu
        mov     [dlx],ecx      ; save for drawlimits
        mov     [dly],edx
        mov     rax,[rdi+8]
        add     rcx,rax
        mov     rax,[rdi+12]
        add     rdx,rax
        mov     [dlxe],ecx
        mov     [dlye],edx
        popdu

        sub     ax,cx
        sub     bx,dx

        mov     rsi,[0xfb0a]
        mov     [0xf300],esi

        pushdu           ; wait for putimages to finish
        mov     rax,5
        call    delay_hs
        mov     rax,[rdi+0]
        mov     [npx],eax
        mov     rax,[rdi+4]
        mov     [npy],eax
        popdu

        pushdu                  ; save old coordinates
        mov     ax,[rdi+00]
        mov     word [oldc+00],ax
        mov     ax,[rdi+04]
        mov     word [oldc+04],ax
        mov     ax,[rdi+8]
        mov     word [oldc+8],ax
        mov     word [npxe],ax
        mov     ax,[rdi+12]
        mov     word [oldc+12],ax
        mov     word [npye],ax
        popdu

        call    drawwindowframes

        mov     [reposition],0
        mov     [0xfb44],byte 1   ; no reaction to mouse up/down

        ; move window

      newchm:

        mov     [0xfff5],byte 1

        call    checkidle

        call    checkEgaCga

        mov     [0xfff4],byte 0

        call    check_mouse_data
        call    draw_pointer

        pushdu
        call    stack_handler
        popdu

        mov     rsi,[0xf300]
        cmp     rsi,[0xfb0a]
        je      cwb

        mov     cx,[0xfb0a]
        mov     dx,[0xfb0c]
        sub     cx,ax
        sub     dx,bx
        push    ax
        push    bx

        call    drawwindowframes

        mov     ax,[0xfe00]
        mov     bx,[0xfe04]

        cmp     [do_resize_from_corner],1
        je      no_new_position

        mov     word [npx],word 0     ; x repos ?
        cmp     ax,cx
        jb      noreposx
        mov     [reposition],1
        sub     ax,word [npxe]
        mov     word [npx],ax
        cmp     ax,cx
        jb      noreposx
        mov     word [npx],cx
      noreposx:

        mov     word [npy],word 0     ; y repos ?
        cmp     bx,dx
        jb      noreposy
        mov     [reposition],1
        sub     bx,word [npye]
        mov     word [npy],bx
        cmp     bx,dx
        jb      noreposy
        mov     word [npy],dx
      noreposy:

      no_new_position:

        cmp     [do_resize_from_corner],0    ; resize from right corner
        je      norepos_size
        pushdu

        mov     rdx,rdi
        sub     rdx,window_data
        shr     rdx,5
        shl     rdx,8
        add     rdx,0x80000                 ; process base at 0x80000+

        movzx   rax,word [0xfb0a]
        cmp     rax,[rdi+0]
        jb      nnepx
        sub     rax,[rdi+0]
        cmp     rax,[rdx+0x90+8]
        jge     nnepx2
        mov     rax,[rdx+0x90+8]
      nnepx2:
        mov     [npxe],eax
      nnepx:

        movzx   rax,word [0xfb0c]
        cmp     rax,[rdi+4]
        jb      nnepy
        sub     rax,[rdi+4]
        cmp     rax,23 ; [rdx+0x90+12]
        jge     nnepy2
        mov     rax,23 ; [rdx+0x90+12]
      nnepy2:
        mov     [npye],eax
      nnepy:

        mov     [reposition],1

        popdu
      norepos_size:

        pop     bx
        pop     ax
        call    drawwindowframes

        mov     rsi,[0xfb0a]
        mov     [0xf300],esi

      cwb:

        cmp     [0xfb40],byte 0
        jne     newchm
                                     ; new position done
        call    drawwindowframes
        mov     [0xfff5],byte 1

        mov     eax,[npx]
        mov     [edi+0],eax
        mov     eax,[npy]
        mov     [edi+4],eax
        mov     eax,[npxe]
        mov     [edi+8],eax
        mov     eax,[npye]
        mov     [edi+12],eax

        cmp     [reposition],1         ; save new X and Y start
        jne     no_xy_save
        pushdu
        mov     rsi,rdi
        sub     rdi,window_data
        shr     rdi,5
        shl     rdi,8
        add     rdi,0x80000+0x90
        mov     rcx,2
        cld
        rep     movsd
        popdu
      no_xy_save:

        pushdu                             ; WINDOW SHADE/FULLSCREEN

        cmp     [reposition],1
        je      no_window_sizing

        mov     rdx,rdi
        sub     rdx,window_data
        shr     rdx,5
        shl     rdx,8
        add     rdx,0x80000                 ; process base at 0x80000+

        cmp     [do_resize],2               ; window shade ?
        jb      no_window_shade
        mov     [reposition],1
        cmp     [rdi+12],dword 23
        je      window_shade_up
        mov     [rdi+12],dword 23           ; on
        jmp     no_window_shade
      window_shade_up:
        mov     rax,[rdi+0]
        add     rax,[rdi+4]
        cmp     rax,0
        je      shade_full
        mov     rax,[rdx+0x9C]              ; off
        mov     [rdi+12],eax
        jmp     no_window_shade
      shade_full:
        mov     rax,[0xfe04]
        mov     [rdi+12],eax
      no_window_shade:

        cmp     [do_resize],1               ; fullscreen/restore ?
        jne     no_fullscreen_restore
        cmp     [latest_window_touch_delta],dword 50
        jg      no_fullscreen_restore
        mov     [reposition],1
        mov     rax,[rdi+12]
        cmp     rax,[0xfe04]
        je      restore_from_fullscreen
        mov     [rdi+0],dword 0             ; set fullscreen
        mov     [rdi+4],dword 0
        mov     rax,[0xfe00]
        mov     [rdi+8],eax
        mov     rax,[0xfe04]
        mov     [rdi+12],eax
        jmp     no_fullscreen_restore
      restore_from_fullscreen:
        push    rdi                         ; restore
        mov     rsi,rdx
        add     rsi,0x90
        mov     rcx,4
        cld
        rep     movsd
        pop     rdi
      no_fullscreen_restore:

        mov     rax,[rdi+4]                 ; check Y inside screen
        add     rax,[rdi+12]
        cmp     rax,[0xfe04]
        jbe     no_window_sizing
        mov     rax,[0xfe04]
        sub     rax,[rdi+12]
        mov     [rdi+4],eax
      no_window_sizing:

        popdu

        cmp     [reposition],0
        je      retwm

        pushdu
        mov     rax,[rdi+00]
        mov     rbx,[rdi+04]
        mov     rcx,[rdi+8]
        mov     rdx,[rdi+12]
        add     rcx,rax
        add     rdx,rbx
        mov     rdi,[0x3004]
        shl     rdi,1
        add     rdi,0xc400
        movzx   rsi,byte [rdi]
        shl     rsi,5
        add     rsi,0x3000+0xe
        movzx   rsi,byte [rsi]

        sub     rdi,draw_data
        shr     rdi,5
        shl     rdi,8
        add     rdi,0x80000+0x80
        cmp     [rdi],dword 0
        jne     no_rect_shaped_move
        call    setscreen
        jmp     move_calculated
      no_rect_shaped_move:
        call    calculatescreen
      move_calculated:

        popdu

        mov     [rdi+31],byte 1 ; mark first as redraw
        mov     [0xfff5],byte 1 ; no mouse

        pushdu
        mov     eax,[oldc+00]
        mov     ebx,[oldc+04]
        mov     ecx,[oldc+8]
        mov     edx,[oldc+12]
        add     rcx,rax
        add     rdx,rbx
        call    calculatescreen
        popdu

        mov     rax,rdi
        call    redrawscreen

        mov     rcx,100         ; wait to avoid mouse residuals
      waitre2:
        call    check_mouse_data
        mov     [0xfff5],byte 1
        call    checkidle
        cmp     [rdi+31],byte 0
        jz      retwm
        loop    waitre2

      retwm:

        mov     [0xfff5],byte 0 ; mouse pointer
        mov     [0xfff4],byte 0 ; no mouse under
        mov     [0xfb44],byte 0 ; react to mouse up/down

        mov     rsi,window_moved
        call    sys_msg_board_str

        popdu
        ret

do_resize_from_corner      db  0x0
reposition                 db  0x0
latest_window_touch        dd  0x0
latest_window_touch_delta  dd  0x0

do_resize db 0x0

oldc    dd 0x0,0x0,0x0,0x0

dlx     dd 0x0
dly     dd 0x0
dlxe    dd 0x0
dlye    dd 0x0

npx     dd 0x0
npy     dd 0x0
npxe    dd 0x0
npye    dd 0x0

mpx     dd 0x0
mpy     dd 0x0


; draw negative window frames

drawwindowframes:

        pushdu

        mov   eax,[npx]
        shl   eax,16
        add   eax,[npx]
        add   eax,[npxe]
        add   eax,65536*1-1
        mov   ebx,[npy]
        shl   ebx,16
        add   ebx,[npy]
        mov   ecx,0x01000000
        push  rdi
        mov   edi,1
        call  draw_line
        pop   rdi

        mov   eax,[npx]
        shl   eax,16
        add   eax,[npx]
        add   eax,[npxe]
        add   eax,65536*1-1
        mov   ebx,[npy]
        add   ebx,[npye]
        shl   ebx,16
        add   ebx,[npy]
        add   ebx,[npye]
        mov   ecx,0x01000000
        push  rdi
        mov   edi,1
        call  draw_line
        pop   rdi

        mov   eax,[npx]
        shl   eax,16
        add   eax,[npx]
        mov   ebx,[npy]
        shl   ebx,16
        add   ebx,[npy]
        add   ebx,[npye]
        mov   ecx,0x01000000
        push  rdi
        mov   edi,1
        call  draw_line
        pop   rdi

        mov   eax,[npx]
        add   eax,[npxe]
        shl   eax,16
        add   eax,[npx]
        add   eax,[npxe]
        mov   ebx,[npy]
        shl   ebx,16
        add   ebx,[npy]
        add   ebx,[npye]
        mov   ecx,0x01000000
        push  rdi
        mov   edi,1
        call  draw_line
        pop   rdi

        popdu

        ret


; redraw screen

redrawscreen:

; rax , if process window_data base is eax, do not set flag/limits

         pushdu
         push   rax

         mov    eax,2
         call   delay_hs

         mov    ecx,0               ; redraw flags for apps

       newdw2:

         inc    ecx
         push   rcx

         mov    eax,ecx
         shl    eax,5
         add    eax,window_data

         cmp    eax,[rsp+8]
         je     not_this_task
                                   ; check if window in redraw area
         mov    edi,eax

         cmp    ecx,1               ; limit for background
         jz     bgli

         mov    eax,[rdi+0]
         mov    ebx,[rdi+8]
         mov    ecx,[rdi+16]
         mov    edx,[rdi+24]
         add    ecx,eax
         add    edx,ebx

         mov    ecx,[dlye]   ; rcx = area y end     rbx = window y start
         cmp    ecx,ebx
         jb     ricino

         mov    ecx,[dlxe]   ; rcx = area x end     rax = window x start
         cmp    ecx,eax
         jb     ricino

         mov    rax,[rdi+0]
         mov    rbx,[rdi+8]
         mov    rcx,[rdi+16]
         mov    rdx,[rdi+24]
         add    ecx,eax
         add    edx,ebx

         mov    eax,[dly]    ; rax = area y start     rdx = window y end
         cmp    edx,eax
         jb     ricino

         mov    eax,[dlx]    ; rax = area x start     rcx = window x end
         cmp    ecx,eax
         jb     ricino

        bgli:

         cmp    edi,esi
         jz     ricino

         mov    eax,edi
         add    rax,draw_data-window_data

         mov    ebx,[dlx]          ; set limits
         mov    [rax+0],ebx
         mov    ebx,[dly]
         mov    [rax+4],ebx
         mov    ebx,[dlxe]
         mov    [rax+8],ebx
         mov    ebx,[dlye]
         mov    [rax+12],ebx

         sub    eax,draw_data-window_data

         cmp    ecx,1
         jne    nobgrd
         cmp    esi,1
         je     newdw8
         call   drawbackground
       newdw8:
       nobgrd:

         mov    [rax+31],byte 1    ; mark as redraw

       ricino:

       not_this_task:

         pop    rcx

         cmp    ecx,[0x6008]
         jg     newdw3
         jmp    newdw2

       newdw3:

         pop    rax
         popdu

         ret

;   check mouse
;
;
;   FB00  ->   FB0F   mouse memory 00 chunk count - FB0A-B x - FB0C-D y
;   FB10  ->   FB17   mouse color mem
;   FB21              x move
;   FB22              y move
;   FB30              color temp
;   FB28              high bits temp
;   FB4A  ->   FB4D   FB4A-B x-under - FB4C-D y-under
;   FC00  ->   FCFE   com1/ps2 buffer
;   FCFF              com1/ps2 buffer count starting from FC00

mousecount  dd  0x0
mousedata   dd  0x0


check_mouse_data:

        pushdu

        mov     [mousecount],dword 0x2e0000+12*4096
        mov     [mousedata],dword 0x2e0000+12*4096+0x10
        cmp     [0xF604],byte 2
        jne     nocom1mouse
        mov     [mousecount],dword 0x2e0000+4*4096
        mov     [mousedata],dword 0x2e0000+4*4096+0x10
      nocom1mouse:
        cmp     [0xF604],byte 3
        jne     nocom2mouse
        mov     [mousecount],dword 0x2e0000+3*4096
        mov     [mousedata],dword 0x2e0000+3*4096+0x10
      nocom2mouse:

      uusicheckmouse:

        mov     ebx,[mousecount]       ; anything at buffer for mouse
        cmp     [rbx],byte 0
        jnz     c_byte

        jmp     checkmouseret

      c_byte:

        ; first byte of comX or ps2 ?

        cmp     [0xF604],byte 2
        jge     com1mousefirst
        jmp     ps2mousefirst

        ; ******************************************
        ; *********** COMX mouse driver ************
        ; ******************************************

       com1mousefirst:

        mov     edi,[mousedata]
        mov     dl,byte [rdi] ; first com1 ?
        and     dl,64
        cmp     dl,64
        jnz     cm2
        mov     [0xfb00],byte 0  ; zero mouse block count
       cm2:
        xor     rbx,rbx

        mov     bl,[0xfb00]
        add     bl,1
        mov     [0xfb00],bl
        mov     rax,0xfb00
        add     rax,rbx
        mov     edi,[mousedata]
        mov     dl,byte [rdi]
        mov     [rax],byte dl
        cmp     bl,3             ; three ?
        jz      com1mouse

        jmp     decm

      com1mouse:

        ; buttons

        movzx   rax,byte [0xfb01]
        shr     rax,4
        and     rax,3
        mov     [0xfb40],al


        ; com1 mouse
        ; x

        mov     dl,[0xfb01]        ; x high bits
        movzx   rax,dl
        and     al,3
        shl     al,6
        mov     dl,byte[0xfb02]    ; x low bits
        add     al,dl
        mov     [0xfb21],byte al
        movzx   rbx,word[0xfb0a]

        mov     al,byte [0xfb01]   ; + or - ?
        and     al,2
        cmp     al,2
        jnz     x_add

       x_sub:
        movzx   rbx,word[0xfb0a]   ; x-
        movzx   rax,byte [0xfb21]
        sub     bx,255
        add     bx,ax
        push    rbx
        mov     [0xfb00],byte 0
        jmp     my_event
       x_add:
        movzx   rbx,word[0xfb0a]   ; x+
        movzx   rax,byte [0xfb21]
        add     bx,ax
        push    rbx
        mov     [0xfb00],byte 0


        ; y


      my_event:

        mov     dl,[0xfb01]       ; y high bits
        movzx   rax,dl
        and     al,12
        shl     al,4
        mov     dl,byte[0xfb03]   ; y low bits
        add     al,dl
        mov     [0xfb22],byte al
        movzx   rbx,word[0xfb0c]

        mov     al,byte [0xfb01]  ; + or - ?
        and     al,8
        cmp     al,8
        jnz     y_add

      y_sub:
        movzx   rbx,word[0xfb0c]  ; y-
        movzx   rax,byte [0xfb22]
        sub     bx,255
        add     bx,ax
        push    rbx
        mov     [0xfb00],byte 0
        jmp     mdraw
      y_add:
        movzx   rbx,word[0xfb0c]  ; y+
        movzx   rax,byte [0xfb22]
        add     bx,ax
        push    rbx
        mov     [0xfb00],byte 0
        jmp     mdraw

        ; end of com1 mouse



        ; ******************************************
        ; ********  PS2 MOUSE DRIVER  **************
        ; ******************************************

      ps2mousefirst:

        movzx   rdx,byte [0x2E0000+4096*12+0x10]   ; first ps2 ?
        cmp     rdx,40
        jne     cm3
        mov     [0xfb00],byte 0  ; zero mouse block count
      cm3:

        movzx   rbx,byte [0xfb00]
        add     rbx,1
        mov     [0xfb00],bl
        mov     rax,0xfb00
        add     rax,rbx
        mov     dl,byte [0x2E0000+4096*12+0x10]
        mov     [rax],byte dl

        cmp     bl,3             ; full packet of three bytes ?
        jz      ps2mouse
        jmp     decm


      ps2mouse:

        mov     [0xfb00],byte 0  ; zero mouse block count

        ; buttons

        movzx   rax,byte [0xfb01]
        and     rax,3
        mov     [0xfb40],al

        ; x

        movzx   rax,word [0xfb0a]
        movzx   rdx,byte [0xfb02]
        cmp     rdx,128
        jb      ps2xp
        shl     rdx,1
        add     rax,rdx
        cmp     rax,512
        jge     ps2xsok
        mov     rax,0
        jmp     ps2xready
       ps2xsok:
        sub     rax,512
        jmp     ps2xready
       ps2xp:
        shl     rdx,1
        add     rax,rdx
        jmp     ps2xready
       ps2xready:
        push    rax

        ; y

        movzx   rax,word [0xfb0c]
        movzx   rdx,byte [0xfb03]
        cmp     rdx,128
        jb      ps2yp
        add     rax,512
        shl     rdx,1
        sub     rax,rdx
        jmp     ps2yready
       ps2yp:
        shl     rdx,1
        cmp     rdx,rax
        jb      ps201
        mov     rdx,rax
       ps201:
        sub     rax,rdx
        jmp     ps2yready
       ps2yready:
        push    rax

        jmp     mdraw

        ; end of ps2 mouse


        ; ****************************
        ; ***** CHECK FOR LIMITS *****
        ; ****************************

      mdraw:

        cmp     [0xfb44],byte 0
        jne     mousedraw4
        cmp     [0xfb40],byte 0
        je      mousedraw4
        mov     [0xfff5],byte 1

      mousedraw4:

        pop     rbx
        pop     rax

        mov     [mouse_active],1

        mov     dx,0                   ; smaller than zero
        cmp     bx,dx
        jge     mnb11
        mov     bx,0
      mnb11:
        mov     [0xfb0c],word bx

        mov     dx,0
        cmp     ax,dx
        jge     mnb22
        mov     ax,0
      mnb22:
        mov     [0xfb0a],word ax

        mov     rdx,[0xfe04]           ; bigger than maximum
        cmp     rbx,rdx
        jb      mnb1
        mov     bx,[0xfe04]
      mnb1:
        mov     [0xfb0c],word bx

        mov     rdx,[0xfe00]
        cmp     rax,rdx
        jb      mnb2
        mov     ax,[0xfe00]
      mnb2:
        mov     [0xfb0a],word ax


        ; ****   NEXT DATA BYTE FROM MOUSE BUFFER   ****

      decm:

        mov     edi,[mousecount]         ; decrease counter
        dec     qword [rdi]

        mov     esi,[mousedata]
        mov     rdi,rsi
        add     rsi,1
        mov     rcx,250
        cld
        rep     movsb

        jmp     uusicheckmouse

      checkmouseret:

        cmp     [0xfb44],byte 0
        jne     cmret
        cmp     [0xfb40],byte 0
        je      cmret
        mov     [0xfff4],byte 0
        mov     [0xfff5],byte 0
      cmret:

        popdu

        ret


draw_mouse_under:

        ; return old picture

        pushdu

        xor     rcx,rcx
        xor     rdx,rdx

      mres:

        movzx   rax,word [0xfb4a]
        movzx   rbx,word [0xfb4c]

        add     rax,rcx
        add     rbx,rdx

        push    rcx
        push    rdx
        push    rax
        push    rbx

        mov     rax,rdx
        shl     rax,6
        shl     rcx,2
        add     rax,rcx
        add     eax,mouseunder
        mov     rcx,[rax]

        pop     rbx
        pop     rax

        push    rdi
        mov     rdi,1 ;force
        call    putpixel
        pop     rdi

        pop     rdx
        pop     rcx

        add     rcx,1
        cmp     rcx,16
        jnz     mres
        xor     rcx,rcx
        add     rdx,1
        cmp     rdx,24
        jnz     mres

        popdu
        ret


save_draw_mouse:

        ; save & draw

        mov     [0xfb4a],ax
        mov     [0xfb4c],bx
        push    rax
        push    rbx
        mov     rcx,0
        mov     rdx,0

      drm:

        push    rax
        push    rbx
        push    rcx
        push    rdx

        pushdu
        add     rax,rcx  ; save picture under mouse
        add     rbx,rdx
        push    rcx
        call    getpixel
        mov     [0xfb30],ecx
        pop     rcx
        mov     rax,rdx
        shl     rax,6
        shl     rcx,2
        add     rax,rcx
        add     rax,mouseunder
        mov     rbx,[0xfb30]
        mov     [rax],rbx
        popdu

        mov     rdi,rdx           ; y cycle
        shl     rdi,4             ; *16 bytes per row
        add     rdi,rcx           ; x cycle
        mov     rsi,rdi
        add     rdi,rsi
        add     rdi,rsi          ; *3
        add     rdi,[0xf200]      ; we have our str address
        mov     rsi,rdi
        add     rsi,16*24*3
        push    rcx
        mov     rcx,[0xfb30]
        call    combine_colors
        mov     [0xfb10],ecx
        pop     rcx


        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax

        add     rax,rcx       ; we have x coord+cycle
        add     rbx,rdx       ; and y coord+cycle

        pushdu
        mov     rcx,[0xfb10]
        mov     rdi,1
        call    putpixel
        popdu

      mnext:

        mov     rbx,[rsp+0]      ; pure y coord again
        mov     rax,[rsp+8]      ; and x

        add     rcx,1        ; +1 cycle
        cmp     rcx,16       ; if more than 16
        jnz     drm
        mov     rcx,0
        add     rdx,1
        cmp     rdx,24
        jnz     drm

        pop     rbx
        pop     rax

        ret


disable_mouse:

        pushdu

        mov     r8,1
        cmp     [0x6000],r8
        je      disable_m

        mov     rdx,[0x6000]
        shl     rdx,5
        add     rdx,window_data

        movzx   rax,word [0xfb0a]
        movzx   rbx,word [0xfb0c]

        mov     rcx,[0xfe00]
        inc     rcx
        imul    rcx,rbx
        add     rcx,rax
        add     rcx,0x400000

        movzx   rax,byte [rdx+twdw+0xe]

        movzx   rbx,byte [rcx]
        cmp     rax,rbx
        je      yes_mouse_disable
        movzx   rbx,byte [rcx+16]
        cmp     rax,rbx
        je      yes_mouse_disable

        mov     rbx,[0xfe00]
        inc     rbx
        imul    rbx,10
        add     rcx,rbx

        movzx   rbx,byte [rcx]
        cmp     rax,rbx
        je      yes_mouse_disable

        mov     rbx,[0xfe00]
        inc     rbx
        imul    rbx,10
        add     rcx,rbx

        movzx   rbx,byte [rcx]
        cmp     rax,rbx
        je      yes_mouse_disable
        movzx   rbx,byte [rcx+16]
        cmp     rax,rbx
        je      yes_mouse_disable

        jmp     no_mouse_disable

    yes_mouse_disable:

        mov     rdx,[0x6000]
        shl     rdx,5
        add     rdx,window_data

        movzx   rax,word [0xfb0a]
        movzx   rbx,word [0xfb0c]

        mov     rcx,[rdx+0]   ; mouse inside the area ?
        add     rax,14
        cmp     rax,rcx
        jb      no_mouse_disable
        sub     rax,14

        add     rcx,[rdx+8]
        cmp     rax,rcx
        jg      no_mouse_disable

        mov     ecx,[rdx+4]
        add     ebx,20
        cmp     ebx,ecx
        jb      no_mouse_disable
        sub     ebx,20

        add     ecx,[edx+12]
        cmp     ebx,ecx
        jg      no_mouse_disable

    disable_m:

        cmp     dword [0xf204],dword 0
        jne     nodmu
        call    draw_mouse_under
    nodmu:

        mov     [0xf204],dword 1

    no_mouse_disable:

        popdu

        ret



draw_pointer:

        pushdu

        cmp     dword [0xf204],dword 0  ; mouse visible ?
        je      chms00

        dec     dword [0xf204]

        cmp     [0xf204],dword 0
        jne     nodmu2

        movzx   rbx,word [0xfb0c]
        movzx   rax,word [0xfb0a]
        call    save_draw_mouse

        popdu
        ret

      nodmu2:

        popdu
        ret

      chms00:

        popdu

        pushdu

        cmp     [0xf204],dword 0
        jne     nodmp

        movzx   rcx,word [0xfb4a]
        movzx   rdx,word [0xfb4c]

        movzx   rbx,word [0xfb0c]
        movzx   rax,word [0xfb0a]

        cmp     rax,rcx
        jne     redrawmouse

        cmp     rbx,rdx
        jne     redrawmouse

        jmp     nodmp

      redrawmouse:

        call    draw_mouse_under

        call    save_draw_mouse

     nodmp:

        popdu

        ret

imax    dd 0x0

set_app_param:

        pushdu

        mov     rdi,[0x6020]
        mov     [rdi],rax

        popdu
        ret



delay_hs:     ; delay in 1/100 secs

        push    rax
        push    rcx
        push    rdx

        mov     rdx,[0xfdf0]
        add     rdx,rax

      newtic:
        mov     rcx,[0xfdf0]
        cmp     rdx,rcx
        jbe     zerodelay

        call    change_task_long

        jmp     newtic

      zerodelay:
        pop     rdx
        pop     rcx
        pop     rax

        ret


memmove:       ; memory move in bytes

        ; rax = from
        ; rbx = to
        ; rcx = no of bytes

        pushdu

        cld

        ; rcx no to move

        mov     rsi,rax
        mov     rdi,rbx
        rep     movsb

        popdu
        ret



random_shaped_window:

;
;  rax = 0    giving address of data area
;      rbx    address
;  rbx = 1    shape area scale
;      rbx    2^rbx scale

        cmp     rax,0
        jne     rsw_no_address
        mov     rax,[0x6000]
        shl     rax,8

        mov     [rax+0x80000+0x80],rbx
   rsw_no_address:

        cmp     rax,1
        jne     rsw_no_scale
        mov     rax,[0x6000]
        shl     rax,8
        mov     [rax+0x80000+0x84],bl
   rsw_no_scale:

        ret


; calculate fat chain

calculatefatchain:

        pushdu

        mov     rsi,fat_base+512
        mov     rdi,fat_table ;0x280000

  fcnew:
        xor     rax,rax
        xor     rbx,rbx
        xor     rcx,rcx
        xor     rdx,rdx
        mov     al,[rsi+0]  ; 1
        mov     bl,[rsi+1]
        and     rbx,15
        shl     rbx,8
        add     rax,rbx
        mov     [rdi],ax
        add     rdi,2

        xor     rax,rax
        xor     rbx,rbx
        xor     rcx,rcx
        xor     rdx,rdx
        mov     bl,[rsi+1]  ; 2
        mov     cl,[rsi+2]
        shr     rbx,4
        shl     rcx,4
        add     rcx,rbx
        mov     [rdi],cx
        add     rdi,2

        add     rsi,3

        cmp     rdi,fat_table+4100*4 ;0x280000+4100*4
        jnz     fcnew

        popdu
        ret


restorefatchain:   ; restore fat chain

        pushdu

        mov     rsi,fat_table ;0x280000
        mov     rdi,fat_base+512

  fcnew2:
        cld
        xor     rax,rax
        xor     rbx,rbx
        xor     rcx,rcx                    ;   rsi  XXXXxxxxxxxx  yyyyyyyyYYYY
        xor     rdx,rdx
        mov     ax,[rsi]                   ;   rdi  xxxxxxxx YYYYXXXX yyyyyyyy
        mov     bx,ax
        shr     bx,8
        and     rbx,15
        mov     [rdi+0],al  ; 1 -> 1 & 2
        mov     [rdi+1],bl
        add     rsi,2

        xor     rax,rax
        xor     rbx,rbx
        xor     rcx,rcx
        xor     rdx,rdx
        mov     bx,[rsi]
        mov     cx,bx
        shr     rcx,4
        mov     [rdi+2],cl
        and     rbx,15
        shl     rbx,4
        mov     rdx,[rdi+1]
        add     rdx,rbx
        mov     [rdi+1],dl  ; 2 -> 2 & 3
        add     rsi,2

        add     rdi,3

        cmp     rdi,fat_base+512+0x1200
        jb      fcnew2

        mov     rsi,fat_base+512           ; duplicate fat chain
        mov     rdi,fat_base+512+0x1200
        mov     rcx,0x1200/4
        cld
        rep     movsq

        popdu
        ret

;
;align 4
;
;read_floppy_file:
;
;; as input
;;
;; eax pointer to file
;; ebx file lenght
;; ecx start 512 byte block number
;; edx number of blocks to read
;; esi pointer to return/work area (atleast 20 000 bytes)
;;
;;
;; on return
;;
;; eax = 0 command succesful
;;       1 no fd base and/or partition defined
;;       2 yet unsupported FS
;;       3 unknown FS
;;       4 partition not defined at hd
;;       5 file not found
;; ebx = size of file
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   esi,[edi]
;     add   eax,[edi]
;     pusha
;
;     mov  edi,esi
;     add  edi,1024
;     mov  esi,fat_base+19*512
;     sub  ecx,1
;     shl  ecx,9
;     add  esi,ecx
;     shl  edx,9
;     mov  ecx,edx
;     cld
;     rep  movsb
;
;     popa
;     mov   [esp+36],eax
;     mov   [esp+24],ebx
;     ret
;
;
;
;
;align 4
;
;sys_programirq:
;
;    mov   edi,[0x3010]
;    add   edi,0x10
;    add   eax,[edi]
;
;    mov   edx,ebx
;    shl   edx,2
;    add   edx,irq_owner
;    mov   edx,[edx]
;    mov   edi,[0x3010]
;    mov   edi,[edi+0x4]
;    cmp   edx,edi
;    je    spril1
;    mov   [esp+36],dword 1
;    ret
;  spril1:
;
;    mov   esi,eax
;    shl   ebx,6
;    add   ebx,irq00read
;    mov   edi,ebx
;    mov   ecx,16
;    cld
;    rep   movsd
;    mov   [esp+36],dword 0
;    ret
;
;
;align 4
;
;get_irq_data:
;
;     mov   edx,eax           ; check for correct owner
;     shl   edx,2
;     add   edx,irq_owner
;     mov   edx,[edx]
;     mov   edi,[0x3010]
;     mov   edi,[edi+0x4]
;     cmp   edx,edi
;     je    gidril1
;     mov   [esp+36],eax
;     mov   [esp+32],dword 2
;     mov   [esp+24],ebx
;     ret
;
;  gidril1:
;
;     mov   ebx,eax
;     shl   ebx,12
;     add   ebx,0x2e0000
;     mov   eax,[ebx]
;     mov   ecx,1
;     test  eax,eax
;     jz    gid1
;
;     dec   eax
;     mov   esi,ebx
;     mov   [ebx],eax
;     movzx ebx,byte [ebx+0x10]
;     add   esi,0x10
;     mov   edi,esi
;     inc   esi
;     mov   ecx,4000 / 4
;     cld
;     rep   movsd
;     xor   ecx,ecx
;   gid1:
;     mov   [esp+36],eax
;     mov   [esp+32],ecx
;     mov   [esp+24],ebx
;     ret
;
;
;set_io_access_rights:
;
;     pusha
;
;     mov   edi,[0x3000]
;     imul  edi,tss_step
;     add   edi,tss_data
;     add   edi,128
;
;     mov   ecx,eax
;     and   ecx,7
;
;     shr   eax,3
;     add   edi,eax
;
;     mov   ebx,1
;
;     shl   ebx,cl
;
;     cmp   ebp,0                ; enable access - ebp = 0
;     jne   siar1
;
;     not   ebx
;     and   [edi],byte bl
;
;     popa
;
;     ret
;
;   siar1:
;
;     or    [edi],byte bl        ; disable access - ebp = 1
;
;     popa
;
;     ret
;
;
;
;
;
;r_f_port_area:
;
;     cmp   eax,0
;     je    r_port_area
;     jmp   free_port_area
;
;   r_port_area:
;
;     pusha
;
;     cmp   ebx,ecx            ; beginning > end ?
;     jg    rpal1
;     mov   esi,[0x2d0000]
;     cmp   esi,0              ; no reserved areas ?
;     je    rpal2
;     cmp   esi,255            ; max reserved
;     jge   rpal1
;   rpal3:
;     mov   edi,esi
;     shl   edi,4
;     add   edi,0x2d0000
;     cmp   ebx,[edi+8]
;     jg    rpal4
;     cmp   ecx,[edi+4]
;     jb    rpal4
;     jmp   rpal1
;   rpal4:
;
;     dec   esi
;     jnz   rpal3
;     jmp   rpal2
;   rpal1:
;     popa
;     mov   eax,1
;     ret
;
;   rpal2:
;     popa
;
;
;     ; enable port access at port IO map
;
;     pusha                        ; start enable io map
;
;     cmp   ecx,65536
;     jge   no_unmask_io
;
;     mov   eax,ebx
;
;   new_port_access:
;
;     pusha
;
;     mov   ebp,0                  ; enable - eax = port
;     call  set_io_access_rights
;
;     popa
;
;     inc   eax
;     cmp   eax,ecx
;     jbe   new_port_access
;
;   no_unmask_io:
;
;     popa                         ; end enable io map
;
;     mov   edi,[0x2d0000]
;     add   edi,1
;     mov   [0x2d0000],edi
;     shl   edi,4
;     add   edi,0x2d0000
;     mov   esi,[0x3010]
;     mov   esi,[esi+0x4]
;     mov   [edi],esi
;     mov   [edi+4],ebx
;     mov   [edi+8],ecx
;
;     mov   eax,0
;     ret
;
;
;
;
;free_port_area:
;
;     pusha
;
;     mov   esi,[0x2d0000]     ; no reserved areas ?
;     cmp   esi,0
;     je    frpal2
;     mov   edx,[0x3010]
;     mov   edx,[edx+4]
;   frpal3:
;     mov   edi,esi
;     shl   edi,4
;     add   edi,0x2d0000
;     cmp   edx,[edi]
;     jne   frpal4
;     cmp   ebx,[edi+4]
;     jne   frpal4
;     cmp   ecx,[edi+8]
;     jne   frpal4
;     jmp   frpal1
;   frpal4:
;     dec   esi
;     jnz   frpal3
;   frpal2:
;     popa
;     mov   eax,1
;     ret
;   frpal1:
;     mov   ecx,256
;     sub   ecx,esi
;     shl   ecx,4
;     mov   esi,edi
;     add   esi,16
;     cld
;     rep   movsb
;
;     dec   dword [0x2d0000]
;
;     popa
;
;
;     ; disable port access at port IO map
;
;     pusha                        ; start disable io map
;
;     cmp   ecx,65536
;     jge   no_mask_io
;
;     mov   eax,ebx
;
;   new_port_access_disable:
;
;     pusha
;
;     mov   ebp,1                  ; disable - eax = port
;     call  set_io_access_rights
;
;     popa
;
;     inc   eax
;     cmp   eax,ecx
;     jbe   new_port_access_disable
;
;   no_mask_io:
;
;     popa                         ; end disable io map
;
;     mov   eax,0
;     ret
;
;
;reserve_free_irq:
;
;     cmp   eax,0
;     jz    reserve_irq
;
;     mov   edi,ebx
;     shl   edi,2
;     add   edi,irq_owner
;     mov   edx,[edi]
;     mov   eax,[0x3010]
;     mov   eax,[eax+0x4]
;     mov   ecx,1
;     cmp   edx,eax
;     jne   fril1
;     mov   [edi],dword 0
;     mov   ecx,0
;   fril1:
;     mov   [esp+36],ecx ; return in eax
;     ret
;
;  reserve_irq:
;
;     mov   edi,ebx
;     shl   edi,2
;     add   edi,irq_owner
;     mov   edx,[edi]
;     mov   ecx,1
;     cmp   edx,0
;     jne   ril1
;
;     mov   edx,[0x3010]
;     mov   edx,[edx+0x4]
;     mov   [edi],edx
;     mov   ecx,0
;
;   ril1:
;
;     mov   [esp+36],ecx ; return in eax
;
;     ret
;
drawbackground:

        cmp     [0xfe0c],word 0x12
        jne     dbrv12
        cmp     [0x400000-12],dword 1
        jne     bgrstr12
        call    vga_drawbackground_tiled
        ret
     bgrstr12:
        call    vga_drawbackground_stretch
        ret
     dbrv12:

        cmp     [0xfe0c],word 0100000000000000b
        jge     dbrv20
        cmp     [0xfe0c],word 0x13
        je      dbrv20
        call    vesa12_drawbackground
        ret
     dbrv20:
        cmp     [0x400000-12],dword 1
        jne     bgrstr
        call    vesa20_drawbackground_tiled
        ret
     bgrstr:
        call    vesa20_drawbackground_stretch
        ret


sys_putimage:

        cmp   [0xfe0c],word 0x12
        jne   spiv20
        call  vga_putimage
        ret
   spiv20:

        cmp   [0xfe0c],word 0100000000000000b
        jge   piv20
        cmp   [0xfe0c],word 0x13
        je    piv20
        call  vesa12_putimage
        ret
   piv20:
        call  vesa20_putimage
        ret



; rax x beginning
; rbx y beginning
; rcx x end
; rdx y end
; rdi color

drawbar:

        cmp     [0xfe0c],word 0x12
        jne     sdbv20
        call    vga_drawbar
        ret
   sdbv20:

        cmp     [0xfe0c],word 0100000000000000b
        jge     dbv20
        cmp     [0xfe0c],word 0x13
        je      dbv20
        call    vesa12_drawbar
        ret

  dbv20:

        call    vesa20_drawbar
        ret



smm:  ; system manegement mode

        cli
        mov   ax,0x0003
        int   0x10
        jmp   $

;rerouteirqs:
;
;        cli
;
;        jmp     $
;        mov     al,0x11         ;  icw4, edge triggered
;        out     0x20,al
;        call    pic_delay
;        out     0xA0,al
;        call    pic_delay
;
;        mov     al,0x20         ;  generate 0x20 +
;        out     0x21,al
;        call    pic_delay
;        mov     al,0x28         ;  generate 0x28 +
;        out     0xA1,al
;        call    pic_delay
;
;        mov     al,0x04         ;  slave at irq2
;        out     0x21,al
;        call    pic_delay
;        mov     al,0x02         ;  at irq9
;        out     0xA1,al
;        call    pic_delay
;
;        mov     al,0x01         ;  8086 mode
;        out     0x21,al
;        call    pic_delay
;        out     0xA1,al
;        call    pic_delay
;
;        mov     al,255          ; mask all irq's
;        out     0xA1,al
;        call    pic_delay
;        out     0x21,al
;        call    pic_delay
;
;        mov     rcx,0x1000
;        cld
;picl1:  call    pic_delay
;        loop    picl1
;
;        mov     al,255          ; mask all irq's
;        out     0xA1,al
;        call    pic_delay
;        out     0x21,al
;        call    pic_delay
;
;        cli
;
;        ret
;
;
;pic_delay:
;
;        jmp     pdl1
;pdl1:   ret


sys_msg_board_str:

        pushdu
   sysmsgb2:
        cmp    [rsi],byte 0
        je     sysmsgb1
        mov    rax,1
        movzx  rbx,byte [rsi]
        call   sys_msg_board
        inc    rsi
        jmp    sysmsgb2
   sysmsgb1:
        popdu
        ret

msg_board_data: times 512 db 0
msg_board_count dq 0x0

sys_msg_board:

; rax=1 : write :  bl byte to write
; rbx=2 :  read :  rbx=0 -> no data, rbx=1 -> data in al

        mov     rcx,[msg_board_count]
        cmp     rax,1
        jne     smbl1
        mov     [msg_board_data+ecx],bl
        inc     rcx
        and     rcx,511
        mov     [msg_board_count],rcx
        mov     [check_idle_semaphore], 5
        ret
   smbl1:

        cmp     rax,2
        jne     smbl2
        test    rcx,rcx
        jz      smbl21
        mov     rdi,msg_board_data
        mov     rsi,msg_board_data+1
        movzx   rax,byte [rdi]
        push    rcx
        shr     rcx,2
        cld
        rep     movsq
        pop     rcx
        and     rcx,3
        rep     movsb
        dec     [msg_board_count]
        mov     [rsp+72],rax
        mov     r8,1
        mov     [rsp+48],r8
        ret
   smbl21:
        mov     [rsp+72],rcx
        mov     [rsp+48],rcx
   smbl2:
        ret


;
;sys_trace:
;
;     cmp  eax,0                     ; get event data
;     jne  no_get_sys_events
;
;     mov  esi,save_syscall_data     ; data
;     mov  edi,[0x3010]
;     mov  edi,[edi+0x10]
;     add  edi,ebx
;     cld
;     rep  movsb
;
;     mov  [esp+24],dword 0
;     mov  eax,[save_syscall_count]  ; count
;     mov  [esp+36],eax
;     ret
;
;   no_get_sys_events:
;
;     ret
;
;
;sys_process_def:
;
;     cmp   eax,1                   ; set keyboard mode
;     jne   no_set_keyboard_setup
;
;     mov   edi,[0x3000]
;     imul  edi,256
;     add   edi,0x80000+0xB4
;     mov   [edi],bl
;
;     ret
;
;   no_set_keyboard_setup:
;
;     cmp   eax,2                   ; get keyboard mode
;     jne   no_get_keyboard_setup
;
;     mov   edi,[0x3000]
;     imul  edi,256
;     add   edi,0x80000+0xB4
;     mov   eax,[edi]
;     and   eax,0xff
;
;     mov   [esp+36],eax
;
;     ret
;
;   no_get_keyboard_setup:
;
;     cmp   eax,3                   ; get keyboard ctrl, alt, shift
;     jne   no_get_keyboard_cas
;
;     xor   eax,eax
;     movzx eax,byte [shift]
;     movzx ebx,byte [ctrl]
;     shl   ebx,2
;     add   eax,ebx
;     movzx ebx,byte [alt]
;     shl   ebx,3
;     add   eax,ebx
;
;     mov   [esp+36],eax
;
;     ret
;
;   no_get_keyboard_cas:
;
;
;
;     ret
;
;
;sys_ipc:
;
;     cmp  eax,1                      ; DEFINE IPC MEMORY
;     jne  no_ipc_def
;     mov  edi,[0x3000]
;     shl  edi,8
;     add  edi,0x80000
;     mov  [edi+0xA0],ebx
;     mov  [edi+0xA4],ecx
;     mov  [esp+36],dword 0
;     ret
;   no_ipc_def:
;
;     cmp  eax,2                      ; SEND IPC MESSAGE
;     jne  no_ipc_send
;     mov  esi,1
;     mov  edi,0x3020
;    ipcs1:
;     cmp  [edi+4],ebx
;     je   ipcs2
;     add  edi,0x20
;     inc  esi
;     cmp  esi,[0x3004]
;     jbe  ipcs1
;     mov  [esp+36],dword 4
;     ret
;    ipcs2:
;
;     cli
;
;     push esi
;     mov  eax,esi
;     shl  eax,8
;     mov  ebx,[eax+0x80000+0xa0]
;     cmp  ebx,0                    ; ipc area not defined ?
;     je   ipc_err1
;
;     add  ebx,[eax+0x80000+0xa4]
;     mov  eax,esi
;     shl  eax,5
;     add  ebx,[eax+0x3000+0x10]    ; ebx <- max data position
;
;     mov  eax,esi                  ; to
;     shl  esi,8
;     add  esi,0x80000
;     mov  edi,[esi+0xa0]
;     shl  eax,5
;     add  eax,0x3000
;     add  edi,[eax+0x10]
;
;     cmp  [edi],byte 0             ; overrun ?
;     jne  ipc_err2
;
;     mov  ebp,edi
;     add  edi,[edi+4]
;     add  edi,8
;
;     mov  esi,ecx                  ; from
;     mov  eax,[0x3010]
;     mov  eax,[eax+0x10]
;     add  esi,eax
;
;     mov  ecx,edx                  ; size
;
;     mov  eax,edi
;     add  eax,ecx
;     cmp  eax,ebx
;     jge  ipc_err3                 ; not enough room ?
;
;     push ecx
;
;     mov  eax,[0x3010]
;     mov  eax,[eax+4]
;     mov  [edi-8],eax
;     mov  [edi-4],ecx
;     cld
;     rep  movsb
;
;     pop  ecx
;     add  ecx,8
;
;     mov  edi,ebp                  ; increase memory position
;     add  dword [edi+4],ecx
;
;     mov  edi,[esp]
;     shl  edi,8
;     or   dword [edi+0x80000+0xA8],dword 01000000b ; ipc message
;
;     cmp  [check_idle_semaphore],dword 20
;     jge  ipc_no_cis
;     mov  [check_idle_semaphore],5
;   ipc_no_cis:
;
;     mov  eax,0
;
;    ipc_err:
;     add  esp,4
;     mov  [esp+36],eax
;     sti
;     ret
;
;    ipc_err1:
;     add  esp,4
;     mov  [esp+36],dword 1
;     sti
;     ret
;    ipc_err2:
;     add  esp,4
;     mov  [esp+36],dword 2
;     sti
;     ret
;    ipc_err3:
;     add  esp,4
;     mov  [esp+36],dword 3
;     sti
;     ret
;
;   no_ipc_send:
;
;     mov  [esp+36],dword -1
;     ret
;
;
;align 4
;
;sys_gs:                         ; direct screen access
;
;     cmp  eax,1                 ; resolution
;     jne  no_gs1
;     mov  eax,[0xfe00]
;     shl  eax,16
;     mov  ax,[0xfe04]
;     add  eax,0x00010001
;     mov  [esp+36],eax
;     ret
;   no_gs1:
;
;     cmp   eax,2                ; bits per pixel
;     jne   no_gs2
;     movzx eax,byte [0xfbf1]
;     mov   [esp+36],eax
;     ret
;   no_gs2:
;
;     cmp   eax,3                ; bytes per scanline
;     jne   no_gs3
;     mov   eax,[0xfe08]
;     mov   [esp+36],eax
;     ret
;   no_gs3:
;
;     mov  [esp+36],dword -1
;     ret
;
;
;align 4 ; PCI functions
;
;sys_pci:
;
;     call  pci_api
;     mov   [esp+36],eax
;     ret
;
;
;align 4  ;  system functions
;
;syscall_setpixel:                       ; SetPixel
;
;
;     mov   edx,[0x3010]
;     add   eax,[edx-twdw]
;     add   ebx,[edx-twdw+4]
;     xor   edi,edi ; no force
;     call  disable_mouse
;     jmp   putpixel
;
;align 4
;
;syscall_writetext:                      ; WriteText
;
;     push  ebx
;     cmp   eax , 30*65536
;     ja    nolabelcorrection
;     cmp   ax , 19
;     ja    nolabelcorrection
;     and   eax , 0xffff
;     add   eax , 20 * 65536
;     cmp   edx , -1
;     je    nolabelcorrection
;     push  edx
;     push  edi
;     and   ebx,0xff000000
;     cmp   ebx,0x01000000
;     je    mul4
;     imul  edx,3
;     jmp   mul3
;   mul4:
;     imul  edx,4
;   mul3:
;     mov   edi,[0x3010]
;     mov   ebx,[edi-twdw+8]
;     shr   ebx,1
;     sub   ebx,edx
;     and   ebx,0xffff
;     imul  ebx,65536
;     and   eax,0xffff
;     add   eax,ebx
;     pop   edi
;     pop   edx
;     or    dword [esp],0xffffff ; color to white
;   nolabelcorrection:
;     pop   ebx
;
;     mov   edi,[0x3010]
;     mov   ebp,[edi-twdw]
;     shl   ebp,16
;     add   ebp,[edi-twdw+4]
;     add   edi,0x10
;     add   ecx,[edi]
;     cmp   edx,-1
;     jne   nozeroterm
;     call  return_string_length
;   nozeroterm:
;     add   eax,ebp
;     xor   edi,edi
;     jmp   dtext
;
;align 4
;
;syscall_openramdiskfile:                ; OpenRamdiskFile
;
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   eax,[edi]
;     add   edx,[edi]
;     mov   esi,12
;     call  fileread
;     mov   [esp+36],ebx
;     ret
;
;align 4
;
;syscall_putimage:                       ; PutImage
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   eax,[edi]
;     mov   edx,ecx
;     mov   ecx,ebx
;     mov   ebx,eax
;     call  sys_putimage
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_drawrect:                       ; DrawRect
;
;     push  ebx
;     mov   ebx,[0x3000]
;     imul  ebx,256
;     add   ebx,0x80000
;     cmp   [ebx],dword 'FASM'
;     jne   nocbgr2
;     mov   ebx , ecx
;     and   ebx , 0xffffff
;     cmp   ebx , 0x2030a0
;     jne   nocbgr2
;     mov   cl , 0x80
;   nocbgr2:
;     pop   ebx
;
;     mov   edi,ecx
;     test  ax,ax
;     je    drectr
;     test  bx,bx
;     je    drectr
;     movzx ecx,ax
;     shr   eax,16
;     movzx edx,bx
;     shr   ebx,16
;     add   ecx,eax
;     add   edx,ebx
;     jmp   drawbar
;    drectr:
;     ret
;
;align 4
;
;syscall_getscreensize:                  ; GetScreenSize
;
;     movzx eax,word[0xfe00]
;     shl   eax,16
;     mov   ax,[0xfe04]
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_system:                         ; System
;
;     call  sys_system
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_startapp:                       ; StartApp
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   eax,[edi]
;     test  ebx,ebx
;     jz    noapppar
;     add   ebx,[edi]
;   noapppar:
;     call  start_application_fl
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_cdaudio:                        ; CD
;
;     call  sys_cd_audio
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_readhd:                         ; ReadHd
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   esi,[edi]
;     add   eax,[edi]
;     call  read_hd_file
;     mov   [esp+36],eax
;     mov   [esp+24],ebx
;     ret
;
;align 4
;
;syscall_starthdapp:                     ; StartHdApp
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   eax,[edi]
;     add   ecx,[edi]
;     mov   ebp,0
;     call  start_application_hd
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_delramdiskfile:                 ; DelRamdiskFile
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   eax,[edi]
;     call  filedelete
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_writeramdiskfile:               ; WriteRamdiskFile
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   eax,[edi]
;     add   ebx,[edi]
;     call  filesave
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_getpixel:                       ; GetPixel
;
;     mov   ecx,[0xfe00]
;     inc   ecx
;     xor   edx,edx
;     div   ecx
;     mov   ebx,edx
;     xchg  eax,ebx
;     call  dword [0xe024]
;     mov   [esp+36],ecx
;     ret
;
;align 4
;
;syscall_readstring:                     ; ReadString
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   eax,[edi]
;     call  read_string
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_drawline:                       ; DrawLine
;
;     mov   edi,[0x3010]
;     movzx edx,word[edi-twdw]
;     mov   ebp,edx
;     shl   edx,16
;     add   ebp,edx
;     movzx edx,word[edi-twdw+4]
;     add   eax,ebp
;     mov   ebp,edx
;     shl   edx,16
;     xor   edi,edi
;     add   edx,ebp
;     add   ebx,edx
;     jmp   draw_line
;
;align 4
;
;syscall_getirqowner:                    ; GetIrqOwner
;
;     shl   eax,2
;     add   eax,irq_owner
;     mov   eax,[eax]
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_reserveportarea:                ; ReservePortArea and FreePortArea
;
;     call  r_f_port_area
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;syscall_appints:                        ; AppInts
;
;     test  eax,eax
;     jnz   unknown_app_int_fn
;     mov   edi,[0x3010]
;     mov   [edi+draw_data-0x3000+0x1c],ebx
;     ret
;   unknown_app_int_fn:
;     mov   [esp+36],dword -1
;     ret
;
;align 4
;
;syscall_threads:                        ; CreateThreads
;
;     call  sys_threads
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;stack_driver_stat:
;
;     call  app_stack_handler            ; Stack status
;
;;     mov   [check_idle_semaphore],5    ; enable these for zero delay
;;     call  change_task                 ; between sent packet
;
;     mov   [esp+36],eax
;     ret
;
;align 4
;
;socket:                                 ; Socket interface
;     call  app_socket_handler
;
;;     mov   [check_idle_semaphore],5    ; enable these for zero delay
;;     call  change_task                 ; between sent packet
;
;     mov   [esp+36],eax
;     mov   [esp+24],ebx
;     ret
;
;align 4
;
;user_events:                            ; User event times
;
;     mov   eax,0x12345678
;     mov   [esp+36],eax
;
;     ret
;
;align 4
;
;read_from_hd:                           ; Read from hd - fn not in use
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   eax,[edi]
;     add   ecx,[edi]
;     add   edx,[edi]
;     call  file_read
;
;     mov   [esp+36],eax
;     mov   [esp+24],ebx
;
;     ret
;
;
;align 4
;
;write_to_hd:                            ; Write a file to hd
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   eax,[edi]
;     add   ecx,[edi]
;     add   edx,[edi]
;     call  file_write
;     ret
;
;align 4
;
;delete_from_hd:                         ; Delete a file from hd
;
;     mov   edi,[0x3010]
;     add   edi,0x10
;     add   eax,[edi]
;     add   ecx,[edi]
;     call  file_delete
;     ret
;
;
;align 4
;
;undefined_syscall:                      ; Undefined system call
;
;     mov   [esp+36],dword -1
;     ret
;
;
;clear_busy_flag_at_caller:
;
;      push  edi
;
;      mov   edi,[0x3000]    ; restore processes tss pointer in gdt, busyfl?
;      imul  edi,8
;      mov   [edi+gdts+ tss0 +5], word 01010000b *256 +11101001b
;
;      pop   edi
;
;      ret

keymap:

     db   '6',27
     db   '1234567890-=',8,9
     db   'qwertyuiop[]',13
     db   '~asdfghjkl;',39,96,0,'\zxcvbnm,./',0,'45 '
     db   '@234567890123',180,178,184,'6',176,'7'
     db   179,'8',181,177,183,185,182
     db   'AB<D',255,'FGHIJKLMNOPQRSTUVWXYZ'
     db   'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
     db   'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
     db   'ABCDEFGHIJKLMNOPQRSTUVWXYZ'


keymap_shift:

     db   '6',27
     db   '!@#$%^&*()_+',8,9
     db   'QWERTYUIOP{}',13
     db   '~ASDFGHJKL:"~',0,'|ZXCVBNM<>?',0,'45 '
     db   '@234567890123',180,178,184,'6',176,'7'
     db   179,'8',181,177,183,185,182
     db   'AB>D',255,'FGHIJKLMNOPQRSTUVWXYZ'
     db   'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
     db   'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
     db   'ABCDEFGHIJKLMNOPQRSTUVWXYZ'


keymap_alt:

     db   ' ',27
     db   ' @ $  {[]}\ ',8,9
     db   '            ',13
     db   '             ',0,'           ',0,'4',0,' '
     db   '             ',180,178,184,'6',176,'7'
     db   179,'8',181,177,183,185,182
     db   'ABCD',255,'FGHIJKLMNOPQRSTUVWXYZ'
     db   'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
     db   'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
     db   'ABCDEFGHIJKLMNOPQRSTUVWXYZ'


; device irq owners

irq_owner:       ; process id

     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0
     dq   0x0


; on irq read ports

irq00read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq01read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq02read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq03read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq04read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq05read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq06read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq07read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq08read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq09read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq10read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq11read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq12read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq13read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq14read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
irq15read  dq  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0


; status

hd1_status                  dq 0x0  ; 0 - free : other - pid
application_table_status    dq 0x0  ; 0 - free : other - pid

; device addresses

mididp     dd 0x0
midisp     dd 0x0

cdbase     dd 0x0
cdid       dd 0x0

hdbase              dd   0x0  ; for boot 0x1f0
hdid                dd   0x0
hdpos               dd   0x0  ; for boot 0x1
fat32part           dd   0x0  ; for boot 0x1
lba_read_enabled    dd   0x0  ; 0 = disabled , 1 = enabled
pci_access_enabled  dd   0x0  ; 0 = disabled , 1 = enabled

keyboard   dd 0x1

sb16       dd 0x0
wss        dd 0x0
sound_dma  dd 0x1

syslang    dd 0x1

buttontype         dd 0x0
windowtypechanged  dd 0x0

endofcode:  ; -MK






