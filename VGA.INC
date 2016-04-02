;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                        ;;
;;  VGA.INC                                               ;;
;;  640x480 mode 0x12 VGA functions for Tensor            ;;
;;  Marco K, oldklickers@gmail.com                        ;;
;;                                                        ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

paletteVGA:

    ; 16 colour palette

        mov     dx,0x3c8
        mov     al,0
        out     dx,al

        mov     rcx,16
        mov     dx,0x3c9
        xor     rax,rax

  palvganew:

        mov     al,0
        test    ah,4
        jz      palvgalbl1
        add     al,31
        test    ah,8
        jz      palvgalbl1
        add     al,32

  palvgalbl1:

        out     dx,al                   ; red 0,31 or 63
        mov     al,0
        test    ah,2
        jz      palvgalbl2
        add     al,31
        test    ah,8
        jz      palvgalbl2
        add     al,32

  palvgalbl2:

        out     dx,al                   ; blue 0,31 or 63
        mov     al,0
        test    ah,1
        jz      palvgalbl3
        add     al,31
        test    ah,8
        jz      palvgalbl3
        add     al,32

  palvgalbl3:

        out     dx,al                   ; green 0,31 or 63
        add     ah,1
        loop    palvganew

        retq

vga_putimage:

        push    rbp ;
        push    rsi ;
        push    rdi ;

        push    rax ;
        push    rbx ; +16 [ptrImage]
        push    rcx ; +8 [BH]
        push    rdx ; +0 [xy]

        movzx   rax,word [rsp+8]        ; rax:=x
        movzx   rbx,word [rsp+0]        ; rbx:=y
        mov     rcx,[0x6020]            ;
        add     rax,[rcx-twdw]          ; rax+=Xwin
        add     rbx,[rcx-twdw+8]        ; rbx+=Ywin
        mov     rcx,rbx                 ; rcx = y+Ywin
        mov     rdx,rax                 ; rdx = x+Xwin

        imul    rbx,640*4               ; (y+Ywin)*BytesPerScanLine
        shl     rax,2                   ; (x+Xwin)*BytesPerPixel
        add     rax,rbx                 ;
        mov     rdi,rax                 ; store copy
        add     rax,[0xFE80]            ; +AddrLFB

        ;entry point in LFB >> EAX:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel+AddrLFB

        shr     rdi,5                   ; change from 4 to 1/8 BytesPerPixel
        add     rdi,0xA0000             ; + VGABasePtr

        ;VGA start address >> EDI:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel+AddrVGA

        mov     rbx, [0xFE00]           ; ScreenXSize
        inc     rbx                     ; +1
        imul    rbx,rcx                 ; *(y+Ywin)
        mov     rbp,rbx                 ;
        add     rbp,rdx                 ; +(x+Xwin)
        add     rbp,0x400000            ; rbp:=(y+Ywin)*(ScreenXSize+1)+(x+Xwin)+AddrBuffer

        mov     rsi,[rsp+32]            ; rsi:=AddrImg
        movzx   rcx,word [rsp+24]       ; rcx:=B
        movzx   rbx,word [rsp+16]       ; rbx:=H

        ; Check limits while draw ?

        push    rcx ; B
        push    rax ; LFB address

        mov     rax,[0x6020]
        mov     rcx,[rax+draw_data-0x6000+0]
        cmp     rcx,0
        jnz     dbcblimitlset_vga

        mov     rcx,[rax+draw_data-0x6000+16]
        cmp     rcx,0
        jnz     dbcblimitlset_vga

        mov     rcx,[rax+draw_data-0x6000+16]
        cmp     rcx,[0xFE00] ; rcx <> Screen X size
        jnz     dbcblimitlset_vga

        mov     rcx,[rax+draw_data-0x6000+24]
        cmp     rcx,[0xFE04] ; rcx <> Screen Y size
        jnz     dbcblimitlset_vga

        pop     rax ; LFB address
        pop     rcx ; B

        mov     r8,0
        push    r8

        jmp     pimvga

  dbcblimitlset_vga:

        pop     rax ; LFB address
        pop     rcx ; B

        mov     r8,1
        push    r8

pimvga:

        push    rdi
        push    rsi
        push    rax  ; LFB address
        push    rcx  ; B
        push    rbx  ; H
        push    rdx  ; x+Xwin

        mov     rbx,[0x6020]
        mov     bl,[rbx+0x1C]
        mov     bh,[rsp+24*8]

        cld

 npvga:

        cmp     bl,[ds:rbp]
        jnz     impvgano

        cmp     bh,0
        jz      impvgayes

        call    voodoodbcplimit
        jnz     impvgano

 impvgayes:

        push    rax  ; LFB address
        push    rbx  ; app no.
        push    rcx  ; B
        push    rdx  ; x+Xwin

        mov     rdx,[rsi] ; color
        mov     [rax],dx
        shr     rdx,16
        mov     [rax+2],dl

        mov     rax,[rsi]   ; color
        mov     rcx,[rsp]   ; x+Xwin
        and     rcx,0x07    ; modulo 8
        call    setvgapixel ; eax=color, ecx=x%8, edi=VGA address

        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax

  impvgano:

        add     rsi,3 ; rsi+=3 ptrImage+=3
        add     rax,4 ; rax+=4 LFBaddr +=4
        inc     rbp
        inc     rdx   ; x+Xwin+n

        test    rdx,0x07  ; test modulo 8
        jnz     impvgacont
        inc     rdi

  impvgacont:

        dec     rcx ; B--
        jnz     npvga

        pop     rdx
        pop     rbx
        pop     rcx
        pop     rax
        pop     rsi
        pop     rdi

        add     rdi,640/8  ; add one VGA line
        add     rax,640*4  ; add one LFB line

        sub     rbp,rcx      ;  -B
        add     rbp,[0xFE00] ;
        inc     rbp          ; ptrBuffer:=ptrBuffer-B+Screen_Xsize+1

        push    rcx
        lea     rcx,[rcx+rcx*2] ;
        add     rsi,rcx         ; ptrImage:=ptrImage+B*3
        pop     rcx

        dec     rbx ; H--
        jnz     near pimvga

        add     rsp,8  ; jump saved limit byte
        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax
        pop     rdi
        pop     rsi
        pop     rbp

        retq


VGA_putpixel:

        ; rax = x
        ; rbx = y

        mov     rcx,rax
        mov     rax,[rsp+64-16+8]       ; color

        imul    rbx,640*4               ; y*BytesPerLine (Vesa2.0 32)
        lea     rdx,[rbx+rcx*4]         ; + x*BytesPerPixel (Vesa2.0 32)

        mov     rdi,rdx
        add     rdi,[0xFE80]            ; + LFB address
        mov     [rdi],rax               ; write to LFB for Vesa2.0

        shr     rdx,5                   ; change BytesPerPixel to 1/8
        mov     rdi,rdx
        add     rdi,0x0A0000            ; address of pixel in VGA area

        and     rcx,0x07                ; bit no. (modulo 8)

setvgapixel:

        ; rdi = address, rax = 24bit colour, rcx = bit no. (modulo 8)

        push    rax
        mov     rbx,rax                 ; color

        ;mov    al,0x08
        ;mov    dx,0x03ce
        ;out    dx,al                   ; select GDC bit mask register

        inc     cl
        mov     ax, 0x100
        shr     ax,cl
        mov     dx,0x03cf
        out     dx,al                   ; set bit mask for pixel

        mov     dl,0
        mov     rax,rbx
        and     rax,0x000000FF          ; blue
        cmp     rax,85
        jle     p13green
        or      dl,0x01
        cmp     rax,170
        jle     p13green
        or      dl,0x08

p13green:

        and     rbx,0x0000FF00          ; green
        cmp     rbx,85*256
        jle     p13red
        or      dl,0x02
        cmp     rbx,170*256
        jle     p13red
        or      dl,0x08

p13red:

        pop     rbx
        and     rbx,0x00FF0000          ; red
        cmp     rbx,85*256*256
        jle     p13cont
        or      dl,0x04
        cmp     rbx,170*256*256
        jle     p13cont
        or      dl,0x08

p13cont:

        mov     al,[rdi]                ; dummy read
        mov     [rdi],dl

        retq

vga_drawbar:

        sub     rdx,rbx ; rdx:=Yend-Ystart=H
        sub     rcx,rax ; rcx:=Xend-Xstat=B

        push    rbp ; +48
        push    rsi ; +40
        push    rdi ; +32
        push    rax ; +24
        push    rbx ; +16
        push    rcx ; +8
        push    rdx ; +0

        mov     rcx,[0x6020]            ;
        add     rax,[rcx-twdw]          ; rax:=Xwin+x
        add     rbx,[rcx-twdw+8]        ; rbx:=Ywin+y
        mov     rcx,rax                 ; rcx:=(x+Xwin)
        mov     rdx,rbx                 ; rdx:=(y+Ywin)

        imul    rbx,640/8               ;
        mov     rdi,rbx                 ; rdi:=BytesPerScanLine*(y+Ywin)
        shr     rax,3                   ;
        add     rdi,rax                 ;  + (x+Xwin)*BytesPerPixel
        add     rdi,0xA0000             ; + VGAbaseaddress

        mov     rax,[0xFE00]            ; ScreenXSize
        inc     rax                     ; +1
        imul    rax,rdx                 ; *(y+Ywin)
        mov     rbp,rax                 ;
        add     rbp,rcx                 ; +(x+Win)
        add     rbp,0x400000            ; +AddrBuffer

        mov     rax,[0xFE08]            ; BytesPerScanLine - LFB
        mul     rdx                     ; *(y+Ywin)
        mov     rsi,rax
        add     rsi,rcx
        add     rsi,rcx
        add     rsi,rcx
        add     rsi,rcx                 ; + 4*(x+Xwin)
        add     rsi,[0xFE80]            ; +AddrLFB

; rdi:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel + AddrVGA
; rsi:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel + AddrLFB
; rbp:=(y+Ywin)*(ScreenXSize+1)+(x+Xwin)+AddrBuffer

; x size

        mov     rax,[rsp+8] ; B [rsp+8]
        mov     rbx,[rsp+0] ; H

        mov     rdx,[rsp+32] ; color
        mov     r8,0x80000000
        test    rdx,r8
        jz      nodbglvga

        ; no color glide for VGA - set to half glide

        shr     rbx,1  ; H/2
        sub     rdx,rbx
        mov     [rsp+32],rdx
        mov     rbx,[rsp+0] ; reset to H

   nodbglvga:

    ; check limits ?

        push    rax
        push    rcx

        mov     rax,[0x6020]

        mov     rcx,[rax+draw_data-0x6000+0]
        cmp     rcx,0
        jnz     dbcblimitlset_vga2

        mov     rcx,[rax+draw_data-0x6000+8]
        cmp     rcx,0
        jnz     dbcblimitlset_vga2

        mov     rcx,[rax+draw_data-0x6000+16]
        cmp     rcx,[0xfe00]
        jnz     dbcblimitlset_vga2

        mov     rcx,[rax+draw_data-0x6000+24]
        cmp     rcx,[0xfe04]
        jnz     dbcblimitlset_vga2

        pop     rcx
        pop     rax

        mov     r8,0

        push    r8

        jmp     dbnewpivga

  dbcblimitlset_vga2:

        pop     rcx ; x+Xwin
        pop     rax ; B

        mov     r8,1
        push    r8

  dbnewpivga:

        push    rax ; B
        push    rbx ; H
        push    rdi
        push    rsi
        push    rcx ; x+Xwin

        mov     rbx,[0x6020]
        movzx   rbx,byte[rbx+0x1C]

        cld

  dbnpvga:

        mov     dl,[ds:rbp]

        cmp     dl,bl
        jnz     dbimpvgano

        mov     rdx,[rsp+10*8] ; check limit?
        cmp     rdx,0
        jz      dbimpvgayes

        call    voodoodbcplimit
        jnz     dbimpvgano

  dbimpvgayes:

        push    rax ; B
        push    rbx
        push    rcx ; x+Xwin

        mov     rax,[rsp+24+40+32+8]    ; color
        mov     rbx,rax

        mov     [rsi],bx                ; write LFB pixel
        shr     rbx,16
        mov     [rsi+2],bl

        and     rcx,0x07                ; modulo 8
        call    setvgapixel             ; rax=color, rcx=x%8, rdi=VGA address

        pop     rcx
        pop     rbx
        pop     rax

  dbimpvgano:

        add     rsi,4    ; ptrLFB+=4
        inc     rbp      ; address buffer
        inc     rcx      ; x posn++
        test    rcx,0x07 ; test modulo 8
        jnz     dbvgacont
        inc     rdi      ; VGA screen ptr++

  dbvgacont:

        dec     rax      ; B--  NB rcx in Vesa20 fn?
        jnz     dbnpvga

  dbnpvgad:

        pop     rcx
        pop     rsi
        pop     rdi
        pop     rbx
        pop     rax

        add     rsi,[0xFE08]            ; ptrLFB+=BytesPerScanLine
        add     rdi,640/8               ; ptrScreen+=BytesPerScanLine

        add     rbp,[0xFE00]            ;
        sub     rbp, rax                ; was ecx in vesa20 fn?
        inc     rbp                     ; ptrBuffer:=ptrBuffer-B+BytesPerLine+1

        dec     rbx                     ; H--
        jz      nodbnewpivga            ; H<>0

        jmp     dbnewpivga

  nodbnewpivga:

        add     rsp,14*8                ; NB includes limit check flag
        ;pop    rbx
        ;pop    rax
        ;pop    rdi
        ;pop    rsi
        pop     rbp

        ;pop    rdx
        ;pop    rcx

        retq

vga_drawbackground_tiled:

        push    rbp
        push    rax
        push    rbx
        push    rcx
        push    rdx

        mov     r8,[0x400000-16]
        mov     rdx,r8 ; B
        add     rdx,r8 ; +B
        add     rdx,r8 ; +B
        push    rdx

        mov     rax,[draw_data+64+0] ; x start:=(x+Xwin)
        mov     rbx,[draw_data+64+8] ; y start:=(y+Ywin)
        mov     rcx,rax
        mov     rdx,rbx

        imul    rdx,640*4           ; (y+Ywin)*BytesPerScanLine
        shl     rcx,2               ; (x+Xwin)*BytesPerPixel
        add     rcx,rdx             ;
        mov     rbp,rcx             ; store copy
        add     rcx,[0xFE80]        ; +AddrLFB

        ;entry point in LFB >> ECX:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel+Addr

        shr     rbp,5                ; change from 4 to 1/8 BytesPerPixel
        add     rbp,0xA0000          ; + VGABasePtr

        ;VGA start address >> EBP:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel+AddrV

        call    calculate_edi

  dp3vga:                                 ; MAIN LOOP

        cmp     [rdi+0x400000],byte 1     ; ptrBuffer^<>byte(1)
        je      ybgpvga

        jmp     nbgpvga

  ybgpvga:

        push    rax  ; x
        push    rbx  ; y
        push    rcx  ; LFB address

        mov     r8,[0x400000-16]
        mov     rcx,r8   ; B
        xor     rdx,rdx                   ; rdx:=0
        div     rcx                       ; Xstart/B

        ; eax=Int(qn) rdx:=Rem

        lea     rsi,[rdx+rdx*2]           ; rsi:=rdx*3

        mov     r8,[0x400000-16]
        mov     rcx,r8                    ; rcx:=H
        mov     rax,[rsp+8]               ; rax:=Ystart
        xor     rdx,rdx                   ;
        div     rcx                       ; Ystart/H

        mov     rax,rdx                   ; rax:=Rem
        xor     rdx,rdx                   ;
        mov     rbx,[rsp+24]              ; rbx:=B*3
        mul     rbx                       ;
        add     rsi,rax                   ;

        mov     rax,[rsi+0x300000]        ; color
        and     rax,0xFFFFFF

        mov     rcx,[rsp]                 ; LFB address
        mov     rbx,rax                   ; copy color
        mov     [rcx],bx
        shr     rbx,16
        mov     [rcx+2],bl

        xchg    rdi,rbp
        mov     rcx,[rsp+16]              ; x position
        and     rcx,0x07                  ; x modulo 8
        call    setvgapixel               ; rax=color, rcx=x%8, rdi=VGA address
        xchg    rbp,rdi

        pop     rcx
        pop     rbx
        pop     rax

  nbgpvga:

        inc     rax                       ; x++
        cmp     rax,[draw_data+64+16]     ; X > xend?
        jg      nodp3vga

        test    rax,0x07                  ; x test modulo 8
        jnz     hook1vga
        inc     rbp                       ; VGA address++

   hook1vga:

        add     rcx,4                     ; LFB address += 4
        inc     rdi                       ; ptrBuffer++
        add     rsi,3                     ; ptrImage+=3
        jmp     dp3vga

  nodp3vga:

        mov     rax,[draw_data+64+0]      ; x+Xwin
        inc     rbx                       ; y position
        mov     rcx,rax
        mov     rdx,rbx

        imul    rdx,640*4            ; (y+Ywin)*BytesPerScanLine
        shl     rcx,2                ; (x+Xwin)*BytesPerPixel
        add     rcx,rdx              ;
        mov     rbp,rcx              ; store copy
        add     rcx,[0xFE80]         ; +AddrLFB

        ;entry point in LFB >> ECX:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel+Addr

        shr     rbp,5                ; change from 4 to 1/8 BytesPerPixel
        add     rbp,0xA0000          ; + VGABasePtr

        ;VGA start address >> EBP:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel+AddrV

        call    calculate_edi

        cmp     rbx,[draw_data+64+24]         ; Y > yend
        jg      dp4vga

        jmp     dp3vga

  dp4vga:

        add     rsp,8

        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax
        pop     rbp

        retq

; ----------

vga_drawbackground_stretch:

        push    rbp
        push    rax
        push    rbx
        push    rcx
        push    rdx

        mov     r8,[0x400000-16]
        mov     rdx,r8 ; B
        add     rdx,r8 ; +B
        add     rdx,r8 ; +B
        push    rdx

        mov     rax,[draw_data+64+0] ; x start:=(x+Xwin)
        mov     rbx,[draw_data+64+8] ; y start:=(y+Ywin)
        mov     rcx,rax
        mov     rdx,rbx

        imul    rdx,640*4            ; (y+Ywin)*BytesPerScanLine
        shl     rcx,2                ; (x+Xwin)*BytesPerPixel
        add     rcx,rdx              ;
        mov     rbp,rcx              ; store copy
        add     rcx,[0xFE80]         ; +AddrLFB

        ;entry point in LFB >> ECX:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel+Addr

        shr     rbp,5                ; change from 4 to 1/8 BytesPerPixel
        add     rbp,0xA0000          ; + VGABasePtr

        ;VGA start address >> EBP:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel+AddrV

        call    calculate_edi

   sdp3vga:                         ; MAIN LOOP

        cmp     [rdi+0x400000],byte 1     ; ptrBuffer^<>byte(1)
        je      sybgpvga

        jmp     snbgpvga

  sybgpvga:

        push    rax   ; x
        push    rbx   ; y
        push    rcx   ; LFB address

        mov     r8,[0x400000-16]
        mov     rax,r8  ; B
        xor     rdx,rdx
        mov     rbx,[rsp+16]            ; Xstart
        mul     rbx                     ; B*Xstart
        xor     rdx,rdx
        mov     rbx,[0xFE00]            ; x screen width
        div     rbx                     ; B*Xstart/xwidth
        lea     rsi,[rax+rax*2]         ; *3
        mov     rax,r8                  ; H
        xor     rdx,rdx
        mov     rbx,[rsp+8]             ; Ystart
        mul     rbx                     ; H*Ystart
        xor     rdx,rdx
        mov     rbx,[0xFE04]            ; y screen height
        div     rbx                     ; H*Ystart/yheight

        xor     rdx,rdx
        mov     rbx,[rsp+24]            ; B*3
        mul     rbx                     ;
        add     rsi,rax
        mov     rax,[rsi+0x300000]      ; color
        and     rax,0xFFFFFF

        mov     rcx,[rsp]               ; LFB address
        mov     rbx,rax                 ; copy color
        mov     [rcx],bx
        shr     rbx,16
        mov     [rcx+2],bl

        xchg    rdi,rbp
        mov     rcx,[rsp+16]            ; x position
        and     rcx,0x07                ; x modulo 8
        call    setvgapixel             ; rax=color, rcx=x%8, rdi=VGA address
        xchg    rbp,rdi                 ; rbp+=3

        pop     rcx
        pop     rbx
        pop     rax

  snbgpvga:

        inc     rax                       ; x++
        cmp     rax,[draw_data+64+16]      ; X > xend?
        jg      snodp3vga

        test    rax,0x07                  ; x test modulo 8
        jnz     shook1vga
        inc     rbp                       ; VGA address++

   shook1vga:

        add     rcx,4                     ; LFB address += 4
        inc     rdi                       ; ptrBuffer++
        add     rsi,3                     ; ptrImage+=3
        jmp     sdp3vga

  snodp3vga:

        mov     rax,[draw_data+64+0]      ; x+Xwin
        inc     rbx                       ; y position
        mov     rcx,rax
        mov     rdx,rbx

        imul    rdx,640*4            ; (y+Ywin)*BytesPerScanLine
        shl     rcx,2                ; (x+Xwin)*BytesPerPixel
        add     rcx,rdx              ;
        mov     rbp,rcx              ; store copy
        add     rcx,[0xFE80]         ; +AddrLFB

        ;entry point in LFB >> ECX:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel+Addr

        shr     rbp,5                ; change from 4 to 1/8 BytesPerPixel
        add     rbp,0xA0000          ; + VGABasePtr

        ;VGA start address >> EBP:=(y+Ywin)*BytesPerScanLine+X*BytesPerPixel+A

        call    calculate_edi

        cmp     rbx,[draw_data+64+24]        ; Y > yend
        jg      sdp4vga

        jmp     sdp3vga

  sdp4vga:

        add     rsp,8

        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax
        pop     rbp

        retq


