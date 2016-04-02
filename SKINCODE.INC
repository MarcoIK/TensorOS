
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                          ;;
;;   WINDOW SKIN for TensorOS               ;;
;;                                          ;;
;;   oldklickers@gmail.com                  ;;
;;                                          ;;
;;   Bugfixes & upgrades by                 ;;
;;             Marco K                      ;;
;;                                          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



include "SKINDATA.INC"

load_default_skin:

        pushdu
        cmp     [_refleft],0
        jnz     skinok1
        mov     [_refleft],0x778000
        mov     rax,_fileleft
        mov     rbx,1
        mov     rcx,-1
        mov     rdx,[_refleft]
        mov     rsi,12
        call    fileread
skinok1:
        cmp     [_refbase], 0
        jnz     skinok2
        mov     rax,[v_skinh]
        mov     rbx,[_skinleftw]
        imul    rax,rbx
        imul    rax,3
        add     rax,[_refleft]
        mov     [_refbase],rax
        mov     rax,_filebase
        mov     rbx,1
        mov     rcx,-1
        mov     rdx,[_refbase]
        mov     rsi,12
        call    fileread
skinok2:
        cmp     [_refoper], 0
        jnz     skinok3
        mov     rax,[v_skinh]
        mov     rbx,[_skinbasew]
        imul    rax,rbx
        imul    rax,3
        add     rax,[_refbase]
        mov     [_refoper],rax
        mov     rax,_fileoper
        mov     rbx,1
        mov     rcx,-1
        mov     rdx,[_refoper]
        mov     rsi,12
        call    fileread
skinok3:
        popdu
        retq


drawwindow_IV:

        pushdu

        push    rdx

        mov     rdi,[rsp]                              ; RECTANGLE

        mov     rax,[rdi+0]
        shl     rax,16
        mov     ax,[rdi+0]
        add     ax,[rdi+16]
        mov     rbx,[rdi+8]
        shl     rbx,16
        mov     bx,[rdi+8]
        add     bx,[rdi+24]
        mov     rsi,[_coloroutborder]
        call    draw_rectangle
        mov     rcx,3
      _dw3l:
        add     rax,1*65536-1
        add     rbx,1*65536-1
        mov     rsi,[_colorframe] ;[rdi+24]
        call    draw_rectangle
        dec     rcx
        jnz     _dw3l
        mov     rsi,[_colorborder]
        add     rax,1*65536-1
        add     rbx,1*65536-1
        call    draw_rectangle

        ; Middle skin

        mov     rsi,[rsp]
        mov     rax,[rsi+16]
        mov     rdx,0
        mov     rbx,[_skinbasew]
        div     rbx
        and     rax,0xFFF
        inc     rax
        mov     rbx,[_refbase]
        mov     rcx,[_skinbasew]
        shl     rcx,16
        add     rcx,[_skinh]
        mov     rdx,[_skinbase]
        shl     rdx,16
        mov     rdx , 0 ; x,y start

      baseskinloop:

        push    rax
        push    rbx
        push    rcx
        push    rdx
        call    sys_putimage
        ; mov   rax , 10
        ; call  delay_hs
        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax

        shr     rdx,16
        add     rdx,[_skinbasew]
        shl     rdx,16

        dec     rax
        cmp     rax, 0
        jnz     baseskinloop

        ; Left skin

        mov     rsi,[rsp]
        mov     rax,[rsi+16]
        mov     rdx,[_skinleft]
        shl     rdx,16
        mov     rcx,[_skinleftw]
        shl     rcx,16
        add     rcx,[_skinh]
        mov     rbx,[_refleft]
        call    sys_putimage

        ; Right skin

        mov     rsi,[rsp]
        mov     rdx,[rsi+8]
        sub     rdx,[_skinoperw]
        inc     rdx
        shl     rdx,16
        mov     rbx,[_refoper]
        mov     rcx,[_skinoperw]
        shl     rcx,16
        add     rcx,[_skinh]
        call    sys_putimage

        mov     rsi,[rsp]

        mov     rdx,[rsi+08]                       ; WORK AREA
        add     rdx,21+5
        mov     rbx,[rsi+08]
        add     rbx,[rsi+24]
        cmp     rdx,rbx
        jg      _noinside2
        mov     rax,5
        mov     rbx,[_skinh]
        mov     rcx,[rsi+16]
        mov     rdx,[rsi+24]
        sub     rcx,4
        sub     rdx,4
        mov     rdi,[rsi+32]
        call  drawbar
      _noinside2:

        mov     rdi,[0xFE88]
        movzx   rax,word [rdi]
        cmp     rax,1000
        jge     no_skin_add_button
        inc     rax
        mov     [rdi],ax

        shl     rax,4
        add     rax,rdi

        mov     bx,[0x6000]
        mov     [rax],bx

        add     rax,2         ; save button id number
        mov     bx,1
        mov     [rax],bx
        mov     bx,0x2000     ; button id : bits 16-31 - do not draw press
      ;  mov   bx , 0x0000
        mov     [rax-2+0x18],bx

        add     rax,2         ; x start
        mov     rbx,[rsp]
        mov     rbx,[rbx+8]
        cmp     [_buttonCx],0
        jg      _bCx_at_right
        mov     rbx,[_buttonCw]    ; ebx will be 0 in next instruction
      _bCx_at_right:
        sub     rbx,[_buttonCw]
        sub     rbx,[_buttonCx]
        mov     [rax],bx
        add     rax,2         ; x size
        mov     rbx,[_buttonCw]
        mov     [rax],bx
        add     rax,2         ; y start
        mov     rbx,[_buttonCy]
        mov     [rax],bx
        add     rax,2         ; y size
        mov     rbx,[_buttonCh]
        mov     [rax],bx

      no_skin_add_button:

        add     rsp,8
        popdu
        retq




drawwindow_V:

        pushdu

        push    rdx

        ; Rectangle

        mov     rdi,[rsp]

        mov     rax,[rdi+0]
        shl     rax,16
        mov     ax,[rdi+0]
        add     ax,[rdi+16]
        mov     rbx,[rdi+8]
        shl     rbx,16
        mov     bx,[rdi+8]
        add     bx,[rdi+24]
        mov     rsi,[_coloroutborder]
        call    draw_rectangle
        mov     rcx,3
      v_dw3l:
        add     rax,1*65536-1
        add     rbx,1*65536-1
        mov     rsi,[_colorframe] ;[rdi+48]
        call    draw_rectangle
        dec     rcx
        jnz     v_dw3l
        mov     rsi,[_colorborder]
        add     rax,1*65536-1
        add     rbx,1*65536-1
        call    draw_rectangle

        ; Middle skin

        mov     rsi,[rsp]
        mov     rax,[rsi+16]
        mov     rdx,0
        mov     rbx,[v_skinbasew]
        div     rbx
        and     rax,0xFFF
        inc     rax
        mov     rbx,[_refbase]
        mov     rcx,[v_skinbasew]
        shl     rcx,16
        add     rcx,[v_skinh]
        mov     rdx,[v_skinbase]
        shl     rdx,16
        mov     rdx,0 ; x,y start

      baseskinloop2:

        push    rax
        push    rbx
        push    rcx
        push    rdx
        call    sys_putimage
        ; mov   eax , 10
        ; call  delay_hs
        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax

        shr     rdx,16
        add     rdx,[v_skinbasew]
        shl     rdx,16

        dec     rax
        cmp     rax, 0
        jnz     baseskinloop2


        ; Left skin

        mov     rsi,[rsp]
        mov     rax,[rsi+16]    ; window width
        mov     rdx,[v_skinleft]
        shl     rdx,16
        mov     rcx,[v_skinleftw]
        shl     rcx,16
        add     rcx,[v_skinh]
        mov     rbx,[_refleft]
        call    sys_putimage

        ; Middle skin

  ;      mov   esi,[esp]
  ;      mov   eax,[esi+8]
  ;      sub   eax,[v_skinleftw]
  ;      sub   eax,[v_skinoperw]
  ;      mov   edx,0
  ;      mov   ebx,[v_skinbasew]
  ;      div   ebx

  ;      and   eax , 0xfff
  ;      inc   eax

  ;      mov   ebx,[_refbase]
  ;      mov   ecx,[v_skinbasew]
  ;      shl   ecx,16
  ;      add   ecx,[v_skinh]
  ;      mov   edx,[v_skinbase]
  ;      sub   edx,[v_skinbasew]
  ;      shl   edx,16
  ;    vbaseskinloop:
  ;      shr   edx,16
  ;      add   edx,[v_skinbasew]
  ;      shl   edx,16
  ;      push  eax
  ;      push  ebx
  ;      push  ecx
  ;      push  edx
  ;      call  sys_putimage

  ;      pop   edx
  ;      pop   ecx
  ;      pop   ebx
  ;      pop   eax
  ;      dec   eax
  ;      cmp   eax, 0
  ;      jnz   vbaseskinloop
  ;    vnon_base:

        ; Skin right

        mov     rsi,[rsp]
        mov     rdx,[rsi+16]
        sub     rdx,[v_skinoperw]
        inc     rdx
        shl     rdx,16
        mov     rbx,[_refoper]
        mov     rcx,[v_skinoperw]
        shl     rcx,16
        add     rcx,[v_skinh]
        call    sys_putimage

        ; Work area

        mov     rsi,[rsp]
        mov     rdx,[rsi+08]
        add     rdx,[v_skinh] ;  21+5+18
        mov     rbx,[rsi+08]
        add     rbx,[rsi+24]
        cmp     rdx,rbx
        jg      v_noinside2
        mov     rax,5
        mov     rbx,[v_skinh]
        mov     rcx,[rsi+16]
        mov     rdx,[rsi+24]
        sub     rcx,4
        sub     rdx,4
        mov     rdi,[rsi+32]
        call    drawbar
      v_noinside2:

        mov     rdi,[0xfe88]
        movzx   rax,word [rdi]
        cmp     rax,1000
        jge     vno_skin_add_button
        inc     rax
        mov     [rdi],ax

        shl     rax,4
        add     rax,rdi

        mov     bx,[0x6000]
        mov     [rax],bx

        add     rax,2         ; save button id number
        mov     bx,1
        mov     [rax],bx
        mov     bx,0x2000     ; button id : bits 16-31 - do not draw press
      ;  mov    bx , 0x0000
        mov     [rax-2+0x18],bx

        add     rax,2         ; x start
        mov     rbx,[rsp]
        mov     rbx,[rbx+16]
        cmp     [_buttonCx],0
        jg      v_bCx_at_right
        mov     rbx,[_buttonCw]    ; ebx will be 0 in next instruction
      v_bCx_at_right:
        sub     rbx,[_buttonCw]
        sub     rbx,[_buttonCx]
        mov     [rax],bx
        add     rax,2         ; x size
        mov     rbx,[_buttonCw]
        mov     [rax],bx
        add     rax,2         ; y start
        mov     rbx,[_buttonCy]
        mov     [rax],bx
        add     rax,2         ; y size
        mov     rbx,[_buttonCh]
        mov     [rax],bx

      vno_skin_add_button:

        add     rsp,8
        popdu
        ret





