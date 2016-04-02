;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                 ;;
;;  UDP.INC                                                        ;;
;;                                                                 ;;
;;  UDP Processes for Tensor OS  TCP/IP stack                      ;;
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
;       udp_rx      Handles received IP packets with the UDP protocol
;
;*******************************************************************




;***************************************************************************
;   Function
;      udp_rx
;
;   Description
;       UDP protocol handler
;       This is a kernel function, called by ip_rx
;       IP buffer address given in edx
;          Free up (or re-use) IP buffer when finished
;
;***************************************************************************
udp_rx:
        push    rax

        ; First validate the header & checksum. Discard buffer if error

        ; Look for a socket where
        ; IP Packet UDP Destination Port = local Port
        ; IP Packet SA = Remote IP

        movzx   rbx,word [rdx + 22]   ; get the local port from
                                  ; the IP packet's UDP header
        mov     rax,SOCKETBUFFSIZE * NUM_SOCKETS
        mov     rcx,NUM_SOCKETS

fs1:
        sub     rax,SOCKETBUFFSIZE
        cmp     [rax + sockets + 12],bx ; bx will hold the 'wrong' value,
                                    ; but the comparision is correct
        loopnz  fs1                     ; Return back if no match
        jz      fs_done

        ; No match, so exit
        jmp     udprx_001

fs_done:
        ; For dhcp, we must allow any remote server to respond.
        ; I will accept the first incoming response to be the one
        ; I bind to, if the socket is opened with a destination IP address of
        ; 255.255.255.255
        mov     rbx, [rax + sockets + 16]
        mov     r8,0xffffffff
        cmp     rbx,r8
        je      udprx_002

        mov     rbx,[rdx + 12]    ; get the Source address from the IP packet
        cmp     [rax + sockets + 16],ebx
        jne     udprx_001          ; Quit if the source IP is not valid

udprx_002:
        ; OK - we have a valid UDP packet for this socket.
        ; First, update the sockets remote port number with the incoming msg
        ; - it will have changed
        ; from the original ( 69 normally ) to allow further connects
        movzx   rbx,word [rdx + 20]      ; get the UDP source port
                                     ; ( was 69, now new )
        mov     [rax + sockets + 20],bx

        ; Now, copy data to socket. We have socket address as [rax + sockets].
        ; We have IP packet in edx

        ; get # of bytes in rcx
        movzx   rcx,byte [rdx + 3]  ; total length of IP packet. Subtract
        mov     ch,byte [rdx + 2]   ; 20 + 8 gives data length
        sub     rcx,28

        mov     rbx,rax
        add     rbx,sockets         ; rbx = address of actual socket

        mov     rax,[rbx+ 4]       ; get socket owner PID
        push    rax

        mov     rax,[rbx + 24]      ; get # of bytes already in buffer
        add     [rbx + 24],ecx      ; increment the count of bytes in buffer

        ; point to the location to store the data
        add     rbx,rax
        add     rbx,SOCKETHEADERSIZE

        ; ebx = location for first byte, ecx has count,
        ; edx points to data

        add     rdx,28        ; rdx now points to the data
        mov     rdi,rbx
        mov     rsi,rdx

        cld
        rep     movsb          ; copy the data across

        ; flag an event to the application
        pop     rax
        mov     rcx,1
        mov     rsi,0x6040+0x8

newsearch:
        cmp     [rsi],rax
        je      foundPID
        inc     rcx
        add     rsi,0x40
        cmp     rcx,[0x6008]
        jbe     newsearch

foundPID:
        shl     rcx,8
        or      dword [rcx+0x80000+0xA8],dword 10000000b ; stack event

        mov     [check_idle_semaphore],200

udprx_001:
        pop     rax
        call    freeBuff    ; Discard the packet
        ret
