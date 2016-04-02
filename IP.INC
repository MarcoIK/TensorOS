;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                 ;;
;;  IP.INC                                                         ;;
;;                                                                 ;;
;;  IP Processes for Tensor OS  TCP/IP stack                       ;;
;;                                                                 ;;
;;  Version 0.3  29 August 2012                                    ;;
;;                                                                 ;;
;;  Copyright 2012 Marco K, oldklickers@gmail.com                  ;;
;;                                                                 ;;
;;  See file COPYING for details                                   ;;
;;                                                                 ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;*******************************************************************
;   Interface
;
;       ip_rx       processes all packets received by the network layer
;                   It calls the appropriate protocol handler
;
;
;
;*******************************************************************


;***************************************************************************
;   Function
;      ip_rx
;
;   Description
;       Handles received IP packets
;       This is a kernel function, called by stack_handler
;
;***************************************************************************
ip_rx:
        ; Look for a buffer to tx
        mov     rax,IPIN_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      ipr_exit         ; Exit if no buffer available

        push    rax

        ; convert buffer pointer eax to the absolute address
        mov     rcx,IPBUFFSIZE
        mul     rcx
        add     rax,IPbuffs

        mov     rdx,rax  ; Save the address in  rdx for use by future processes

        ; Validate the IP checksum
        mov     rbx,rdx
        mov     ah,[rbx + 10]
        mov     al,[rbx + 11]      ; Get the checksum in intel format
        mov     [rbx + 10],word 0  ; clear checksum field - need to when
                                    ; recalculating checksum

        ;  this needs two data pointers and two size #.
        ;  2nd pointer can be of length 0
        mov     rbx,rdx
        mov     [checkAdd1],rbx
        mov     [checkSize1],word 20
        mov     [checkAdd2],dword 0
        mov     [checkSize2],word 0

        call    checksum           ; Recalculate IP checksum
        cmp     ax,[checkResult]
        jnz     ipr_dump

        ; If the IP address is 255.255.255.255, accept it
        ; - it is a broadcast packet, which we need for dhcp
        mov     rax,[rdx + 16]
        mov     r8,0xffffffff
        cmp     rax,r8
        je      ipr_p0

        ; Validate the IP address, if it isn't broadcast
        cmp     rax,[stack_ip]
        jnz     ipr_dump

ipr_p0:
        mov     al,[rdx]
        and     al,0x0f
        cmp     al,0x05
        jnz     ipr_dump

        cmp     [rdx+8],byte 0
        jz      ipr_dump

        mov     ax,[rdx + 6]
        and     ax,0xFFBF
        cmp     ax,0
        jnz     ipr_dump

        ; Check the protocol, and call the appropriate handler
        ; Each handler will re-use or free the queue buffer as appropriate
        mov     al,[rdx + 9]
        cmp     al,PROTOCOL_ICMP
        jnz     ipr_p1
        pop     rax
        call    icmp_rx
        jmp     ipr_exit

ipr_p1:
        cmp     al,PROTOCOL_TCP
        jnz     ipr_p2
        pop     rax
        call    tcp_rx
        jmp     ipr_exit

ipr_p2:
        cmp     al,PROTOCOL_UDP
        jnz     ipr_dump
        pop     rax
        call    udp_rx
        jmp     ipr_exit

ipr_dump:
        ; No protocol handler available, so
        ; silently dump the packet, freeing up the queue buffer

;    inc     dword [dumped_rx_count]

        pop     rax
        call    freeBuff

ipr_exit:
        ret



;***************************************************************************
;   Function
;      icmp_rx
;
;   Description
;       ICMP protocol handler
;       This is a kernel function, called by ip_rx
;       edx contains the address of the buffer in use.
;       This buffer must be reused or marked as empty afterwards
;
;***************************************************************************
icmp_rx:
        cmp     [rdx + 20],byte 8 ; Is this an echo request? discard if not
        jz      icmp_echo

        call    freeBuff
        jmp     icmp_exit

icmp_echo:
        push    rax
        mov     [rdx + 10],word 0  ; I think this was already done by IP rx

        ; swap the source and destination addresses
        mov     rcx,[rdx + 16]
        mov     rax,[rdx + 12]
        mov     [rdx + 16],eax
        mov     [rdx + 12],ecx

        ; recaluculate the IP header checksum

        mov     rbx,rdx
        mov     [checkAdd1],ebx
        mov     [checkSize1],word 20
        mov     [checkAdd2],dword 0
        mov     [checkSize2],word 0

        call    checksum
        mov     ax,[checkResult]
        mov     [rdx + 10],ah
        mov     [rdx + 11],al      ; ?? correct byte order?

        mov     [rdx + 20],byte 0  ; change the request to a response
        mov     [rdx + 22],word 0  ; clear ICMP checksum prior to re-calc

        ; Calculate the length of the ICMP data ( IP payload)
        mov     ah,[rdx + 2]
        mov     al,[rdx + 3]
        sub     ax,20

        mov     [checkSize1],ax
        mov     rbx,rdx
        add     rbx,20

        mov     [checkAdd1],ebx
        mov     [checkAdd2],dword 0
        mov     [checkSize2],word 0

        call    checksum

        mov     ax,[checkResult]
        mov     [edx + 22],ah
        mov     [edx + 23],al

        ; Queue packet for transmission

        pop     rbx
        mov     rax,NET1OUT_QUEUE
        call    queue

icmp_exit:
        ret

