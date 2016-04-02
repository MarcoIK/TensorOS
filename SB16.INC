;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                 ;;
;;  Sb16.inc                                                       ;;
;;                                                                 ;;
;;  Sound Blaster 16 functions for Tensor                          ;;
;;  Copyright 2012 Marco K, oldklickers@gmail.com                  ;;
;;  See file COPYING for details                                   ;;
;;  - 11.07.2012 8 bit stereo mode - MK                            ;;
;;                                                                 ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SB16_load_music  equ   0xc0000000
SB16_play_music  equ   0xc0000001
DMAPage          equ   0x2A
Rate             equ   44100
SB16Buffer       equ   0x2A0000
SB16_Status      equ   SB16Buffer+65536

sound_data_format:

    dd    0x1

sound_data_length:

    dd    65536

sound_data_freq:

    dd    44100

sound_interface:

        cmp     rax,0                     ; Load data
        jne     no_SB16_load_music
        mov     rdi,[0x6020]
        add     rdi,0x20
        add     rbx,[rdi]
        call    code_SB16_load_music
        ret
  no_SB16_load_music:

        cmp     rax,1                     ; Play data
        jne     no_SB16_play_music
        call    code_SB16_play_music
        ret
  no_SB16_play_music:

        cmp     rax,2                     ; Set data formats
        jne     no_SB16_data_format
        cmp     rbx,0                     ; ebx=0 play format
        jne     no_sound_format
        mov     [sound_data_format],ecx     ; 1=8b mono, 2=8b stereo
        ret
  no_sound_format:
        cmp     rbx,1                     ; ebx=1 data length
        jne     no_sound_length
        mov     [sound_data_length],ecx     ;
        ret
  no_sound_length:
        cmp     rbx,2                     ; ebx=2 sound data frequency
        jne     no_sound_freq
        mov     [sound_data_freq],ecx
        ret
  no_sound_freq:
        ret

 no_SB16_data_format:

        ret

code_SB16_play_music:

        cmp     [sound_data_format],dword 1
        jne     no_sound_8bm
        call    sb_play_8b_mono
        ret
  no_sound_8bm:

        cmp     [sound_data_format],dword 2
        jne     no_sound_8bs
        call    sb_play_8b_stereo
        ret
  no_sound_8bs:

        ret

Blaster_command:

        push    rax
        push    rcx
        push    rdx

        mov     dx,word [sb16]
        add     dx,0xc
        mov     cx,1000
  bcl1:
        in      al,dx
        and     al,128
        jz      bcl2
        loop    bcl1
 bcl2:
        mov     al,[rsp+16]
        mov     dx,[rsp+0]
        add     dx,word [sb16]
        out     dx,al

        pop     rdx
        pop     rcx
        pop     rax

        ret

sb_play_8b_stereo:

        pushdu

        call    sb_set_dma

        call    sb_set_stereo

        mov     dx,0xc
        mov     al,0xa8
        call    Blaster_command

        mov     al,0x40
        call    Blaster_command

        mov     al,245
        call    Blaster_command

        mov     al,0x48
        call    Blaster_command

        mov     al,0xff
        call    Blaster_command
        call    Blaster_command

        mov     al,0x91
        call    Blaster_command

        popdu
        ret

sb_set_stereo:

        push    rax
        push    rdx

        call    sb_wait

        mov     dx,word [sb16]
        add     dx,0x4
        mov     al,0xe
        out     dx,al
        inc     dx
        in      al,dx
        and     al,253
        or      al,2     ; stereo
        out     dx,al

        pop     rdx
        pop     rax

        ret

code_SB16_load_music:

        cmp     byte [SB16_Status],1
        je      nol
        mov     edi,SB16Buffer
        mov     rsi,rbx
        mov     rcx,65536/4
        cld
        rep     movsd
  nol:
        ret

dma_table:

        db    0x87,0x83,0x81,0x82


sb_set_dma:

        pushdu

        mov     eax,[sound_dma]
        add     rax,4
        out     0xa,al

        mov     al,0
        out     0xc,al

        mov     eax,[sound_dma]
        add     rax,0x48
        out     0xb,al

        mov     edx,[sound_dma]
        shl     rdx,1
        mov     al,0
        out     dx,al

        mov     al,0
        out     dx,al

        mov     edx,[sound_dma]
        add     rdx,dma_table
        movzx   rdx,byte [rdx]
        mov     al,DMAPage
        out     dx,al

        mov     edx,[sound_dma]
        shl     rdx,1
        inc     rdx
        mov     rax,[sound_data_length]
        dec     rax
        and     rax,0xff
        ; mov al,(DataLength-1) and 0xff
        out     dx,al

        mov     rax,[sound_data_length]
        dec     rax
        shr     rax,8
        ; mov al,(DataLength-1) shr 8
        out     dx,al

        mov     eax,[sound_dma] ; DMA
        out     0xa,al

        popdu
        ret


sb_play_8b_mono:

        call    sb_set_dma

        cmp     byte [SB16_Status],1
        jne     contsb16
        jmp     retserve

  contsb16:

        mov     dx,word [sb16]
        add     dx,4
        mov     ecx,[sound_dma]
        mov     ax,0x01
        shl     ax,cl
        shl     ax,8
        add     ax,0x81
        out     dx,ax

        mov     ax,0f280h   ;enable irq5
        out     dx,ax

  adr1_SB:

        mov     dx,word [sb16]
        add     dx,0ch
        in      al,dx
        and     al,080h
        jnz     adr1_SB

        call    sb_set_stereo

        mov     al,0d1h
        out     dx,al

        mov     dx,word [sb16]
        add     dx,0ch

        call    sb_wait

        mov     al,40h                              ; Rate
        out     dx,al
        call    sb_wait
        mov     al,256-1000000/Rate
        out     dx,al

        call    sb_wait

        mov     al,14h                              ; Datalength
        out     dx,al
        call    sb_wait

        mov     rax,[sound_data_length]
        dec     rax
        and     rax,0xff
        ;mov   al,(DataLength-1) and 0xff
        out     dx,al
        call    sb_wait
        mov     rax,[sound_data_length]
        dec     rax
        shr     rax,8
        ;mov   al,(DataLength-1) shr 8
        out     dx,al

  retserve:

        ret


sb_wait:

        in      al,dx       ; Wait
        and     al,080h
        jnz     sb_wait

        ret


;****************************************
; END CODE SB16 by Marco K
;***************************************


