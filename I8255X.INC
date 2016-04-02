;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                 ;;
;;  I8255X.INC                                                     ;;
;;                                                                 ;;
;;  Ethernet driver for Tensor OS                                  ;;
;;                                                                 ;;
;;  Version 0.3  11 August 2013                                    ;;
;;                                                                 ;;
;;  This driver is based on the eepro100 driver from               ;;
;;  the etherboot 5.0.6 project. The copyright statement is        ;;
;;                                                                 ;;
;;          GNU GENERAL PUBLIC LICENSE                             ;;
;;             Version 2, June 1991                                ;;
;;                                                                 ;;
;;  remaining parts Copyright 2012 Marco K,                        ;;
;;   oldklickers@gmail.com                                         ;;
;;                                                                 ;;
;;  See file COPYING for details                                   ;;
;;                                                                 ;;
;;                                                                 ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;********************************************************************
;   Interface
;      I8255x_reset
;      I8255x_probe
;      I8255x_poll
;      I8255x_transmit
;
;      These functions are referenced in ethernet.inc
;
;********************************************************************


rxfd_status             equ  eth_data_start
rxfd_command            equ  eth_data_start + 2
rxfd_link               equ  eth_data_start + 4
rxfd_rx_buf_addr        equ  eth_data_start + 8
rxfd_count              equ  eth_data_start + 12
rxfd_size               equ  eth_data_start + 14
rxfd_packet             equ  eth_data_start + 16




eeprom_data:  times 16  dd 0

align 4

lstats:
tx_good_frames: dd 0
tx_coll16_errs:  dd 0
tx_late_colls: dd 0
tx_underruns: dd 0
tx_lost_carrier: dd 0
tx_deferred: dd 0
tx_one_colls: dd 0
tx_multi_colls: dd 0
tx_total_colls: dd 0
rx_good_frames: dd 0
rx_crc_errs: dd 0
rx_align_errs: dd 0
rx_resource_errs: dd 0
rx_overrun_errs: dd 0
rx_colls_errs: dd 0
rx_runt_errs: dd 0
done_marker: dd 0

align 4

confcmd:
confcmd_status:         dw    0
confcmd_command:        dw    0
confcmd_link:           dd    0
confcmd_data:           db    22, 0x08, 0, 0, 0, 0x80, 0x32, 0x03, 1
                        db    0, 0x2e, 0, 0x60, 0, 0xf2, 0x48, 0, 0x40, 0xf2
                        db    0x80, 0x3f, 0x05

align 4

txfd:
txfd_status:            dw   0
txfd_command:           dw   0
txfd_link:              dd   0
txfd_tx_desc_addr:      dd   0
txfd_count:             dd   0
txfd_tx_buf_addr0:      dd   0
txfd_tx_buf_size0:      dd   0
txfd_tx_buf_addr1:      dd   0
txfd_tx_buf_size1:      dd   0

align 4

hdr:
hdr_dst_addr:           times 6 db 0
hdr_src_addr:           times 6 db 0
hdr_type:               dw   0



;***************************************************************************
;   Function
;      wait_for_cmd_done
;
;   Description
;       waits for the hardware to complete a command
;       port address in edx
;
;       al destroyed
;***************************************************************************
wait_for_cmd_done:
        in       al,dx
        cmp      al,0
        jne      wait_for_cmd_done
        ret



;***************************************************************************
;   Function
;      mdio_read
;
;   Description
;       This probably reads a register in the "physical media interface chip"
;         Phy_id in ebx
;         location in ecx
;
;       Data returned in eax
;
;***************************************************************************
mdio_read:
        mov      rdx,[io_addr]
        add      rdx,16         ; SCBCtrlMDI

        mov      rax,0x08000000
        shl      rcx,16
        or       rax,rcx
        shl      rbx,21
        or       rax,rbx

        out      dx,eax

mrlp:
        call     delay_us
        in       eax,dx
        mov      rcx,rax
        and      rcx,0x10000000
        jz       mrlp

        mov      r8,0xffff
        and      rax,r8
        ret



;***************************************************************************
;   Function
;      mdio_write
;
;   Description
;       This probably writes a register in the "physical media interface chip"
;         Phy_id in ebx
;         location in ecx
;         data in edx
;       Data returned in eax
;
;***************************************************************************
mdio_write:
        mov      rax,0x04000000
        shl      rcx,16
        or       rax,rcx
        shl      rbx,21
        or       rax,rbx
        or       rax,rdx

        mov      rdx,[io_addr]
        add      rdx,16         ; SCBCtrlMDI
        out      dx,eax

mwlp:
        call     delay_us
        in       eax,dx
        mov      rcx,rax
        and      rcx,0x10000000
        jz       mwlp

        mov      r8,0xffff
        and      rax,r8
        ret



;/***********************************************************************/
;/*                       I82557 related defines                        */
;/***********************************************************************/

; Serial EEPROM section.
;   A "bit" grungy, but we work our way through bit-by-bit :->.
;  EEPROM_Ctrl bits.
EE_SHIFT_CLK    equ   0x01    ; EEPROM shift clock.
EE_CS           equ   0x02    ; EEPROM chip select.
EE_DATA_WRITE   equ   0x04    ; EEPROM chip data in.
EE_DATA_READ    equ   0x08    ; EEPROM chip data out.
EE_WRITE_0      equ   0x4802
EE_WRITE_1      equ   0x4806
EE_ENB          equ   0x4802


; The EEPROM commands include the alway-set leading bit.
EE_READ_CMD     equ   6

; The SCB accepts the following controls for the Tx and Rx units:
CU_START       equ   0x0010
CU_RESUME      equ   0x0020
CU_STATSADDR   equ   0x0040
CU_SHOWSTATS   equ   0x0050   ; Dump statistics counters.
CU_CMD_BASE    equ   0x0060   ; Base address to add to add CU commands.
CU_DUMPSTATS   equ   0x0070   ; Dump then reset stats counters.

RX_START       equ   0x0001
RX_RESUME      equ   0x0002
RX_ABORT       equ   0x0004
RX_ADDR_LOAD   equ   0x0006
RX_RESUMENR    equ   0x0007
INT_MASK       equ   0x0100
DRVR_INT       equ   0x0200   ; Driver generated interrupt.


;***************************************************************************
;   Function
;      do_eeprom_cmd
;
;   Description
;       writes a cmd to the ethernet cards eeprom, by bit bashing
;       cmd in ebx
;       cmd length in ecx
;       return in eax
;***************************************************************************
do_eeprom_cmd:
        mov      rdx,[io_addr]    ; We only require the value in dx
        add      dx,14            ; the value SCBeeprom

        mov      ax,EE_ENB
        out      dx,ax
        call     delay_us

        mov      ax,0x4803        ; EE_ENB | EE_SHIFT_CLK
        out      dx,ax
        call     delay_us

        ; dx holds ee_addr
        ; rcx holds count
        ; rax holds cmd
        xor      rdi,rdi          ; this will be the receive data

dec_001:
        mov      rsi,1

        dec      rcx
        shl      rsi,cl
        inc      rcx
        and      rsi,rbx
        mov      rax,EE_WRITE_0   ; I am assuming this doesnt affect the flags..
        cmp      rsi,0
        jz       dec_002
        mov      rax,EE_WRITE_1

dec_002:
        out      dx,ax
        call     delay_us

        or       ax,EE_SHIFT_CLK
        out      dx,ax
        call     delay_us

        shl      rdi,1

        in       ax,dx
        and      ax,EE_DATA_READ
        cmp      ax,0
        jz       dec_003
        inc      rdi

dec_003:
        loop     dec_001

        mov      ax,EE_ENB
        out      dx,ax
        call     delay_us

        mov      ax,0x4800
        out      dx,ax
        call     delay_us

        mov      rax,rdi

        ret



;***************************************************************************
;   Function
;      I8255x_reset
;   Description
;      Place the chip (ie, the ethernet card) into a virgin state
;      No inputs
;      All registers destroyed
;
;***************************************************************************
I8255x_reset:
        ret



;***************************************************************************
;   Function
;      I8255x_probe
;   Description
;      Searches for an ethernet card, enables it and clears the rx buffer
;      If a card was found, it enables the ethernet -> TCPIP link
;
;***************************************************************************
I8255x_probe:
        mov      rax,[io_addr]

        mov      rbx,[pci_bus]
        mov      rcx,[pci_dev]
        mov      rdx,0x04      ; PCI_COMMAND
        call     pcibios_read_config_word

        or       ax,0x05
        mov      rbx,[pci_bus]
        mov      rcx,[pci_dev]
        mov      rdx,0x04      ; PCI_COMMAND
        call     pcibios_write_config_word

        mov      rbx,0x6000000
        mov      rcx,27
        call     do_eeprom_cmd
        and      rax,0xffe0000
        cmp      rax,0xffe0000
        je       bige

        mov      rbx,0x1800000
        mov      rcx,0x40
        jmp      doread

bige:
        mov      rbx,0x6000000
        mov      rcx,0x100

doread:
        ; do-eeprom-cmd will destroy all registers
        ; we have eesize in ecx
        ; read_cmd in ebx

        ; Ignore full eeprom - just load the mac address
        mov      rcx, 0

drlp:
        push     rcx      ; save count
        push     rbx
        mov      rax,rcx
        shl      rax,16
        or       rbx,rax
        mov      rcx,27
        call     do_eeprom_cmd

        pop      rbx
        pop      rcx

        mov      rdx,rcx
        shl      rdx,2
        mov      rsi,eeprom_data
        add      rsi,rdx
        mov      [rsi],rax

        inc      rcx
        cmp      rcx,16
        jne      drlp

        ; OK, we have the MAC address.
        ; Now reset the card

        mov      rdx,[io_addr]
        add      dx,8         ; SCBPort
        xor      rax,rax      ; The reset cmd == 0
        out      dx,eax

        mov      rsi,10
        call     delay_ms      ; Give the card time to warm up.

        mov      rax,lstats
        mov      rdx,[io_addr]
        add      rdx,4            ; SCBPointer
        out      dx,eax

        mov      rax,0x0140         ; INT_MASK | CU_STATSADDR
        mov      rdx,[io_addr]
        add      rdx,2            ; SCBCmd
        out      dx,ax

        call     wait_for_cmd_done

        mov      rax,0
        mov      rdx,[io_addr]
        add      rdx,4            ; SCBPointer
        out      dx,eax

        mov      rax,0x0106         ; INT_MASK | RX_ADDR_LOAD
        mov      rdx,[io_addr]
        add      rdx,2            ; SCBCmd
        out      dx,ax

        call     wait_for_cmd_done

        ; build rxrd structure
        mov      ax,0x0001
        mov      [rxfd_status],ax
        mov      ax,0x0000
        mov      [rxfd_command],ax

        mov      rax,rxfd_status
        mov      [rxfd_link],eax

        mov      rax,Ether_buffer
        mov      [rxfd_rx_buf_addr],eax

        mov      ax,0
        mov      [rxfd_count],ax

        mov      ax,1528
        mov      [rxfd_size],ax

        mov      rdx,[io_addr]
        add      rdx,4           ; SCBPointer

        mov      rax,rxfd_status
        out      dx,eax

        mov      rdx,[io_addr]
        add      rdx,2           ; SCBCmd

        mov      ax,0x0101         ; INT_MASK | RX_START
        out      dx,ax

        call     wait_for_cmd_done

        ; start the reciver

        mov      ax,0
        mov      [rxfd_status],ax

        mov      ax,0xc000
        mov      [rxfd_command],ax

        mov      rdx,[io_addr]
        add      rdx,4           ; SCBPointer

        mov      rax,rxfd_status
        out      dx,eax

        mov      rdx,[io_addr]
        add      rdx,2           ; SCBCmd

        mov      ax,0x0101         ; INT_MASK | RX_START
        out      dx,ax

        ; Init TX Stuff

        mov      rdx,[io_addr]
        add      rdx,4           ; SCBPointer

        mov      rax,0
        out      dx,eax

        mov      edx,[io_addr]
        add      edx,2           ; SCBCmd

        mov      ax,0x0160         ; INT_MASK | CU_CMD_BASE
        out      dx,ax

        call     wait_for_cmd_done

        ; Set TX Base address

        ; First, set up confcmd values

        mov      ax,2
        mov      [confcmd_command],ax
        mov      rax,txfd
        mov      [confcmd_link],eax

        mov      ax,1
        mov      [txfd_command],ax         ; CmdIASetup

        mov      ax,0
        mov      [txfd_status],ax

        mov      rax,confcmd
        mov      [txfd_link],eax

        ; ETH_ALEN is 6 bytes

        mov      rsi,eeprom_data
        mov      rdi,node_addr
        mov      rcx,3
drp000:
        mov      rax,[rsi]
        mov      [rdi],al
        shr      rax,8
        inc      rdi
        mov      [rdi],al
        inc      rdi
        add      rsi,4
        loop     drp000

        ; Hard code your MAC address into node_addr at this point,
        ; If you cannot read the MAC address from the eeprom in the previous step.
        ; You also have to write the mac address into txfd_tx_desc_addr, rather
        ; than taking data from eeprom_data

        mov      rsi,eeprom_data
        mov      rdi,txfd_tx_desc_addr
        mov      rcx,3
drp001:
        mov      rax,[rsi]
        mov      [rdi],al
        shr      rax,8
        inc      rdi
        mov      [rdi],al
        inc      rdi
        add      rsi,4
        loop     drp001

        mov      rsi,eeprom_data + (6 * 4)
        mov      rax,[rsi]
        shr      rax,8
        and      rax,0x3f
        cmp      rax,4            ; DP83840
        je       drp002
        cmp      rax,10            ; DP83840A
        je       drp002
        jmp      drp003

drp002:
        mov      rbx,[rsi]
        and      rbx,0x1f
        push     rbx
        mov      rcx,23
        call     mdio_read
        pop      rbx
        or       rax,0x0422
        mov      rcx,23
        mov      rdx,rax
        call     mdio_write

drp003:
        mov      ax,0x4002         ; Cmdsuspend | CmdConfigure
        mov      [confcmd_command],ax
        mov      ax,0
        mov      [confcmd_status],ax
        mov      rax,txfd
        mov      [confcmd_link],eax
        mov      rbx,confcmd_data
        mov      al,0x88         ; fifo of 8 each
        mov      [rbx + 1],al
        mov      al,0
        mov      [rbx + 4],al
        mov      al,0x80
        mov      [rbx + 5],al
        mov      al,0x48
        mov      [rbx + 15],al
        mov      al,0x80
        mov      [rbx + 19],al
        mov      al,0x05
        mov      [rbx + 21],al

        mov      rax,txfd
        mov      rdx,[io_addr]
        add      rdx,4            ; SCBPointer
        out      dx,eax

        mov      rax,0x0110         ; INT_MASK | CU_START
        mov      rdx,[io_addr]
        add      rdx,2            ; SCBCmd
        out      dx,ax

        call     wait_for_cmd_done
        jmp      skip

        ; wait for thing to start
drp004:
        mov      ax,[txfd_status]
        cmp      ax,0
        je       drp004

skip:
        ; Indicate that we have successfully reset the card
        mov      rax,[pci_data]
        mov      [eth_status],eax

I8255x_exit:
        ret



;***************************************************************************
; Function
;    I8255x_poll
;
; Description
;    Polls the ethernet card for a received packet
;    Received data, if any, ends up in Ether_buffer
;
;***************************************************************************
I8255x_poll:
        mov      ax,0      ; assume no data
        mov      [eth_rx_data_len],ax

        mov      ax,[rxfd_status]
        cmp      ax,0
        je       i8p_exit

        mov      ax,0
        mov      [rxfd_status],ax

        mov      ax,0xc000
        mov      [rxfd_command],ax

        mov      rdx,[io_addr]
        add      rdx,4           ; SCBPointer

        mov      rax,rxfd_status
        out      dx,eax

        mov      rdx,[io_addr]
        add      rdx,2           ; SCBCmd

        mov      ax,0x0101         ; INT_MASK | RX_START
        out      dx,ax

        call     wait_for_cmd_done

        mov      rsi,rxfd_packet
        mov      rdi,Ether_buffer
        mov      rcx,1518
        cld
        rep      movsb

        mov      ax,[rxfd_count]
        and      ax,0x3fff
        mov      [eth_rx_data_len],ax

i8p_exit:
        ret



;***************************************************************************
;   Function
;      I8255x_transmit
;
;   Description
;       Transmits a packet of data via the ethernet card
;          Pointer to 48 bit destination address in rdi
;         Type of packet in bx
;         size of packet in rcx
;         pointer to packet data in rsi
;
;***************************************************************************
I8255x_transmit:

        mov      [hdr_type],bx

        mov      rax,[rdi]
        mov      [hdr_dst_addr],eax
        mov      ax,[rdi+4]
        mov      [hdr_dst_addr+4],ax

        mov      rax,[node_addr]
        mov      [hdr_src_addr],rax
        mov      ax,[node_addr+4]
        mov      [hdr_src_addr+4],ax

        mov      rdx,[io_addr]
        in       ax,dx
        and      ax,0xfc00
        out      dx,ax

        xor      ax,ax
        mov      [txfd_status],ax
        mov      ax,0x400C            ; Cmdsuspend | CmdTx | CmdTxFlex
        mov      [txfd_command],ax
        mov      eax,txfd
        mov      [txfd_link],eax
        mov      eax,0x02208000
        mov      [txfd_count],eax
        mov      rax,txfd_tx_buf_addr0
        mov      [txfd_tx_desc_addr],eax
        mov      rax,hdr
        mov      [txfd_tx_buf_addr0],eax
        mov      rax,14   ; sizeof hdr
        mov      [txfd_tx_buf_size0],eax

        ; Copy the buffer address and size in
        mov      rax,rsi
        mov      [txfd_tx_buf_addr1],eax
        mov      rax,rcx
        mov      [txfd_tx_buf_size1],eax

        mov      rax,txfd
        mov      rdx,[io_addr]
        add      rdx,4            ; SCBPointer
        out      dx,eax

        mov      ax,0x0110         ; INT_MASK | CU_START
        mov      rdx,[io_addr]
        add      rdx,2            ; SCBCmd
        out      dx,ax

        call     wait_for_cmd_done

        mov      rdx,[io_addr]
        in       ax,dx

I8t_001:
        mov      ax,[txfd_status]
        cmp      ax,0
        je       I8t_001

        mov      rdx,[io_addr]
        in       ax,dx

        ret


