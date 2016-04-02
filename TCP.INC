;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                 ;;
;;  TCP.INC                                                        ;;
;;                                                                 ;;
;;  TCP Processes for Tensor OS  TCP/IP stack                      ;;
;;                                                                 ;;
;;  Version 0.6  4th July 2014                                     ;;
;;                                                                 ;;
;;  Copyright 2012 Marco K, oldklickers@gmail.com                  ;;
;;                                                                 ;;
;;  See file COPYING for details                                   ;;
;;  v0.6 : Added reset handling in the established state           ;;
;;         Added a timer per socket to allow delays when rx window ;;
;;         gets below 1KB                                          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;*******************************************************************
;   Interface
;
;       tcp_tx_handler      Handles the TCP transmit queue
;       tcp_rx              The protocol handler for received data
;       buildTCPPacket      fills in the packet headers and data
;       tcpStateMachine     Main state machine for received TCP packets
;       tcp_tcb_handler     1s timer, to erase tcb's in TIME_WAIT state
;
;*******************************************************************



;***************************************************************************
;   Function
;      tcp_tcb_handler
;
;   Description
;       Handles sockets in the timewait state, closing them
;       when the TCB timer expires
;
;***************************************************************************
tcp_tcb_handler:
    ; scan through all the sockets, decrementing active timers

        mov     rax,SOCKETBUFFSIZE * NUM_SOCKETS
        mov     rcx,NUM_SOCKETS



tth1:
        sub     rax,SOCKETBUFFSIZE
        cmp     [rax + sockets + 32],dword 0
        jne     tth2

tth1a:
        cmp     [rax + sockets + 72],dword 0
        jne     tth4

        loop    tth1
        ret

tth3:
        popdu

        jmp     tth1a

        loop    tth1
        ret

        ; TODO - prove it works!

tth4:
        dec     dword [rax + sockets + 72]
        loop    tth1
        ret

tth2:
        ; decrement it, delete socket if TCB timer = 0 & socket in timewait state
        pushdu
        dec     dword [rax + sockets + 32]
        cmp     [rax + sockets + 32],dword 0
        jne     tth3

        cmp     [rax + sockets + 28],dword TCB_TIME_WAIT
        jne     tth3

        ; OK, delete socket
        mov     rdi,rax
        add     rdi,sockets

        xor     rax,rax
        mov     rcx,SOCKETHEADERSIZE
        cld
        rep     stosb





tth_exit:
        ret


;***************************************************************************
;   Function
;      tcp_tx_handler
;
;   Description
;       Handles queued TCP data
;       This is a kernel function, called by stack_handler
;
;***************************************************************************
tcp_tx_handler:
        ; decrement all resend buffers timers. If they
        ; expire, queue them for sending, and restart the timer.
        ; If the retries counter reach 0, delete the entry

        mov     rsi,resendQ
        mov     rcx,0

tth001:
        cmp     rcx,NUMRESENDENTRIES
        je      tth003              ; None left
        cmp     [rsi],byte 0xFF
        jne     tth002             ; found one
        inc     rcx
        add     rsi,4
        jmp     tth001

tth002:
        ; we have one. decrement it's timer by 1
        dec     word [rsi+2]
        mov     ax,[rsi+2]
        cmp     ax, 0
        je      tth002a
        inc     rcx
        add     rsi,4
        jmp     tth001              ; Timer not zero, so move on

tth002a:
        mov     bl,0xff
        ; restart timer, and decrement retries
        ; After the first resend, back of on next, by a factor of 5
        mov     [rsi+2],word TCP_TIMEOUT * 5
        dec     byte [rsi+1]
        mov     al,[rsi+1]
        cmp     al,0
        jne     tth004

        ; retries now 0, so delete from queue
        xchg    [rsi],bl
tth004:

        ; resend packet
        pushdu

        mov     rax,EMPTY_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        jne     tth004z

        ; TODO - try again in 10ms.
        cmp     bl,0xff
        jne     tth004za
        mov     [rsi], bl

tth004za:
        ; Mark it to expire in 10ms - 1 tick
        mov     [rsi+1],byte 1
        mov     [rsi+2],word 1
        jmp     tth005

tth004z:
        ; we have a buffer # in ax

        push    rax
        push    rcx
        mov     rcx,IPBUFFSIZE
        mul     rcx
        add     rax,IPbuffs

        ; we have the buffer address in eax
        mov     rdi,rax
        pop     rcx
        ; get resend data address
        inc     rcx
        ; Now get buffer location, and copy buffer across. argh! more copying,,
        mov     rsi,resendBuffer - IPBUFFSIZE
tth004a:
        add     rsi,IPBUFFSIZE
        loop    tth004a

        ; we have resend buffer location in esi
        mov     rcx,IPBUFFSIZE

        ; copy data across
        cld
        rep     movsb

        ; queue packet



        mov     rax,NET1OUT_QUEUE

        mov     rdx,[stack_ip]
        mov     rcx,[ rdi + 16 ]
        cmp     rdx,rcx
        jne     tth004b
        mov     rax,IPIN_QUEUE

tth004b:
        pop     rbx

        call    queue


tth005:
        popdu

        inc     rcx
        add     rsi,4
        jmp     tth001

tth003:
        ret




;***************************************************************************
;   Function
;      tcp_rx
;
;   Description
;       TCP protocol handler
;       This is a kernel function, called by ip_rx
;       IP buffer address given in rdx
;          IP buffer number in rax
;          Free up (or re-use) IP buffer when finished
;
;***************************************************************************
tcp_rx:
        ; The process is as follows.
        ; Look for a socket with matching remote IP, remote port, local port
        ; if not found, then
        ; look for remote IP + local port match ( where sockets remote port = 0)
        ; if not found, then
        ; look for a socket where local socket port == IP packets remote port
        ; where sockets remote port, remote IP = 0
        ; discard if not found
        ; Call sockets tcbStateMachine, with pointer to packet.
        ; the state machine will not delete the packet, so do that here.

        push    rax

        ; Look for a socket where
        ; IP Packet TCP Destination Port = local Port
        ; IP Packet SA = Remote IP
        ; IP Packet TCP Source Port = remote Port

        mov     rax,SOCKETBUFFSIZE * NUM_SOCKETS
        mov     rcx,NUM_SOCKETS
ss1:
        sub     rax,SOCKETBUFFSIZE
        movzx   rbx,word [rdx + 22]     ; get the dest. port from the TCP hdr
        cmp     [eax + sockets + 12],bx ; compare with socket's local port
        jnz     nxttst1                        ; different - try next socket

        movzx   rbx,word [rdx + 20]       ; get the source port from the TCP hdr
        cmp     [rax + sockets + 20],bx ; compare with socket's remote port
        jnz     nxttst1                        ; different - try next socket


        mov     rbx,[rdx + 12]           ; get the source IP Addr from the IP hdr
        cmp     [rax + sockets + 16],ebx ; compare with socket's remote IP
        jnz     nxttst1                        ; different - try next socket

        ; We have a complete match - use this socket
        jmp     tcprx_001

nxttst1:
        loop    ss1                     ; Return back if no match

        ; If we got here, there was no match
        ; Look for a socket where
        ; IP Packet TCP Destination Port = local Port
        ; IP Packet SA = Remote IP
        ; socket remote Port = 0

        mov     rax,SOCKETBUFFSIZE * NUM_SOCKETS
        mov     rcx,NUM_SOCKETS

ss2:
        sub     rax,SOCKETBUFFSIZE

        movzx   rbx,word [rdx + 22]     ; get the dest. port from the TCP hdr
        cmp     [rax + sockets + 12],bx ; compare with socket's local port
        jnz     nxttst2                        ; different - try next socket

        mov     rbx,[rdx + 12]          ; get the source IP Addr from the IP hdr
        cmp     [eax + sockets + 16],ebx ; compare with socket's remote IP
        jnz     nxttst2                        ; different - try next socket

        mov     rbx,0
        cmp     [rax + sockets + 20],bx ; only match a remote socket of 0
        jnz     nxttst2                        ; different - try next socket

        ; We have a complete match - use this socket
        jmp     tcprx_001

nxttst2:
        loop    ss2                     ; Return back if no match

        ; If we got here, there was no match
        ; Look for a socket where
        ; IP Packet TCP Destination Port = local Port
        ; socket Remote IP = 0
        ; socket remote Port = 0

        mov     rax,SOCKETBUFFSIZE * NUM_SOCKETS
        mov     rcx,NUM_SOCKETS

ss3:
        sub     rax,SOCKETBUFFSIZE

        movzx   rbx,word [rdx + 22]     ; get destination port from the TCP hdr
        cmp     [rax + sockets + 12],bx ; compare with socket's local port
        jnz     nxttst3                        ; different - try next socket

        mov     rbx,0
        cmp     [rax + sockets + 20],bx ; only match a remote socket of 0
        jnz     nxttst3                        ; different - try next socket

        mov     rbx,0
        cmp     [rax + sockets + 16],ebx ; only match a socket remote IP of 0
        jnz     nxttst3                        ; different - try next socket

        ; We have a complete match - use this socket
        jmp     tcprx_001

nxttst3:
        loop    ss3                     ; Return back if no match

        ; If we got here, we need to reject the packet
        inc     dword [dumped_rx_count]
        jmp     tcprx_exit

tcprx_001:
        ; We have a valid socket/TCB, so call the TCB State Machine for that skt.
        ; socket is pointed to by [eax + sockets]
        ; IP packet is pointed to by [edx]
        ; IP buffer number is on stack ( it will be popped at the end)
        call    tcpStateMachine

tcprx_exit:
        pop     rax
        call    freeBuff

        ret



;***************************************************************************
;   Function
;      buildTCPPacket
;
;   Description
;       builds an IP Packet with TCP data fully populated for transmission
;       You may destroy any and all registers
;          TCP control flags specified in bl
;          This TCB is in [sktAddr]
;          User data pointed to by rsi
;       Data length in rcx
;          Transmit buffer number in rax
;
;***************************************************************************
buildTCPPacket:
        push    rcx                        ; Save data length

        ; convert buffer pointer eax to the absolute address
        mov     rcx,IPBUFFSIZE
        mul     rcx
        add     rax,IPbuffs

        mov     rdx,rax

        mov     [rdx + 33],bl            ; TCP flags

        mov     rbx,[sktAddr]

        ; So, ebx holds the socket ptr, edx holds the IPbuffer ptr

        ; Fill in the IP header ( some data is in the socket descriptor)
        mov     rax,[ebx + 8]
        mov     [rdx + 12],eax      ; source IP
        mov     rax,[ebx + 16]
        mov     [edx + 16],eax      ; Destination IP

        mov     al,0x45
        mov     [rdx],al         ; Version, IHL
        xor     al,al
        mov     [rdx + 1],al     ; Type of service

        pop     rax                   ; Get the TCP data length
        push    rax

        add     rax,20 + 20           ; add IP header and TCP header lengths
        mov     [rdx + 2],ah
        mov     [rdx + 3],al
        xor     al,al
        mov     [rdx + 4],al
        mov     [rdx + 5],al
        mov     al,0x40
        mov     [rdx + 6],al
        xor     al,al
        mov     [rdx + 7],al
        mov     al,0x20
        mov     [rdx + 8],al
        mov     al,6                         ; TCP protocol
        mov     [rdx + 9],al

        ; Checksum left unfilled
        xor     ax,ax
        mov     [rdx + 10],ax

        ; Fill in the TCP header ( some data is in the socket descriptor)
        mov     ax,[rbx + 12]
        mov     [rdx + 20],ax        ; Local Port

        mov     ax,[rbx + 20]
        mov     [rdx + 20 + 2],ax    ; desitination Port

        ; Checksum left unfilled
        xor     ax,ax
        mov     [rdx + 20 + 16],ax

        ; sequence number
        mov     rax,[rbx + 48]
        mov     [rdx + 20 + 4],eax

        ; ack number
        mov     rax,[rbx + 56]
        mov     [rdx + 20 + 8],eax

        ; window ( 0x2000 is default ).I could accept 4KB, fa0, ( skt buffer size)
        ; 768 bytes seems better
        mov     ax,0x0003
        mov     [rdx + 20 + 14],ax

        ; Urgent pointer (0)
        mov     ax,0
        mov     [rdx + 20 + 18],ax

        ; data offset ( 0x50 )
        mov     al,0x50
        mov     [rdx + 20 + 12],al

        pop     rcx                  ; count of bytes to send
        mov     rbx,rcx            ; need the length later

        cmp     rbx,0
        jz      btp_001

        mov     rdi,rdx
        add     rdi,40
        cld
        rep     movsb               ; copy the data across

btp_001:
        ; we have rdx as IPbuffer ptr.
        ; Fill in the TCP checksum
        ; First, fill in pseudoheader
        mov     rax,[rdx + 12]
        mov     [pseudoHeader],eax
        mov     rax,[rdx + 16]
        mov     [pseudoHeader+4],eax
        mov     ax,0x0600            ; 0 + protocol
        mov     [pseudoHeader+8],ax
        add     rbx,20
        mov     rax,rbx
        mov     [pseudoHeader+10],ah
        mov     [pseudoHeader+11],al

        mov     rax,pseudoHeader
        mov     [checkAdd1],eax
        mov     [checkSize1],word 12
        mov     rax,rdx
        add     rax,20
        mov     [checkAdd2],eax
        mov     rax,rbx
        mov     [checkSize2],ax

        call    checksum

        ; store it in the TCP checksum ( in the correct order! )
        mov     ax,[checkResult]

        mov     [rdx + 20 + 16], ah
        mov     [rdx + 20 + 17], al

        ; Fill in the IP header checksum
        mov     rax,rdx
        mov     [checkAdd1],eax
        mov     [checkSize1],word 20
        mov     [checkAdd2],dword 0
        mov     [checkSize2],word 0

        call    checksum

        mov     ax,[checkResult]
        mov     [rdx + 10],ah
        mov     [rdx + 11],al

        ret


; Increments the 32 bit value pointed to by esi in internet order
inc_inet_esi:
        push    rax
        add     rsi, 3
        mov     al,byte[rsi]
        inc     al
        mov     byte[rsi],al
        cmp     al,0
        jnz     iie_exit
        dec     rsi
        mov     al,byte[rsi]
        inc     al
        mov     byte[rsi],al
        cmp     al, 0
        jnz     iie_exit
        dec     rsi
        mov     al,byte[rsi]
        inc     al
        mov     byte[rsi],al
        cmp     al,0
        jnz     iie_exit
        dec     rsi
        mov     al,byte[rsi]
        inc     al
        mov     byte[rsi],al

iie_exit:
        pop     rax
        ret


; Increments the 32 bit value pointed to by esi in internet order
; by the value in ecx
add_inet_esi:
        push    rax

        mov     al,[rsi]
        shl     rax,8
        inc     rsi
        mov     al,[rsi]
        shl     rax,8
        inc     rsi
        mov     al,[rsi]
        shl     rax,8
        inc     rsi
        mov     al,[rsi]
        add     rax,rcx
        mov     [rsi],al
        dec     rsi
        shr     rax,8
        mov     [rsi],al
        dec     rsi
        shr     rax,8
        mov     [rsi],al
        dec     rsi
        shr     rax,8
        mov     [rsi],al
        pop     rax
        ret



TCBStateHandler:
        dd      stateTCB_LISTEN
        dd      stateTCB_SYN_SENT
        dd      stateTCB_SYN_RECEIVED
        dd      stateTCB_ESTABLISHED
        dd      stateTCB_FIN_WAIT_1
        dd      stateTCB_FIN_WAIT_2
        dd      stateTCB_CLOSE_WAIT
        dd      stateTCB_CLOSING
        dd      stateTCB_LAST_ACK
        dd      stateTCB_TIME_WAIT
        dd      stateTCB_CLOSED


;***************************************************************************
;   Function
;      tcpStateMachine
;
;   Description
;       TCP state machine
;       This is a kernel function, called by tcp_rx
;
;       IP buffer address given in edx
;          Socket/TCB address in [eax + sockets]
;
;       The IP buffer will be released by the caller
;***************************************************************************
tcpStateMachine:
        mov     rbx,sockets
        add     rbx,rax
        mov     [sktAddr],rbx

        ; as a packet has been received, update the TCB timer
        mov     rcx,TWOMSL
        mov     [rbx + 32],rcx

        ; If the received packet has an ACK bit set,
        ; remove any packets in the resend queue that this
        ; received packet acknowledges
        pushdu
        mov     cl,[rdx + 33]
        and     cl,0x10
        cmp     cl,0x10
        jne     tsm001                      ; No ACK, so no data yet


        ; get skt number in al
        shr     rax,12

        ; The ack number is in [edx + 28], inet format
        ; skt in al

        mov     rsi,resendQ
        mov     rcx,0

t001:
        cmp     rcx,NUMRESENDENTRIES
        je      t003              ; None left
        cmp     [rsi],al
        je      t002              ; found one
        inc     rcx
        add     rsi,4
        jmp     t001

t002:                   ; Can we delete this buffer?

                        ; If yes, goto t004. No, goto t001
                        ; Get packet data address

        push    rcx
        inc     rcx
        ; Now get buffer location, and copy buffer across. argh! more copying,,
        mov     rdi,resendBuffer - IPBUFFSIZE
t002a:
        add     rdi,IPBUFFSIZE
        loop    t002a

        ; we have dest buffer location in edi. incoming packet in edx.
        ; Get this packets sequence number
        ; preserve al, ecx, esi, edx

        mov     cl,[rdi + 24]
        shl     rcx,8
        mov     cl,[rdi + 25]
        shl     rcx,8
        mov     cl,[rdi + 26]
        shl     rcx,8
        mov     cl,[rdi + 27]
        movzx   rbx,byte [rdi + 3]
        mov     bh,[rdi + 2]
        sub     rbx,40
        add     rcx,rbx          ; ecx is now seq# of last byte +1, intel format

        ; get recievd ack #, in intel format
        mov     bl,[rdx + 28]
        shl     rbx,8
        mov     bl,[rdx + 29]
        shl     rbx,8
        mov     bl,[rdx + 30]
        shl     rbx,8
        mov     bl,[rdx + 31]

        cmp     rbx,rcx        ; Finally. ecx = rx'ed ack. ebx = last byte in que
                            ; DANGER! need to handle case that we have just
                            ; passed the 2**32, and wrapped round!
        pop     rcx

        jae     t004             ; if rx > old, delete old
        inc     rcx
        add     rsi,4
        jmp     t001


t004:
        dec     dword [arp_rx_count] ; ************ TEST ONLY!

        mov     [rsi],byte 0xFF
        inc     rcx
        add     rsi,4
        jmp     t001

t003:

tsm001:
        popdu

        ; Call handler for given TCB state
        mov     rbx,[rax + sockets+28]
        cmp     rbx,TCB_LISTEN
        jb      tsm_exit
        cmp     rbx,TCB_CLOSED
        ja      tsm_exit

        dec     rbx
        call    qword [TCBStateHandler+rbx*4]

tsm_exit:
        ret



stateTCB_LISTEN:
        ; In this case, we are expecting a SYN packet
        ; For now, if the packet is a SYN, process it, and send a response
        ; If not, ignore it

        ; Look at control flags
        mov     bl,[rdx + 33]
        and     bl,0x02
        cmp     bl,0x02
        jnz     stl_exit

        ; We have a SYN. update the socket with this IP packets details,
        ; And send a response

        mov     rbx,[rdx + 12] ; IP source address
        mov     [rax + sockets + 16],ebx
        mov     bx,[rdx + 20] ; IP source port
        mov     [rax + sockets + 20],bx
        mov     rbx, [rdx + 24] ; IRS
        mov     [rax + sockets + 40],ebx
        mov     [rax + sockets + 56],ebx
        mov     rsi,sockets
        add     rsi,rax
        add     rsi,56
        call    inc_inet_esi ; RCV.NXT
        mov     rbx,[rax + sockets + 36]    ; ISS
        mov     [rax + sockets + 48],ebx    ; SND.NXT

        ; Now construct the response, and queue for sending by IP
        mov     rax,EMPTY_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      stl_exit

        push    rax
        mov     bl,0x12        ; SYN + ACK
        mov     rcx,0
        mov     rsi,0

        call    buildTCPPacket

        mov     rax,NET1OUT_QUEUE
        mov     rdx,[stack_ip]
        mov     rcx,[ sktAddr ]
        mov     rcx,[ rcx + 16 ]
        cmp     rdx,rcx
        jne     stl_notlocal
        mov     rax,IPIN_QUEUE

stl_notlocal:
        ; Send it.
        pop     rbx
        call    queue


        mov     rbx,TCB_SYN_RECEIVED
        mov     rsi,[sktAddr]
        mov     [rsi + 28],ebx

        ; increament SND.NXT in socket
        add     rsi,48
        call    inc_inet_esi

stl_exit:
        ret



stateTCB_SYN_SENT:
        ; We are awaiting an ACK to our SYN, with a SYM
        ; Look at control flags - expecting an ACK
        mov     bl,[rdx + 33]
        and     bl,0x12
        cmp     bl,0x12
        jnz     stss_exit

        mov     rbx,TCB_ESTABLISHED
        mov     rsi,[sktAddr]
        mov     [rsi + 28],ebx

        ; Store the recv.nxt field
        mov     rax,[rdx + 24]

        ; Update our recv.nxt field
        mov     rsi,[sktAddr]
        add     rsi,56
        mov     [rsi],eax
        call    inc_inet_esi

        ; Send an ACK
        ; Now construct the response, and queue for sending by IP
        mov     rax,EMPTY_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      stss_exit

        push    rax

        mov     bl,0x10        ; ACK
        mov     rcx,0
        mov     rsi,0

        call    buildTCPPacket

        mov     rax,NET1OUT_QUEUE

        mov     rdx,[stack_ip]
        mov     rcx,[ sktAddr ]
        mov     rcx,[ rcx + 16 ]
        cmp     rdx,rcx
        jne     stss_notlocal
        mov     rax,IPIN_QUEUE

stss_notlocal:
        ; Send it.
        pop     rbx
        call    queue

stss_exit:
        ret



stateTCB_SYN_RECEIVED:
        ; In this case, we are expecting an ACK packet
        ; For now, if the packet is an ACK, process it,
        ; If not, ignore it

        ; Look at control flags - expecting an ACK
        mov     bl,[rdx + 33]
        and     bl,0x10
        cmp     bl,0x10
        jnz     stsr_exit

        mov     rbx,TCB_ESTABLISHED
        mov     rsi,[sktAddr]
        mov     [rsi + 28],rbx

stsr_exit:
        ret



stateTCB_ESTABLISHED:
        ; Here we are expecting data, or a request to close
        ; OR both...

        ; Did we receive a FIN or RST?
        mov     bl,[rdx + 33]
        and     bl,0x05
        cmp     bl,0
        je      ste_chkack

        ; It was a fin or reset.

        ; Remove resend entries from the queue  - I dont want to send any more data
        pushdu

        mov     rbx,[sktAddr]
        sub     rbx,sockets
        shr     rbx,12             ; get skt #

        mov     rsi,resendQ
        mov     rcx,0

ste001:
        cmp     rcx,NUMRESENDENTRIES
        je      ste003              ; None left
        cmp     [rsi],bl
        je      ste002              ; found one
        inc     rcx
        add     rsi,4
        jmp     ste001

ste002:
        dec     dword [arp_rx_count] ; ************ TEST ONLY!

        mov     [rsi],byte 0xFF
        jmp     ste001

ste003:
        popdu

        ; was it a reset?
        mov     bl,[rdx + 33]
        and     bl,0x04
        cmp     bl,0x04
        jne     ste003a

        mov     rsi,[sktAddr]
        mov     rbx,TCB_CLOSED
        mov     [rsi + 28],rbx
        jmp     ste_exit

ste003a:
        ; Send an ACK to that fin, and enter closewait state

        mov     rsi,[sktAddr]
        mov     rbx,TCB_CLOSE_WAIT
        mov     [rsi + 28],ebx
        add     rsi,56
        mov     rax,[rsi]              ; save original
        call    inc_inet_esi
        ;; jmp    ste_ack - NO, there may be data

ste_chkack:
        ; Check that we received an ACK
        mov     bl,[rdx + 33]
        and     bl,0x10
        cmp     bl,0x10
        jnz     ste_exit


        ; TODO - done, I think!
        ; First, look at the incoming window. If this is less than or equal to 1024,
        ; Set the socket window timer to 1. This will stop an additional packets being
        ; queued.
        ; ** I may need to tweak this value, since I do not know how many packets are already queued
        mov     ch,[rdx + 34]
        mov     cl,[rdx + 35]
        cmp     cx,1024
        ja      ste004

        mov     rcx,[sktAddr]
        mov     [rcx+72],dword 1

ste004:

        ; OK, here is the deal
        ; My recv.nct field holds the seq of the expected next rec byte
        ; if the recevied sequence number is not equal to this, do not
        ; increment the recv.nxt field, do not copy data - just send a
        ; repeat ack.

        ; recv.nxt is in dword [rdx+24], in inext format
        ; recv seq is in [sktAddr]+56, in inet format
        ; just do a comparision
        mov     rcx,[sktAddr]
        add     rcx,56

        cmp     [rcx - 56 + 28],dword TCB_CLOSE_WAIT
        mov     rcx,[rcx]
        jne     stenofin
        mov     rcx,rax

stenofin:
        cmp     rcx,[rdx+24]
        jne     ste_ack


        ; Read the data bytes, store in socket buffer
        xor     rcx,rcx
        mov     ch,[rdx + 2]
        mov     cl,[rdx + 3]
        sub     rcx,40                    ; Discard 40 bytes of header

        cmp     rcx,0
        jnz     ste_data                ; Read data, if any

        ; If we had received a fin, we need to ACK it.
        mov     rsi,[sktAddr]
        mov     rbx,[rsi + 28]
        cmp     rbx,TCB_CLOSE_WAIT
        jz      ste_ack
        jnz     ste_exit

ste_data:
        push    rcx
        mov     rsi,[sktAddr]

        add     [rsi + 24],ecx      ; increment the count of bytes in buffer

        mov     rax,[rsi + 4]       ; get socket owner PID
        push    rax

        mov     rax,[rsi + 24]      ; get # of bytes already in buffer

        ; point to the location to store the data
        add     rsi,rax
        sub     rsi,rcx
        add     rsi,SOCKETHEADERSIZE

        add     rdx,40        ; rdx now points to the data
        mov     rdi,rsi
        mov     rsi,rdx

        cld
        rep     movsb          ; copy the data across

        ; flag an event to the application
        pop     rax
        mov     rcx,1
        mov     rsi,0x6040+0x8

news:
        cmp     [rsi],eax
        je      foundPID1
        inc     rcx
        add     rsi,0x20
        cmp     rcx,[0x6008]
        jbe     news

foundPID1:
        shl     rcx,8
        or      dword [rcx+0x80000+0xA8],dword 10000000b ; stack event

        pop     rcx

        ; Update our recv.nxt field
        mov     rsi,[sktAddr]
        add     rsi,56
        call    add_inet_esi

ste_ack:
        ; Send an ACK
        ; Now construct the response, and queue for sending by IP
        mov     rax,EMPTY_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      ste_exit

        push    rax

        mov     bl,0x10        ; ACK
        mov     rcx,0
        mov     rsi,0

        call    buildTCPPacket

        mov     rax,NET1OUT_QUEUE

        mov     rdx,[stack_ip]
        mov     rcx,[ sktAddr ]
        mov     rcx,[ rcx + 16 ]
        cmp     rdx,rcx
        jne     ste_notlocal
        mov     rax,IPIN_QUEUE
ste_notlocal:

        ; Send it.
        pop     rbx
        call    queue

ste_exit:
        ret



stateTCB_FIN_WAIT_1:
        ; We can either receive an ACK of a fin, or a fin
        mov     bl,[rdx + 33]
        and     bl,0x10
        cmp     bl,0x10
        jnz     stfw1_001

        ; It was an ACK
        mov     rsi,[sktAddr]
        mov     rbx,TCB_FIN_WAIT_2
        mov     [rsi + 28],ebx
        jmp     stfw1_exit

stfw1_001:
        ; It must be a fin then
        mov     rsi,[sktAddr]
        mov     rbx,TCB_CLOSING
        mov     [rsi + 28],ebx
        add     rsi,56
        call    inc_inet_esi

        ; Send an ACK
        mov     rax,EMPTY_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      stfw1_exit

        push    rax

        mov     bl,0x10        ; ACK
        mov     rcx,0
        mov     rsi,0

        call    buildTCPPacket
        mov     rax,NET1OUT_QUEUE

        mov     rdx,[stack_ip]
        mov     rcx,[ sktAddr ]
        mov     rcx,[ rcx + 16 ]
        cmp     rdx,rcx
        jne     stfw1_notlocal
        mov     rax,IPIN_QUEUE

stfw1_notlocal:
        ; Send it.
        pop     rbx
        call    queue

stfw1_exit:
        ret



stateTCB_FIN_WAIT_2:
        mov     rsi,[sktAddr]

        ; Get data length
        xor     rcx,rcx
        mov     ch,[rdx+2]
        mov     cl,[rdx+3]
        sub     rcx,40

        mov     bl,[rdx + 33]
        and     bl,0x01
        cmp     bl,0x01
        jne     stfw2001

        ; Change state, as we have a fin
        mov     rbx,TCB_TIME_WAIT
        mov     [rsi + 28],ebx

        inc     rcx                     ; FIN is part of the sequence space

stfw2001:
        add     rsi,56
        call    add_inet_esi

        ; Send an ACK
        mov     rax,EMPTY_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      stfw2_exit

        push    rax

        mov     bl,0x10        ; ACK
        mov     rcx,0
        mov     rsi,0

        call    buildTCPPacket

        mov     rax,NET1OUT_QUEUE

        mov     rdx,[stack_ip]
        mov     rcx,[ sktAddr ]
        mov     rcx,[ rcx + 16 ]
        cmp     rdx,rcx
        jne     stfw2_notlocal
        mov     rax,IPIN_QUEUE

stfw2_notlocal:
        ; Send it.
        pop     rbx
        call    queue

        ; Only delete the socket if we received the FIN

        mov     bl,[rdx + 33]
        and     bl,0x01
        cmp     bl,0x01
        jne     stfw2_exit

;    mov     rdi,[sktAddr]

    ; delete the socket. Should really wait for 2MSL
;    xor     rax, rax
;    mov     rcx,SOCKETHEADERSIZE
;    cld
;    rep     stosb

stfw2_exit:
        ret



stateTCB_CLOSE_WAIT:
        ; Intentionally left empty
        ; socket_close_tcp handles this
        ret



stateTCB_CLOSING:
        ; We can either receive an ACK of a fin, or a fin
        mov     bl,[rdx + 33]
        and     bl,0x10
        cmp     bl,0x10
        jnz     stc_exit

        ; It was an ACK

        mov     rdi,[sktAddr]

        ; delete the socket
        xor     rax,rax
        mov     rcx,SOCKETHEADERSIZE
        cld
        rep     stosb

stc_exit:
        ret



stateTCB_LAST_ACK:
        ; Look at control flags - expecting an ACK
        mov     bl,[rdx + 33]
        and     bl,0x10
        cmp     bl,0x10
        jnz     stla_exit

        mov     rdi,[sktAddr]

        ; delete the socket
        xor     rax,rax
        mov     rcx,SOCKETHEADERSIZE
        cld
        rep     stosb

stla_exit:
        ret



stateTCB_TIME_WAIT:
        ret



stateTCB_CLOSED:
        ret
