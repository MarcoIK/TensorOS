;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                        ;;
;;  VESA12.INC                                            ;;
;;                                                        ;;
;;  Vesa 1.2 functions for Tensor                         ;;
;;                                                        ;;
;;  Copyright 2012 Marco K                                ;;
;;                                                        ;;
;;  oldklickers@gmail.com - bankswitch for S3 cards       ;;
;;                                                        ;;
;;  See file COPYING for details                          ;;
;;                                                        ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


; A complete video driver should include the following types of function
;
; Putpixel
; Getpixel
;
; Drawimage
; Drawbar
;
; Drawbackground
;
;
; Modifying the set_bank -function is mostly enough
; for different Vesa 1.2 setups.


set_bank:

        cli
        cmp     al,[0xfff2]
        je      retsb
        mov     [0xfff2],al
        push    ax
        push    dx
        mov     ah,al
        mov     dx,0x03D4
        mov     al,0x39
        out     dx,al
        inc     dl
        mov     al,0xA5
        out     dx,al
        dec     dl
        mov     al,6Ah
        out     dx,al
        inc     dl
        mov     al,ah
        out     dx,al
        dec     dl
        mov     al,0x39
        out     dx,al
        inc     dl
        mov     al,0x5A
        out     dx,al
        dec     dl
        pop     dx
        pop     ax

 retsb:

   ret


vesa12_drawbackground:

        call    disable_mouse

        push    rax
        push    rbx
        push    rcx
        push    rdx

        xor     rdx,rdx
        mov     rax,qword[0x400000-8]
        mov     rbx,qword[0x400000-4]
        mul     rbx
        mov     rbx,3
        mul     rbx
        mov     [imax],eax
        mov     rax,[draw_data+32+0]
        mov     rbx,[draw_data+32+4]
        mov     rdi,0 ;no force

      v12dp3:

        push    rax
        push    rbx
        mov     rsi,0x300000

        cmp     [0x400000-12],dword 1     ; tiled background
        jne     no_vesa12_tiled_bgr

        push    rdx

        xor     rdx,rdx
        mov     rcx,[0x400000-8]
        div     rcx
        mov     rax,rdx

        push    rax
        mov     rax,rbx
        xor     rdx,rdx
        mov     rcx,[0x400000-4]
        div     rcx
        mov     rbx,rdx
        pop     rax

        pop     rdx

      no_vesa12_tiled_bgr:

        cmp     [0x400000-12],dword 2     ; stretched background
        jne     no_vesa12_stretched_bgr

        push    rdx

        imul    rax,qword [0x400000-8]
        xor     rdx,rdx
        mov     rcx,[0xfe00]
        inc     rcx
        div     rcx

        push    rax
        mov     rax,rbx
        imul    rax,qword [0x400000-4]
        xor     rdx,rdx
        mov     rcx,[0xfe04]
        inc     rcx
        div     rcx
        mov     rbx,rax
        pop     rax

        pop     rdx

      no_vesa12_stretched_bgr:


        push    rax
        mov     rax,rbx
        xor     rdx,rdx
        mov     rbx,[0x400000-8]
        add     rbx,[0x400000-8]
        add     rbx,[0x400000-8]
        mul     rbx
        mov     rsi,rax
        pop     rax
        add     rsi,rax
        add     rsi,rax
        add     rsi,rax
        add     rsi,0x300000
        pop     rbx
        pop     rax

      v12di4:

        mov     rcx,[rsi]
        pushdu
        mov     rsi,rax
        mov     rdi,rbx
        mov     rax,[0xfe00]
        add     rax,1
        mul     rbx
        add     rax,rsi
        add     rax,0x400000
        cmp     [rax],byte 1
        jnz     v12nbgp
        mov     rax,[0xfe08]
        mov     rbx,rdi
        mul     rbx
        add     rax,rsi
        add     rax,rsi
        add     rax,rsi
        cmp     [0xFBF1],byte 24
        jz      v12bgl3
        add     rax,rsi

      v12bgl3:

        push    rbx
        push    rax

;;        sub  rax,[0xfe80]

        shr     rax,16
        call    set_bank
        pop     rax
        and     rax,65535
        add     rax,0xa0000
        pop     rbx

        mov     [rax],cx
        add     rax,2
        shr     rcx,16
        mov     [rax],cl
        sti

      v12nbgp:

        popdu
        add     rsi,3
        inc     rax
        cmp     rax,[draw_data+32+8]
        jg      v12nodp31
        jmp     v12dp3

      v12nodp31:

        mov     rax,[draw_data+32+0]
        inc     rbx
        cmp     rbx,[draw_data+32+12]
        jg      v12dp4
        jmp     v12dp3

      v12dp4:

        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax
        ret


vesa12_drawbar:

        call    disable_mouse

        mov     r8,0
        mov     [novesachecksum],r8
        sub     edx,ebx
        sub     ecx,eax
        push    rsi
        push    rdi
        push    rax
        push    rbx
        push    rcx
        push    rdx
        mov     rcx,[0x3010]
        add     rax,[rcx-twdw]
        add     rbx,[rcx-twdw+4]
        push    rax
        mov     rax,rbx         ; y
        mov     rbx,[0xfe08]
        mul     rbx
        pop     rcx
        add     rax,rcx         ; x
        add     rax,rcx
        add     rax,rcx
        cmp     [0xfbf1],byte 24     ; 24 or 32 bpp ? - x start
        jz      dbpi2412
        add     rax,rcx

  dbpi2412:

        add     rax,[0xfe80]
        mov     rdi,rax

        ; x size

        mov     rax,[rsp+8] ; [esp+6]
        mov     rcx,rax
        add     rcx,rax
        add     rcx,rax
        cmp     [0xfbf1],byte 24     ; 24 or 32 bpp ? - x size
        jz      dbpi24312
        add     rcx,rax

  dbpi24312:

        mov     rbx,[rsp+0]

        ; check limits ?

        push    rax
        push    rcx
        mov     rax,[0x3010]
        mov     rcx,[rax+draw_data-0x3000+0]
        cmp     rcx,0
        jnz     dbcblimitlset12
        mov     rcx,[rax+draw_data-0x3000+4]
        cmp     rcx,0
        jnz     dbcblimitlset12
        mov     rcx,[rax+draw_data-0x3000+8]
        cmp     rcx,[0xfe00]
        jnz     dbcblimitlset12
        mov     rcx,[rax+draw_data-0x3000+12]
        cmp     rcx,[0xfe04]
        jnz     dbcblimitlset12
        pop     rcx
        pop     rax
        mov     r8,0
        push    r8
        jmp     dbcblimitlno12

  dbcblimitlset12:

        pop     rcx
        pop     rax
        mov     r8,1
        push    r8

  dbcblimitlno12:

        cmp     [0xfbf1],byte 24     ; 24 or 32 bpp ?
        jz      dbpi24bit12
        jmp     dbpi32bit12


; DRAWBAR 24 BBP


dbpi24bit12:

        push    rax
        push    rbx
        push    rdx
        mov     rax,rcx
        mov     rbx,3
        div     rbx
        mov     rcx,rax
        pop     rdx
        pop     rbx
        pop     rax
        cld

  dbnewpi12:

        push    rbx
        push    rdi
        push    rcx

        xor     rdx,rdx
        mov     rax,rdi
        sub     rax,[0xfe80]
        mov     rbx,3
        div     rbx
        add     rax,0x400000
        mov     rbx,[0x3010]
        movzx   rbx,byte[rbx+0xe]
        cld

      dbnp2412:

        mov     dl,[rax]
        push    rax
        push    rcx
        cmp     dl,bl
        jnz     dbimp24no12
        mov     r8,0
        cmp     [rsp+10*4],r8
        jz      dbimp24yes12
        call    dbcplimit
        jnz     dbimp24no12

     dbimp24yes12:

        push    rdi
        mov     rax,rdi
        sub     rax,[0xfe80]
        shr     rax,16
        call    set_bank
        and     rdi,0xffff
        add     rdi,0xa0000
        mov     rax,[rsp+16+6*4+32+8+8]
        stosw
        shr     rax,16
        stosb
        sti
        pop     rdi
        add     rdi,3
        pop     rcx
        pop     rax
        inc     rax
        loop    dbnp2412
        jmp     dbnp24d12

      dbimp24no12:

        pop     rcx
        pop     rax
        cld
        add     rdi,3
        inc     rax
        loop    dbnp2412

      dbnp24d12:

        mov     rax,[rsp+6*4+32+8]
        mov     r8,0x80000000
        test    rax,r8
        jz      nodbgl2412
        cmp     al,0
        jz      nodbgl2412
        dec     rax
        mov     [rsp+6*4+32+8],rax

      nodbgl2412:

        pop     rcx
        pop     rdi
        pop     rbx
        add     rdi,[0xfe08]
        dec     rbx
        jz      dbnonewpi12
        jmp     dbnewpi12

  dbnonewpi12:

        add     rsp,14*4

        ret


; DRAWBAR 32 BBP


  dbpi32bit12:

        cld
        shr     rcx,2

   dbnewpi3212:

        push    rbx
        push    rdi
        push    rcx

        mov     rax,rdi
        sub     rax,[0xfe80]
        shr     rax,2
        add     rax,0x400000
        mov     rbx,[0x3010]
        movzx   rbx,byte[rbx+0xe]
        cld

      dbnp3212:

        mov     dl,[rax]
        push    rax
        push    rcx
        cmp     dl,bl
        jnz     dbimp32no12
        mov     r8,0
        cmp     [rsp+10*4],r8
        jz      dbimp32yes12
        call    dbcplimit
        jnz     dbimp32no12

      dbimp32yes12:

        push    rdi
        mov     rax,rdi
        sub     rax,[0xfe80]
        shr     rax,16
        call    set_bank
        and     rdi,0xffff
        add     rdi,0xa0000
        mov     rax,[rsp+16+6*4+32+8+8]
        stosw
        shr     rax,16
        stosb
        sti
        pop     rdi
        add     rdi,4
        inc     rbp
        pop     rcx
        pop     rax
        inc     rax
        loop    dbnp3212
        jmp     dbnp32d12

      dbimp32no12:

        pop     rcx
        pop     rax
        inc     rax
        add     rdi,4
        inc     rbp
        loop    dbnp3212

      dbnp32d12:

        mov     rax,[rsp+24+32+8]
        mov     r8,0x80000000
        test    rax,r8
        jz      nodbgl3212
        cmp     al,0
        jz      nodbgl3212
        dec     rax
        mov     [rsp+24+32+8],rax

      nodbgl3212:

        pop     rcx
        pop     rdi
        pop     rbx
        add     rdi,[0xfe08]
        dec     rbx
        jz      nodbnewpi3212
        jmp     dbnewpi3212

  nodbnewpi3212:

        add     rsp,14*4
        ret


Vesa12_putpixel24:

        mov     rdi,rax ; x
        mov     rax,rbx ; y
        lea     rdi,[rdi+rdi*2]
        mov     rbx,[0xfe08]
        mul     rbx
        add     rdi,rax
        mov     rax,rdi
        shr     rax,16
        call    set_bank
        and     rdi,65535
        add     rdi,0xa0000
        mov     rax,[rsp+56]
        stosw
        shr     rax,16
        mov     [rdi],al
        sti
        ret



Vesa12_putpixel32:

        mov     rdi,rax ; x
        mov     rax,rbx ; y
        shl     rdi,2
        mov     rbx,[0xfe08]
        mul     rbx
        add     rdi,rax
        mov     rax,rdi
        shr     rax,16
        call    set_bank
        and     rdi,65535
        add     rdi,0xa0000
        mov     rcx,[rsp+56]
        mov     [rdi],rcx
        sti
        ret


Vesa12_getpixel24:

        mov     rdi,rax ; x
        mov     rax,rbx ; y
        lea     rdi,[rdi+rdi*2]
        mov     rbx,[0xfe08]
        mul     rbx
        add     rdi,rax
        mov     rax,rdi
        shr     rax,16
        call    set_bank
        and     rdi,65535
        add     rdi,0xa0000
        mov     rcx,[rdi]
        and     rcx,255*256*256+255*256+255
        sti
        ret


Vesa12_getpixel32:

        mov     rdi,rax ; x
        mov     rax,rbx ; y
        shl     rdi,2
        mov     rbx,[0xfe08]
        xor     rdx,rdx
        mul     rbx
        add     rdi,rax
        mov     rax,rdi
        shr     rax,16
        call    set_bank
        and     rdi,65535
        add     rdi,0xa0000
        mov     rcx,[rdi]
        and     rcx,255*256*256+255*256+255
        sti

        ret



vesa12_putimage:

;    mov  ebx,image
;    mov  ecx,320*65536+240
;    mov  edx,20*65536+20

        call    disable_mouse

        mov     r8,0
        mov     [novesachecksum],r8
        push    rsi
        push    rdi
        push    rax
        push    rbx
        push    rcx
        push    rdx
        movzx   rax,word [rsp+4]
        movzx   rbx,word [rsp+0]
        mov     rcx,[0x3010]
        add     rax,[rcx-twdw]
        add     rbx,[rcx-twdw+4]
        push    rax
        mov     rax,rbx         ; y
        mov     rbx,[0xfe08]
        mul     rbx
        pop     rcx
        add     rax,rcx         ; x
        add     rax,rcx
        add     rax,rcx
        cmp     [0xfbf1],byte 24     ; 24 or 32 bpp ? - x start
        jz      pi2412
        add     rax,rcx

  pi2412:

        add     rax,[0xfe80]
        mov     rdi,rax

        ; x size

        movzx   rax,word [rsp+12]
        mov     rcx,rax
        add     rcx,rax
        add     rcx,rax
        cmp     [0xfbf1],byte 24     ; 24 or 32 bpp ? - x size
        jz      pi24312
        add     rcx,rax

  pi24312:

        mov     rsi,[rsp+16]
        movzx   rbx,word [rsp+8]

        ; check limits while draw ?

        push    rax
        push    rcx
        mov     rax,[0x3010]
        mov     rcx,[rax+draw_data-0x3000+0]
        cmp     rcx,0
        jnz     dbcblimitlset212
        mov     rcx,[rax+draw_data-0x3000+4]
        cmp     rcx,0
        jnz     dbcblimitlset212
        mov     rcx,[rax+draw_data-0x3000+8]
        cmp     rcx,[0xfe00]
        jnz     dbcblimitlset212
        mov     rcx,[rax+draw_data-0x3000+12]
        cmp     rcx,[0xfe04]
        jnz     dbcblimitlset212
        pop     rcx
        pop     rax
        mov     r8,0
        push    r8
        jmp     dbcblimitlno212

  dbcblimitlset212:

        pop     rcx
        pop     rax
        mov     r8,1
        push    r8

  dbcblimitlno212:

        cmp     [0xfbf1],byte 24     ; 24 or 32 bpp ?
        jz      pi24bit12
        jmp     pi32bit12

  pi24bit12:

        cld
        push    rax
        push    rbx
        push    rdx
        xor     rdx,rdx
        mov     rax,rcx
        mov     rbx,3
        div     rbx
        mov     rcx,rax
        pop     rdx
        pop     rbx
        pop     rax

  newpi12:

        push    rdi
        push    rsi
        push    rcx
        push    rbx

        xor     rdx,rdx
        mov     rax,rdi
        sub     rax,[0xfe80]
        mov     rbx,3
        div     rbx
        add     rax,0x400000
        mov     rbx,[0x3010]
        mov     bl,[rbx+0xe]
        mov     bh,[rsp+8*4]

      np2412:

        cmp     bl,[rax]
        jnz     imp24no12
        mov     rdx,[rsi]
        cmp     bh,0
        jz      imp24yes12
        call    dbcplimit
        jnz     imp24no12

     imp24yes12:

        push    rax
        push    rdi
        mov     rax,rdi
        sub     rax,[0xfe80]
        shr     rax,16
        call    set_bank
        and     rdi,0xffff
        add     rdi,0xa0000
        mov     [rdi],rdx
        shr     rdx,2
        mov     [rdi+2],dl
        sti
        pop     rdi
        pop     rax

     imp24no12:

        inc     rax
        add     rsi,3
        add     rdi,3
        dec     rcx
        jnz     np2412

      np24d12:

        pop     rbx
        pop     rcx
        pop     rsi
        pop     rdi

        add     rdi,[0xfe08]
        xor     rax,rax
        mov     ax,[rsp+8+4+8]
        lea     rax,[rax+rax*2]
        add     rsi,rax
        dec     rbx
        jz      nonewpi12
        jmp     newpi12

  nonewpi12:

        add     rsp,14*4
        mov     rax,0
        ret


  pi32bit12:

        cld
        shr     rcx,2

   newpi3212:

        push    rdi
        push    rsi
        push    rcx
        push    rbx

        mov     rax,rdi
        sub     rax,[0xfe80]
        shr     rax,2
        add     rax,0x400000
        mov     rbx,[0x3010]
        mov     bl,[rbx+0xe]
        mov     bh,[rsp+8*4]

      np3212:

        cmp     bl,[rax]
        jnz     imp32no12
        mov     rdx,[rsi]
        cmp     bh,0
        jz      imp32yes12
        call    dbcplimit
        jnz     imp32no12

      imp32yes12:

        push    rax
        push    rdi
        mov     rax,rdi
        sub     rax,[0xfe80]
        shr     rax,16
        call    set_bank
        and     rdi,0xffff
        add     rdi,0xa0000
        mov     [rdi],rdx
        sti
        pop     rdi
        pop     rax

      imp32no12:

        inc     rax
        add     rsi,3
        add     rdi,4
        dec     rcx
        jnz     np3212

      np32d12:

        pop     rbx
        pop     rcx
        pop     rsi
        pop     rdi

        add     rdi,[0xfe08]
        movzx   rax,word [rsp+8+4+8]
        lea     rax,[rax+rax*2]
        add     rsi,rax
        dec     rbx
        jz      nonewpi3212
        jmp     newpi3212

  nonewpi3212:

        add     rsp,14*4
        mov     rax,0
        ret


vesa12_read_screen_pixel:

        and     rax,0x3FFFFF
        cmp     [0xfbf1],byte 24      ; 24 or 32 bpp ?
        jz      v12rsp24
        mov     rdi,rax
        shl     rdi,2
        mov     rax,rdi
        shr     rax,16
        call    set_bank
        and     rdi,65535
        add     rdi,0xa0000
        mov     rax,[rdi]
        and     rax,0x00ffffff
        ret
  v12rsp24:

        imul    rax,3
        mov     rdi,rax
        shr     rax,16
        call    set_bank
        and     rdi,65535
        add     rdi,0xa0000
        mov     rax,[rdi]
        and     rax,0x00ffffff
        ret


