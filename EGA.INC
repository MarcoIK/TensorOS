;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;   Ega.inc
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


checkEgaCga:

        mov     r8d,0x13
        cmp     [0xFE0C],r8d
        je      cnvl
        retq
      cnvl:

        pushdu
        mov     rcx,[0xFB0A]
        cmp     rcx,[novesachecksum]
        jne     novesal
        popdu
        retq

      novesal:
        mov     [novesachecksum],rcx
        mov     rcx,0
        movzx   rax,word [0xFB0C]
        cmp     rax,100
        jge     m13l3
        mov     rax,100
      m13l3:
        cmp     rax,480-100
        jbe     m13l4
        mov     rax,480-100
      m13l4:
        sub     rax,100
        imul    rax,640*4
        add     rcx,rax
        movzx   rax,word [0xFB0A]
        cmp     rax,160
        jge     m13l1
        mov     rax,160
      m13l1:
        cmp     rax,640-160
        jbe     m13l2
        mov     rax,640-160
      m13l2:
        sub     rax,160
        shl     rax,2
        add     rcx,rax
        mov     rsi,[0xFE80]
        add     rsi,rcx
        mov     rdi,0xa0000
        mov     rdx,200
        mov     rcx,320
        cld
     m13pix:
        lodsd
        push    rax
        mov     rbx,rax
        and     rax,(128+64+32)      ; blue
        shr     rax,5
        and     rbx,(128+64+32)*256  ; green
        shr     rbx,8+2
        add     rax,rbx
        pop     rbx
        and     rbx,(128+64)*256*256 ; red
        shr     rbx,8+8
        add     rax,rbx
        stosb
        loop    m13pix
        mov     rcx,320
        add     rsi,4*(640-320)
        dec     rdx
        jnz     m13pix

        popdu
        retq


palette320x200:

        mov     rdx,0x3C8
        mov     rax,0
        out     dx,al
        mov     rcx,256
        mov     rdx,0x3C9
        xor     rax,rax

     palnew:
        mov     al,0
        test    ah,64
        jz      pallbl1
        add     al,21
     pallbl1:
        test    ah,128
        jz      pallbl2
        add     al,42
     pallbl2:
        out     dx,al
        mov     al,0
        test    ah,8
        jz      pallbl3
        add     al,8
     pallbl3:
        test    ah,16
        jz      pallbl4
        add     al,15
     pallbl4:
        test    ah,32
        jz      pallbl5
        add     al,40
     pallbl5:
        out     dx,al
        mov     al,0
        test    ah,1
        jz      pallbl6
        add     al,8
     pallbl6:
        test    ah,2
        jz      pallbl7
        add     al,15
     pallbl7:
        test    ah,4
        jz      pallbl8
        add     al,40
     pallbl8:
        out     dx,al
        add     ah,1
        loop    palnew

        retq
