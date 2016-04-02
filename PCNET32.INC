;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                    ;;
;;  PCNET32.INC                                                       ;;
;;                                                                    ;;
;;  Ethernet driver for Menuet OS                                     ;;
;;                                                                    ;;
;;  Version 1.0  31 July 2004                                         ;;
;;                                                                    ;;
;;  This driver is based on the PCNet32 driver from                   ;;
;;  the etherboot 5.0.6 project. The copyright statement is           ;;
;;                                                                    ;;
;;          GNU GENERAL PUBLIC LICENSE                                ;;
;;             Version 2, June 1991                                   ;;
;;                                                                    ;;
;;  remaining parts Copyright 2004 Jarek Pelczar,                     ;;
;;   jpelczar@interia.pl                                              ;;
;;                                                                    ;;
;;  See file COPYING for details                                      ;;
;;                                                                    ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;macro PutStr X
;{
; local .__xyz1
; local .__xyz2
; push esi
; mov esi,.__xyz1
; call sys_msg_board_str
; push eax
; mov eax,1
; call delay_hs
; pop eax
; jmp .__xyz2
;.__xyz1:
; db X
; db 13,10,0
;.__xyz2:
; pop esi
;}
PCNET32_PORT_AUI      equ 0x00
PCNET32_PORT_10BT     equ 0x01
PCNET32_PORT_GPSI     equ 0x02
PCNET32_PORT_MII      equ 0x03
PCNET32_PORT_PORTSEL  equ 0x03
PCNET32_PORT_ASEL     equ 0x04
PCNET32_PORT_100      equ 0x40
PCNET32_PORT_FD       equ 0x80
PCNET32_DMA_MASK      equ 0xffffffff
PCNET32_LOG_TX_BUFFERS  equ 1
PCNET32_LOG_RX_BUFFERS  equ 2
PCNET32_TX_RING_SIZE            equ (1 shl PCNET32_LOG_TX_BUFFERS)
PCNET32_TX_RING_MOD_MASK        equ (PCNET32_TX_RING_SIZE-1)
PCNET32_TX_RING_LEN_BITS        equ 0
PCNET32_RX_RING_SIZE            equ (1 shl PCNET32_LOG_RX_BUFFERS)
PCNET32_RX_RING_MOD_MASK        equ (PCNET32_RX_RING_SIZE-1)
PCNET32_RX_RING_LEN_BITS        equ (PCNET32_LOG_RX_BUFFERS shl 4)
PCNET32_PKT_BUF_SZ              equ 1544
PCNET32_PKT_BUF_SZ_NEG          equ 0xf9f8
pcnet32_txb equ (eth_data_start)
pcnet32_rxb equ ((pcnet32_txb+(PCNET32_PKT_BUF_SZ*PCNET32_TX_RING_SIZE)+0xf) and 0xfffffff0)
pcnet32_tx_ring equ ((pcnet32_rxb+(PCNET32_PKT_BUF_SZ*PCNET32_RX_RING_SIZE)+0xf) and 0xfffffff0)
pcnet32_rx_ring equ ((pcnet32_tx_ring+(16*PCNET32_TX_RING_SIZE)+0xf) and 0xfffffff0)
virtual at ((pcnet32_rx_ring+(16*PCNET32_RX_RING_SIZE)+0xf) and 0xfffffff0)
pcnet32_private:
.mode                   dw ?
.tlen_rlen              dw ?
.phys_addr              db ?,?,?,?,?,?
.reserved               dw ?
.filter                 dd ?,?
.rx_ring                dd ?
.tx_ring                dd ?
.cur_rx                 dd ?
.cur_tx                 dd ?
.dirty_rx               dd ?
.dirty_tx               dd ?
.tx_full                db ?
.options                dd ?
.full_duplex            db ?
.chip_version           dd ?
.mii                    db ?
.ltint                  db ?
.dxsuflo                db ?
.fset                   db ?
.fdx                    db ?
end virtual
virtual at 0
pcnet32_rx_head:
.base           dd ?
.buf_length     dw ?
.status         dw ?
.msg_length     dd ?
.reserved       dd ?
end virtual
virtual at 0
pcnet32_tx_head:
.base           dd ?
.length         dw ?
.status         dw ?
.misc           dd ?
.reserved       dd ?
end virtual
pcnet32_access:
.read_csr               dd ?
.write_csr              dd ?
.read_bcr               dd ?
.write_bcr              dd ?
.read_rap               dd ?
.write_rap              dd ?
.reset                  dd ?
pcnet32_options_mapping:
dd PCNET32_PORT_ASEL    ;  0 Auto-select
dd PCNET32_PORT_AUI     ;  1 BNC/AUI
dd PCNET32_PORT_AUI     ;  2 AUI/BNC
dd PCNET32_PORT_ASEL    ;  3 not supported
dd PCNET32_PORT_10BT or PCNET32_PORT_FD ;  4 10baseT-FD
dd PCNET32_PORT_ASEL    ;  5 not supported
dd PCNET32_PORT_ASEL    ;  6 not supported
dd PCNET32_PORT_ASEL    ;  7 not supported
dd PCNET32_PORT_ASEL    ;  8 not supported
dd PCNET32_PORT_MII     ;  9 MII 10baseT
dd PCNET32_PORT_MII or PCNET32_PORT_FD  ; 10 MII 10baseT-FD
dd PCNET32_PORT_MII     ; 11 MII (autosel)
dd PCNET32_PORT_10BT    ; 12 10BaseT
dd PCNET32_PORT_MII or PCNET32_PORT_100 ; 13 MII 100BaseTx
dd PCNET32_PORT_MII or PCNET32_PORT_100 or PCNET32_PORT_FD      ; 14 MII 100BaseTx-FD
dd PCNET32_PORT_ASEL    ; 15 not supported
PCNET32_WIO_RDP         equ 0x10
PCNET32_WIO_RAP         equ 0x12
PCNET32_WIO_RESET       equ 0x14
PCNET32_WIO_BDP         equ 0x16
PCNET32_DWIO_RDP        equ 0x10
PCNET32_DWIO_RAP        equ 0x14
PCNET32_DWIO_RESET      equ 0x18
PCNET32_DWIO_BDP        equ 0x1C
PCNET32_TOTAL_SIZE      equ 0x20
; ebx - index
; return:
; eax - data
pcnet32_wio_read_csr:
    push rdx
    lea rdx,[rbp+PCNET32_WIO_RAP]
    mov ax,bx
    out dx,ax
    lea rdx,[rbp+PCNET32_WIO_RDP]
    in ax,dx
    and rax,0xffff
    pop rdx
    ret
; eax - data
; ebx - index
pcnet32_wio_write_csr:
    push rdx
    lea rdx,[rbp+PCNET32_WIO_RAP]
    xchg rax,rbx
    out dx,ax
    xchg rax,rbx
    lea rdx,[rbp+PCNET32_WIO_RDP]
    out dx,ax
    pop rdx
    ret
; ebx - index
; return:
; eax - data
pcnet32_wio_read_bcr:
    push rdx
    lea rdx,[rbp+PCNET32_WIO_RAP]
    mov ax,bx
    out dx,ax
    lea rdx,[rbp+PCNET32_WIO_BDP]
    in ax,dx
    and rax,0xffff
    pop rdx
    ret
; eax - data
; ebx - index
pcnet32_wio_write_bcr:
    push rdx
    lea rdx,[rbp+PCNET32_WIO_RAP]
    xchg rax,rbx
    out dx,ax
    xchg rax,rbx
    lea rdx,[rbp+PCNET32_WIO_BDP]
    out dx,ax
    pop rdx
    ret
pcnet32_wio_read_rap:
    push rdx
    lea rdx,[rbp+PCNET32_WIO_RAP]
    in ax,dx
    and rax,0xffff
    pop rdx
    ret
; eax - val
pcnet32_wio_write_rap:
    push rdx
    lea rdx,[rbp+PCNET32_WIO_RAP]
    out dx,ax
    pop rdx
    ret
pcnet32_wio_reset:
    push rdx
    push rax
    lea rdx,[rbp+PCNET32_WIO_RESET]
    in ax,dx
    pop rax
    pop rdx
    ret
pcnet32_wio_check:
    push rdx
    mov ax,88
    lea rdx,[rbp+PCNET32_WIO_RAP]
    out dx,ax
    nop
    nop
    in ax,dx
    cmp ax,88
    sete al
    pop rdx
    ret
pcnet32_wio:
    dd pcnet32_wio_read_csr
    dd pcnet32_wio_write_csr
    dd pcnet32_wio_read_bcr
    dd pcnet32_wio_write_bcr
    dd pcnet32_wio_read_rap
    dd pcnet32_wio_write_rap
    dd pcnet32_wio_reset
; ebx - index
; return:
; eax - data
pcnet32_dwio_read_csr:
    push rdx
    lea rdx,[rbp+PCNET32_DWIO_RAP]
    mov rbx,rax
    out dx,eax
    lea rdx,[rbp+PCNET32_DWIO_RDP]
    in eax,dx
    and rax,0xffff
    pop rdx
    ret
; ebx - index
; eax - data
pcnet32_dwio_write_csr:
    push rdx
    lea rdx,[rbp+PCNET32_DWIO_RAP]
    xchg rax,rbx
    out dx,eax
    lea rdx,[rbp+PCNET32_DWIO_RDP]
    xchg rax,rbx
    out dx,eax
    pop rdx
    ret
; ebx - index
; return:
; eax - data
pcnet32_dwio_read_bcr:
    push rdx
    lea rdx,[rbp+PCNET32_DWIO_RAP]
    mov rbx,rax
    out dx,eax
    lea rdx,[rbp+PCNET32_DWIO_BDP]
    in eax,dx
    and rax,0xffff
    pop rdx
    ret
; ebx - index
; eax - data
pcnet32_dwio_write_bcr:
    push rdx
    lea rdx,[rbp+PCNET32_DWIO_RAP]
    xchg rax,rbx
    out dx,eax
    lea rdx,[rbp+PCNET32_DWIO_BDP]
    xchg rax,rbx
    out dx,eax
    pop rdx
    ret
pcnet32_dwio_read_rap:
    push rdx
    lea rdx,[rbp+PCNET32_DWIO_RAP]
    in eax,dx
    and rax,0xffff
    pop rdx
    ret
; eax - val
pcnet32_dwio_write_rap:
    push rdx
    lea rdx,[rbp+PCNET32_DWIO_RAP]
    out dx,eax
    pop rdx
    ret
pcnet32_dwio_reset:
    push rdx
    push rax
    lea rdx,[rbp+PCNET32_DWIO_RESET]
    in eax,dx
    pop rax
    pop rdx
    ret
pcnet32_dwio_check:
    push rdx
    lea rdx,[PCNET32_DWIO_RAP]
    mov rax,88
    out dx,eax
    nop
    nop
    in eax,dx
    and rax,0xffff
    cmp rax,88
    sete al
    pop rdx
    ret
pcnet32_dwio:
    dd pcnet32_dwio_read_csr
    dd pcnet32_dwio_write_csr
    dd pcnet32_dwio_read_bcr
    dd pcnet32_dwio_write_bcr
    dd pcnet32_dwio_read_rap
    dd pcnet32_dwio_write_rap
    dd pcnet32_dwio_reset
pcnet32_init_ring:
    mov [pcnet32_private.tx_full],0
    mov [pcnet32_private.cur_rx],0
    mov [pcnet32_private.cur_tx],0
    mov [pcnet32_private.dirty_rx],0
    mov [pcnet32_private.dirty_tx],0
    mov rdi,pcnet32_rx_ring
    mov rcx,PCNET32_RX_RING_SIZE
    mov rbx,pcnet32_rxb
.rx_init:
    mov [rdi+pcnet32_rx_head.base],ebx
    mov [rdi+pcnet32_rx_head.buf_length],word PCNET32_PKT_BUF_SZ_NEG
    mov [rdi+pcnet32_rx_head.status],word 0x8000
    add rbx,PCNET32_PKT_BUF_SZ
;    inc ebx
    add rdi,16
    loop .rx_init
    mov rdi,pcnet32_tx_ring
    mov rcx,PCNET32_TX_RING_SIZE
.tx_init:
    mov [rdi+pcnet32_tx_head.base],dword 0
    mov [rdi+pcnet32_tx_head.status],word 0
    add rdi,16
    loop .tx_init
    mov [pcnet32_private.tlen_rlen],(PCNET32_TX_RING_LEN_BITS or PCNET32_RX_RING_LEN_BITS)
    mov rsi,node_addr
    mov rdi,pcnet32_private.phys_addr
    cld
    movsd
    movsw
    mov dword [pcnet32_private.rx_ring],pcnet32_rx_ring
    mov dword [pcnet32_private.tx_ring],pcnet32_tx_ring
    ret
pcnet32_reset:
    ; Reset PCNET32
    mov rbp,[io_addr]
    call qword [pcnet32_access.reset]
    ; set 32bit mode
    mov rbx,20
    mov rax,2
    call qword [pcnet32_access.write_bcr]
    ; set/reset autoselect bit
    mov rbx,2
    call qword [pcnet32_access.read_bcr]
    and rax,not 2
    test [pcnet32_private.options],PCNET32_PORT_ASEL
    jz .L1
    or rax,2
.L1:
    call qword [pcnet32_access.write_bcr]
    ; Handle full duplex setting
    cmp byte [pcnet32_private.full_duplex],0
    je .L2
    mov rbx,9
    call qword [pcnet32_access.read_bcr]
    and rax,not 3
    test [pcnet32_private.options],PCNET32_PORT_FD
    jz .L3
    or rax,1
    cmp [pcnet32_private.options],PCNET32_PORT_FD or PCNET32_PORT_AUI
    jne .L4
    or rax,2
    jmp .L4
.L3:
    test [pcnet32_private.options],PCNET32_PORT_ASEL
    jz .L4
    cmp [pcnet32_private.chip_version],0x2627
    jne .L4
    or rax,3
.L4:
    mov rbx,9
    call qword [pcnet32_access.write_bcr]
.L2:
    ; set/reset GPSI bit
    mov rbx,124
    call qword [pcnet32_access.read_csr]
    mov ecx,[pcnet32_private.options]
    and rcx,PCNET32_PORT_PORTSEL
    cmp rcx,PCNET32_PORT_GPSI
    jne .L5
    or rax,0x10
.L5:
    call qword [pcnet32_access.write_csr]
    cmp [pcnet32_private.mii],0
    je .L6
    test [pcnet32_private.options],PCNET32_PORT_ASEL
    jnz .L6
    mov rbx,32
    call qword [pcnet32_access.read_bcr]
    and rax,not 0x38
    test [pcnet32_private.options],PCNET32_PORT_FD
    jz .L7
    or rax,0x10
.L7:
    test [pcnet32_private.options],PCNET32_PORT_100
    jz .L8
    or rax,0x08
.L8:
    call qword [pcnet32_access.write_bcr]
    jmp .L9
.L6:
    test [pcnet32_private.options],PCNET32_PORT_ASEL
    jz .L9
    mov rbx,32
;    PutStr "ASEL, enable auto-negotiation"
    call qword [pcnet32_access.read_bcr]
    and rax,not 0x98
    or rax,0x20
    call qword [pcnet32_access.write_bcr]
.L9:
    cmp [pcnet32_private.ltint],0
    je .L10
    mov rbx,5
    call qword [pcnet32_access.read_csr]
    or rax,(1 shl 14)
    call qword [pcnet32_access.write_csr]
.L10:
    mov eax,[pcnet32_private.options]
    and rax,PCNET32_PORT_PORTSEL
    shl rax,7
    mov [pcnet32_private.mode],ax
    mov [pcnet32_private.filter],dword 0xffffffff
    mov [pcnet32_private.filter+4],dword 0xffffffff
    call pcnet32_init_ring
    mov rbx,1
    mov rax,pcnet32_private
    and rax,0xffff
    call qword [pcnet32_access.write_csr]
    mov rax,pcnet32_private
    mov rbx,2
    shr rax,16
    call qword [pcnet32_access.write_csr]
    mov rbx,4
    mov rax,0x0915
    call qword [pcnet32_access.write_csr]
    mov rbx,0
    mov rax,1
    call qword [pcnet32_access.write_csr]
    mov rcx,100
.L11:
    xor rbx,rbx
    call qword [pcnet32_access.read_csr]
    test ax,0x100
    jnz .L12
    loop .L11
.L12:
;    PutStr "hardware reset"
    xor rbx,rbx
    mov rax,0x0002
    call qword [pcnet32_access.write_csr]
    xor rbx,rbx
    call qword [pcnet32_access.read_csr]
;    PutStr "PCNET reset complete"
    ret
pcnet32_adjust_pci_device:
   ;*******Get current setting************************
   mov     al, 2                                        ;read a word
   mov     bh, [pci_dev]
   mov     ah, [pci_bus]
   mov     bl, 0x04                                 ;from command Register
   call    pci_read_reg
   ;******see if its already set as bus master********
   mov      bx, ax
   and      bx,5
   cmp      bx,5
   je       pcnet32_adjust_pci_device_Latency
   ;******Make card a bus master*******
   mov      cx, ax                              ;value to write
   mov     bh, [pci_dev]
   mov     al, 2                                ;write a word
   or       cx,5
   mov     ah, [pci_bus]
   mov     bl, 0x04                             ;to command register
   call    pci_write_reg
   ;******Check latency setting***********
pcnet32_adjust_pci_device_Latency:
   ;*******Get current latency setting************************
;   mov     al, 1                                       ;read a byte
;   mov     bh, [pci_dev]
;   mov     ah, [pci_bus]
;   mov     bl, 0x0D                                ;from Lantency Timer Register
;   call    pci_read_reg
   ;******see if its aat least 64 clocks********
;   cmp      ax,64
;   jge      pcnet32_adjust_pci_device_Done
   ;******Set latency to 32 clocks*******
;   mov     cx, 64                              ;value to write
;   mov     bh, [pci_dev]
;   mov     al, 1                               ;write a byte
;   mov     ah, [pci_bus]
;   mov     bl, 0x0D                            ;to Lantency Timer Register
;   call    pci_write_reg
   ;******Check latency setting***********
pcnet32_adjust_pci_device_Done:
   ret
pcnet32_probe:
    mov rbp,[io_addr]
    call pcnet32_wio_reset
    xor rbx,rbx
    call pcnet32_wio_read_csr
    cmp rax,4
    jne .try_dwio
    call pcnet32_wio_check
    and al,al
    jz .try_dwio
;    PutStr "Using WIO"
    mov rsi,pcnet32_wio
    jmp .L1
.try_dwio:
    call pcnet32_dwio_reset
    xor rbx,rbx
    call pcnet32_dwio_read_csr
    cmp rax,4
    jne .no_dev
    call pcnet32_dwio_check
    and al,al
    jz .no_dev
;    PutStr "Using DWIO"
    mov rsi,pcnet32_dwio
    jmp .L1
.no_dev:
;    PutStr "PCNET32 not found"
    ret
.L1:
    mov rdi,pcnet32_access
    mov rcx,7
    cld
    rep movsd
    mov rbx,88
    call qword [pcnet32_access.read_csr]
    mov rcx,rax
    mov rbx,89
    call qword [pcnet32_access.read_csr]
    shl rax,16
    or rax,rcx
    mov rcx,rax
    and rcx,0xfff
    cmp rcx,3
    jne .no_dev
    shr rax,12
    and rax,0xffff
    mov [pcnet32_private.chip_version],eax
;    PutStr "PCNET32 chip version OK"
    mov [pcnet32_private.fdx],0
    mov [pcnet32_private.mii],0
    mov [pcnet32_private.fset],0
    mov [pcnet32_private.dxsuflo],0
    mov [pcnet32_private.ltint],0
    mov eax,[pcnet32_private.chip_version]
    cmp rax,0x2420
    je .L2
    cmp rax,0x2430
    je .L3
    cmp rax,0x2621
    je .L4
    cmp rax,0x2623
    je .L5
    cmp rax,0x2624
    je .L6
    cmp rax,0x2625
    je .L7
    cmp rax,0x2626
    je .L8
    cmp rax,0x2627
    je .L9
;    PutStr "Invalid chip rev"
    jmp .no_dev
.L2:
;    PutStr "PCnet/PCI 79C970"
    jmp .L10
.L3:
;    PutStr "PCnet/PCI 79C970"
    jmp .L10
.L4:
;    PutStr "PCnet/PCI II 79C970A"
    mov [pcnet32_private.fdx],1
    jmp .L10
.L5:
;    PutStr "PCnet/FAST 79C971"
    mov [pcnet32_private.fdx],1
    mov [pcnet32_private.mii],1
    mov [pcnet32_private.fset],1
    mov [pcnet32_private.ltint],1
    jmp .L10
.L6:
;    PutStr "PCnet/FAST+ 79C972"
    mov [pcnet32_private.fdx],1
    mov [pcnet32_private.mii],1
    mov [pcnet32_private.fset],1
    jmp .L10
.L7:
;    PutStr "PCnet/FAST III 79C973"
    mov [pcnet32_private.fdx],1
    mov [pcnet32_private.mii],1
    jmp .L10
.L8:
;    PutStr "PCnet/Home 79C978"
    mov [pcnet32_private.fdx],1
    mov rbx,49
    call qword [pcnet32_access.read_bcr]
    call qword [pcnet32_access.write_bcr]
    jmp .L10
.L9:
;    PutStr "PCnet/FAST III 79C975"
    mov [pcnet32_private.fdx],1
    mov [pcnet32_private.mii],1
.L10:
    cmp [pcnet32_private.fset],1
    jne .L11
    mov rbx,18
    call qword [pcnet32_access.read_bcr]
    or rax,0x800
    call qword [pcnet32_access.write_bcr]
    mov rbx,80
    call qword [pcnet32_access.read_csr]
    and rax,0xc00
    or rax,0xc00
    call qword [pcnet32_access.write_csr]
    mov [pcnet32_private.dxsuflo],1
    mov [pcnet32_private.ltint],1
.L11:
    ; read MAC
    mov rdi,node_addr
    mov rdx,rbp
    mov rcx,6
.Lmac:
    in al,dx
    stosb
    inc rdx
    loop .Lmac
;    PutStr "MAC read"
    call pcnet32_adjust_pci_device
;    PutStr "PCI done"
    mov rax,PCNET32_PORT_ASEL
    mov [pcnet32_private.options],eax
    mov [pcnet32_private.mode],word 0x0003
    mov [pcnet32_private.tlen_rlen],word (PCNET32_TX_RING_LEN_BITS or PCNET32_RX_RING_LEN_BITS)
    mov rsi,node_addr
    mov rdi,pcnet32_private.phys_addr
    cld
    movsd
    movsw
    mov [pcnet32_private.filter],dword 0
    mov [pcnet32_private.filter+4],dword 0
    mov dword [pcnet32_private.rx_ring],pcnet32_rx_ring
    mov dword [pcnet32_private.tx_ring],pcnet32_tx_ring
;    PutStr "Switching to 32"
    mov rbx,20
    mov rax,2
    call qword [pcnet32_access.write_bcr]
    mov rbx,1
    mov rax,(pcnet32_private and 0xffff)
    call qword [pcnet32_access.write_csr]
    mov rbx,2
    mov rax,(pcnet32_private shr 16) and 0xffff
    call qword [pcnet32_access.write_csr]
    mov rbx,0
    mov rax,1
    call qword [pcnet32_access.write_csr]
    mov rsi,1
    call delay_ms
    call pcnet32_reset
    mov rax, [pci_data]
    mov [eth_status], eax
    ret
pcnet32_poll:
    xor rax,rax
    mov [eth_rx_data_len],ax
    mov eax,[pcnet32_private.cur_rx]
    and rax,PCNET32_RX_RING_MOD_MASK
    mov rbx,rax
    imul rsi,rax,PCNET32_PKT_BUF_SZ
    add rsi,pcnet32_rxb
    shl rbx,4
    add rbx,pcnet32_rx_ring
    mov cx,[ebx+pcnet32_rx_head.status]
    test cx,0x8000
    jnz .L1
    cmp ch,3
    jne .L1
;    PutStr "PCNETRX"
    mov ecx,[rbx+pcnet32_rx_head.msg_length]
    and rcx,0xfff
    sub rcx,4
    mov [eth_rx_data_len],cx
    push rcx
    shr rcx,2
    mov rdi,Ether_buffer
    cld
    rep movsd
    pop rcx
    and rcx,3
    rep movsb
    mov [rbx+pcnet32_rx_head.buf_length],word PCNET32_PKT_BUF_SZ_NEG
    or [rbx+pcnet32_rx_head.status],word 0x8000
    inc [pcnet32_private.cur_rx]
.L1:
    ret
;         Pointer to 48 bit destination address in edi
;         Type of packet in bx
;         size of packet in ecx
;         pointer to packet data in esi
pcnet32_xmit:
    push rdi
    push rsi
    push rbx
    push rcx
;    PutStr "PCNETTX"
    mov rsi,rdi
    mov edi,[pcnet32_private.cur_tx]
    imul rdi,PCNET32_PKT_BUF_SZ
    add rdi,pcnet32_txb ; edi=ptxb
    mov rax,rdi
    cld         ; copy MAC
    movsd
    movsw
    mov rsi,node_addr
    cld
    movsd
    movsw
    mov [rdi],bx
    add rdi,2
    mov rsi,[rsp+16]
    mov rcx,[rsp]
    push rcx
    shr rcx,2
    cld
    rep movsd
    pop rcx
    and rcx,3
    rep movsb
;    mov ecx,[esp]
;    add ecx,14 ; ETH_HLEN
;    xor eax,eax
; pad to min length (60=ETH_ZLEN)
;    cmp ecx,60
;    jae .L1
;    sub ecx,60
;    cld
;    rep stosb
;.L1:
    mov rdi,pcnet32_tx_ring+0   ; entry=0
    mov rcx,[rsp]
    add rcx,14
    cmp cx,60
    jae .L1
    mov cx,60
.L1:
    neg cx
    mov [rdi+pcnet32_tx_head.length],cx
    mov [rdi+pcnet32_tx_head.misc],dword 0
    mov [rdi+pcnet32_tx_head.base],eax
    mov [rdi+pcnet32_tx_head.status],word 0x8300
    ; trigger an immediate send poll
    mov rbx,0
    mov rax,0x0008      ; 0x0048
    mov rbp,[io_addr]
    call qword [pcnet32_access.write_csr]
    mov dword [pcnet32_private.cur_tx],0
    ; wait for TX to complete
    mov rcx,[0xfdf0]
    add rcx,100
.L2:
    mov ax,[rdi+pcnet32_tx_head.status]
    test ax,0x8000
    jz .L3
    cmp rcx,[0xfdf0]
    jb .L4
    mov rsi,10
    call delay_ms
    jnz .L2
.L4:
;    PutStr "PCNET: Send timeout"
.L3:
    mov dword [rdi+pcnet32_tx_head.base],0
    pop rcx
    pop rbx
    pop rsi
    pop rdi
    ret
