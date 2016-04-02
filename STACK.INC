;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                 ;;
;;  STACK.INC                                                      ;;
;;                                                                 ;;
;;  TCP/IP stack for Tensor OS                                     ;;
;;                                                                 ;;
;;  Version 0.7  4th July 2014                                     ;;
;;                                                                 ;;
;;  Copyright 2012 Marco K, oldklickers@gmail.com                  ;;
;;                                                                 ;;
;;  See file COPYING for details                                   ;;
;;                                                                 ;;
;; Version 0.7                                                     ;;
;;         Added a timer per socket to allow delays when rx window ;;
;;         gets below 1KB                                          ;;
;;                                                                 ;;
;;  10.01.2007 Bugfix for checksum function from Marco K           ;;
;;                                                                 ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;*******************************************************************
;   Interface
;      The interfaces defined in ETHERNET.INC plus:
;      stack_init
;      stack_handler
;      app_stack_handler
;      app_socket_handler
;      checksum
;
;*******************************************************************



;
;   IP Packet after reception - Normal IP packet format
;
;    0                   1                   2                   3
;    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
;
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;0  |Version|  IHL  |Type of Service|       Total Length            |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;4  |         Identification        |Flags|      Fragment Offset    |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;8  |  Time to Live |    Protocol   |         Header Checksum       |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;12 |                       Source Address                          |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;16 |                    Destination Address                        |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |      Data                                                     |
;   +-+-+-..........                                               -+


;   TCP Payload ( Data field in IP datagram )
;
;    0                   1                   2                   3
;    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;20 |          Source Port          |       Destination Port        |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;24 |                        Sequence Number                        |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;28 |                    Acknowledgment Number                      |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;32 |  Data |           |U|A|P|R|S|F|                               |
;   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
;   |       |           |G|K|H|T|N|N|                               |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;36 |           Checksum            |         Urgent Pointer        |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;40 |                    Options                    |    Padding    |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |                             data


;
;   UDP Payload ( Data field in IP datagram )
;
;    0                   1                   2                   3
;    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
;
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |       Source Port             |      Destination Port         |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   | Length ( UDP Header + Data )  |           Checksum            |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |       UDP Data                                                |
;   +-+-+-..........                                               -+
;


;
;  Socket Descriptor + Buffer
;
;    0                   1                   2                   3
;    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
;
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |                    Status ( of this buffer )                  |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |  Application Process ID                                       |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |                  Local IP Address                             |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   | Local IP Port                 | Unused ( set to 0 )           |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |                  Remote IP Address                            |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   | Remote IP Port                | Unused ( set to 0 )           |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 24|   Rx Data Count                                   INTEL format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 28|                 TCB STATE                         INTEL format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 32|   TCB Timer (seconds)                             INTEL format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 36| ISS (Inital Sequence # used by this connection )   INET format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 40| IRS ( Inital Receive Sequence # )                  INET format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 44| SND.UNA  Seq # of unack'ed sent packets            INET format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 48| SND.NXT  Next send seq # to use                    INET format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 52| SND.WND  Send window                               INET format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 56| RCV.NXT  Next expected receive sequence #          INET format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 60| RCV.WND  Receive window                            INET format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 64| SEG.LEN  Segment length                           INTEL format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 68| SEG.WND  Segment window                           INTEL format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 72| Retransmit queue # NOW WINDOW SIZE TIMER          INTEL format|
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
; 76|       RX Data                                                 |
;   +-+-+-..........                                               -+



; IP protocol numbers
PROTOCOL_ICMP     equ      1
PROTOCOL_TCP      equ      6
PROTOCOL_UDP      equ      17


; TIPBUFF status values
BUFF_EMPTY         equ     0
BUFF_RX_FULL       equ     1
BUFF_ALLOCATED     equ     2
BUFF_TX_FULL       equ     3

NUM_IPBUFFERS      equ     20    ; buffers allocated for TX/RX

SOCK_EMPTY         equ     0        ; socket not in use
SOCK_OPEN          equ     1        ; open issued, but no data sent

; TCP opening modes
SOCKET_PASSIVE     equ     0
SOCKET_ACTIVE      equ     1

; TCP TCB states
TCB_LISTEN         equ        1
TCB_SYN_SENT       equ        2
TCB_SYN_RECEIVED   equ        3
TCB_ESTABLISHED    equ        4
TCB_FIN_WAIT_1     equ        5
TCB_FIN_WAIT_2     equ        6
TCB_CLOSE_WAIT     equ        7
TCB_CLOSING        equ        8
TCB_LAST_ACK       equ        9
TCB_TIME_WAIT      equ        10
TCB_CLOSED         equ        11

TWOMSL              equ     10      ; # of secs to wait before closing socket

; socket buffers
SOCKETBUFFSIZE     equ        4096  ; state + config + buffer.
SOCKETHEADERSIZE   equ        76    ; thus 4096 - SOCKETHEADERSIZE bytes data

NUM_SOCKETS        equ        16    ; Number of open sockets supported. Was 20


NUMQUEUES          equ        4
EMPTY_QUEUE        equ        0
IPIN_QUEUE         equ        1
IPOUT_QUEUE        equ        2
NET1OUT_QUEUE      equ        3

NO_BUFFER          equ        0xFFFF
IPBUFFSIZE         equ        1500                ; MTU of an ethernet packet
NUMQUEUEENTRIES    equ        NUM_IPBUFFERS
NUMRESENDENTRIES    equ         18              ; Buffers for TCP resend packets
TCP_RETRIES         equ         5               ; Number of times to resend a packet
TCP_TIMEOUT         equ         10              ; resend if not replied to in x hs

; These are the 0x40 function codes for application access to the stack
STACK_DRIVER_STATUS  equ   52
SOCKET_INTERFACE     equ   53


; 128KB allocated for the stack and network driver buffers and other
; data requirements
stack_data_start     equ   0x700000
eth_data_start       equ   0x700000
stack_data           equ   0x704000
stack_data_end       equ   0x71ffff

; 32 bit word
stack_config         equ   stack_data
; 32 bit word - IP Address in network format
stack_ip             equ   stack_data + 4
; 1 byte. 0 == inactive, 1 = active
slip_active          equ   stack_data + 8       ; no longer used
; 1 byte. 0 == inactive, 1 = active
ethernet_active      equ   stack_data + 9
unused               equ   stack_data + 10
;  word. Buffer number, -1 if none
rx_buff_ptr          equ   stack_data + 12
; dword. Buffer number, -1 if none
tx_buff_ptr          equ   stack_data + 16
; byte.
slip_rx_state        equ   stack_data + 20      ; no longer used
; byte
slip_tx_state        equ   stack_data + 21      ; no longer used
; dword. Index into data
rx_data_ptr          equ   stack_data + 22
; dword. Index into data
tx_data_ptr          equ   stack_data + 26
; word. Count of bytes to send
tx_msg_len           equ   stack_data + 30
; Address of selected socket
sktAddr              equ   stack_data + 32
; Parameter to checksum routine - data ptr
checkAdd1            equ   stack_data + 36
; Parameter to checksum routine - 2nd data ptr
checkAdd2            equ   stack_data + 40
; Parameter to checksum routine - data size
checkSize1           equ   stack_data + 44
; Parameter to checksum routine - 2nd data size
checkSize2           equ   stack_data + 46
; result of checksum routine
checkResult          equ   stack_data + 48

; holds the TCP/UDP pseudo header. SA|DA|0|prot|UDP len|
pseudoHeader         equ   stack_data + 50

; receive and transmit IP buffer allocation
sockets              equ   stack_data + 62
Next_free2           equ   sockets + (SOCKETBUFFSIZE * NUM_SOCKETS)
; 1560 byte buffer for rx / tx ethernet packets
Ether_buffer         equ   Next_free2
Next_free3           equ   Ether_buffer + 1560
last_1sTick          equ   Next_free3
IPbuffs              equ   Next_free3 + 1
queues               equ   IPbuffs + ( NUM_IPBUFFERS * IPBUFFSIZE )
queueList            equ   queues + (2 * NUMQUEUES)
last_1hsTick         equ   queueList + ( 2 * NUMQUEUEENTRIES )

;resendQ              equ   queueList + ( 2 * NUMQUEUEENTRIES )
;resendBuffer         equ    resendQ + ( 4 * NUMRESENDENTRIES ) ; for TCP
;                    equ    resendBuffer + ( IPBUFFSIZE * NUMRESENDENTRIES )



resendQ             equ     0x770000
resendBuffer        equ     resendQ + ( 4 * NUMRESENDENTRIES ) ; for TCP


;***************************************************************************
;   Function
;      stack_init
;
;   Description
;      Clear all allocated memory to zero. This ensures that
;       on startup, the stack is inactive, and consumes no resources
;       This is a kernel function, called prior to the OS main loop
;       in set_variables
;
;***************************************************************************
stack_init:
        xor     rax,rax
        mov     rdi,stack_data_start
        mov     rcx,0x20000 / 8  ; Assume that we have 128KB of data
        cld
        rep     stosq

        ; Initialise TCP resend queue data structures
        mov     rax,0xFFFFFFFF
        mov     rdi,resendQ
        mov     rcx,NUMRESENDENTRIES  ; 1 dword per entry
        cld
        rep     stosq

        mov     rax,0xFFFFFFFF
        mov     [rx_buff_ptr],eax
        mov     [tx_buff_ptr],eax

        ; Put in some defaults : slip, 0x3f8, 4, ip=192.168.1.22
        ; Saves me entering them each boot up when debugging
        mov     rax,0x03f80401
        mov     [stack_config],eax
        mov     rax,0xc801a8c0
        mov     [stack_ip],eax

        call    queueInit

        ; The following block sets up the 1s timer
        mov     al,0x0
        out     0x70,al
        in      al,0x71
        mov     [last_1sTick],al

        retq



;***************************************************************************
;   Function
;      stack_handler
;
;   Description
;       The kernel loop routine for the stack
;       This is a kernel function, called in the main loop
;
;***************************************************************************
stack_handler:

        call    ethernet_driver
        call    ip_rx


        ; Test for 10ms tick, call tcp timer
        mov     rax,[0xFDF0]
        cmp     rax,[last_1hsTick]
        je      sh_001

        mov     [last_1hsTick],eax
        call    tcp_tx_handler

sh_001:

        ; Test for 1 second event, call 1s timer functions
        mov     al,0x0   ;second
        out     0x70,al
        in      al,0x71
        cmp     al,[last_1sTick]
        je      sh_exit

        mov     [last_1sTick],al

        call    arp_timer
        call    tcp_tcb_handler

sh_exit:
        retq




;***************************************************************************
;   Function
;      is_localport_unused
;
;   Description
;         scans through all the active sockets , looking to see if the
;      port number specified in bx is in use as a localport number.
;      This is useful when you want a to generate a unique local port
;      number.
;          On return, rax = 1 for free, 0 for in use
;
;***************************************************************************
is_localport_unused:
        mov     al,bh
        mov     ah,bl
        mov     bx,ax

        mov     rdx,SOCKETBUFFSIZE * NUM_SOCKETS
        mov     rcx,NUM_SOCKETS
        mov     rax,0                    ; Assume the return value is 'in use'

ilu1:
        sub     rdx,SOCKETBUFFSIZE
        cmp     [rdx + sockets + 12],bx
        loopnz  ilu1                  ; Return back if the socket is occupied

        jz      ilu_exit
        inc     rax                         ; return port not in use

ilu_exit:
        retq



;***************************************************************************
;   Function
;      get_free_socket
;
;   Description
;
;***************************************************************************
get_free_socket:
        push    rcx
        mov     rax,SOCKETBUFFSIZE * NUM_SOCKETS
        mov     rcx,NUM_SOCKETS

gfs1:
        sub     rax,SOCKETBUFFSIZE
        cmp     [rax + sockets],dword SOCK_EMPTY
        loopnz  gfs1                  ; Return back if the socket is occupied
        mov     rax,rcx
        pop     rcx
        jz      gfs_exit
        mov     rax,0xFFFFFFFF

gfs_exit:
        retq



;***************************************************************************
;   Function
;      checksum
;
;   Description
;       checkAdd1,checkAdd2, checkSize1, checkSize2, checkResult
;       Dont break anything; Most registers are used by the caller
;       This code is derived from the 'C' source, cksum.c, in the book
;       Internetworking with TCP/IP Volume II by D.E. Comer
;
;***************************************************************************

checksum:

        pushdu

        mov     rax,[checkAdd1]

        xor     rdx,rdx                  ; edx is the accumulative checksum
        xor     rbx,rbx
        mov     cx,[checkSize1]
        shr     cx,1
        jz      cs1_1

cs1:

        mov     bh,[rax]
        mov     bl,[rax + 1]

        add     rax,2
        add     rdx,rbx

        loopq   cs1

cs1_1:

        and     word [checkSize1],0x01
        jz      cs_test2

        mov     bh,[rax]
        xor     bl,bl

        add     rdx,rbx

cs_test2:

        mov     cx,[checkSize2]
        cmp     cx,0
        jz      cs_exit                     ; Finished if no 2nd buffer

        mov     rax,[checkAdd2]

        shr     cx,1
        jz      cs2_1

cs2:
        mov     bh,[rax]
        mov     bl,[rax + 1]

        add     rax,2
        add     rdx,rbx

        loopq   cs2

cs2_1:
        and     word [checkSize2],0x01
        jz      cs_exit

        mov     bh,[rax]
        xor     bl,bl

        add     rdx,rbx

cs_exit:
        mov     rbx, rdx

        shr     rbx,16
        and     rdx,0xffff
        add     rdx,rbx
        mov     rax,rdx
        shr     rax,16
        add     rdx,rax
        not     dx

        mov     [checkResult],dx
        popdu
        retq




;***************************************************************************
;   Function
;      app_stack_handler
;
;   Description
;       This is an application service, called by int 0x60 fn 52
;       It provides application access to the network interface layer
;
;***************************************************************************
app_stack_handler:
        cmp     rax,0
        jnz     not0
        ; Read the configuartion word
        mov     rax,[stack_config]
        retq

not0:
        cmp     rax,1
        jnz     not1
        ; read the IP address

        mov     rax,[stack_ip]
        retq

not1:
        cmp     rax,2
        jnz     not2

        ; write the configuration word
        mov     [stack_config],ebx

        ; <Slip shouldn't be active anyway - thats an operational issue.>
        ; If ethernet now enabled, probe for the card, reset it and empty
        ; the packet buffer
        ; If all successfull, enable the card.
        ; If ethernet now disabled, set it as disabled. Should really
        ; empty the tcpip data area too.

        ; ethernet interface is '3' in ls 7 bits
        and     bl,0x7f
        cmp     bl,3

        je      ash_eth_enable
        ; Ethernet isn't enabled, so make sure that the card is disabled
        mov     [ethernet_active],byte 0

        retq

ash_eth_enable:
        ; Probe for the card. This will reset it and enable the interface
        ; if found
        call    eth_probe
        cmp     rax,0
        je      ash_eth_done            ; Abort if no hardware found

        mov     [ethernet_active],byte 1

ash_eth_done:
        retq

not2:
        cmp     rax,3
        jnz     not3
        ; write the IP Address
        mov     [stack_ip],ebx
        retq

not3:
        cmp     rax,4
        jnz     not4
        ; Enabled the slip driver on the comm port
        ; slip removed
        retq

not4:
        cmp     rax,5
        jnz     not5
        ; Disable the slip driver on the comm port
        ; slip removed

not5:
        cmp     rax,6
        jnz     not6

        ; Insert an IP packet into the stacks received packet queue
        call    stack_insert_packet
        retq

not6:
        cmp     rax,7
        jnz     not7

        ; Test for any packets queued for transmission over the network

not7:
        cmp     rax,8
        jnz     not8

        call    stack_get_packet
        ; Extract a packet queued for transmission by the network
        retq

not8:
        cmp     rax,9
        jnz     not9

        ; read the gateway IP address

        mov     rax,[gateway_ip]
        retq

not9:
        cmp     rax,10
        jnz     not10

        ; read the subnet mask

        mov     rax,[subnet_mask]
        retq

not10:
        cmp     rax,11
        jnz     not11

        ; write the gateway IP Address
        mov     [gateway_ip],ebx

        retq

not11:
        cmp     rax,12
        jnz     not12

        ; write the subnet mask
        mov     [subnet_mask],ebx


not12:
        cmp     rax,13
        jnz     not13

        ; read the dns

        mov     rax,[dns_ip]
        retq

not13:
        cmp     rax,14
        jnz     stack_driver_end

        ; write the dns IP Address
        mov     [dns_ip],ebx

        retq

stack_driver_end:
        retq



;***************************************************************************
;   Function
;      app_socket_handler
;
;   Description
;       This is an application service, called by int 0x60
;       It provides application access to stack socket services
;       such as opening sockets
;
;***************************************************************************
app_socket_handler:
        cmp     rax,0
        jnz     nots0

        call    socket_open
        retq

nots0:
        cmp     rax,1
        jnz     nots1

        call    socket_close
        retq

nots1:
        cmp     rax,2
        jnz     nots2

        call    socket_poll
        retq

nots2:
        cmp     rax,3
        jnz     nots3

        call    socket_read
        retq

nots3:
        cmp     rax,4
        jnz     nots4

        call    socket_write
        retq

nots4:
        cmp     rax,5
        jnz     nots5

        call    socket_open_tcp
        retq

nots5:
        cmp     rax,6
        jnz     nots6

        call    socket_status
        retq

nots6:
        cmp     rax,7
        jnz     nots7

        call    socket_write_tcp
        retq

nots7:
        cmp     rax,8
        jnz     nots8

        call    socket_close_tcp
        retq

nots8:
        cmp     rax,9
        jnz     nots9

        call    is_localport_unused
        retq

nots9:
        cmp     rax,254
        jnz     notdump

        retq

notdump:
        cmp     rax,255
        jnz     notsdebug

        ; This sub function allows access to debugging information on the stack
        ; ebx holds the request:
        ;  100 : return length of empty queue
        ;  101 : return length of IPOUT QUEUE
        ;  102 : return length of IPIN QUEUE
        ;  103 : return length of NET1OUT QUEUE
        ; 200 : return # of ARP entries
        ; 201 : return size of ARP table ( max # entries )
        ; 202 : select ARP table entry #
        ; 203 : return IP of selected table entry
        ; 204 : return High 4 bytes of MAC address of selected table entry
        ; 205 : return low  2 bytes of MAC address of selected table entry
        ; 206 : return status word of selected table entry
        ; 207 : return Time to live of selected table entry


        ;  2 : return number of IP packets received
        ;  3 : return number of packets transmitted
        ;  4 : return number of received packets dumped
        ;  5 : return number of arp packets received
        ;  6 : return status of packet driver
        ;      ( 0 == not active, FFFFFFFF = successful )

        call    stack_internal_status
        retq

notsdebug:
        ; Invalid Option
        retq


ARPTmp:
times 14 db 0

;***************************************************************************
;   Function
;      stack_internal_status
;
;   Description
;       Returns information about the internal status of the stack
;       This is only useful for debugging
;       It works with the ethernet driver
;       sub function in rbx
;       return requested data in rax
;
;***************************************************************************
stack_internal_status:
        cmp     rbx,100
        jnz     notsis100

        ;  100 : return length of EMPTY QUEUE
        mov     rbx,EMPTY_QUEUE
        call    queueSize
        ret

notsis100:
        cmp     rbx,101
        jnz     notsis101

        ;  101 : return length of IPOUT QUEUE
        mov     rbx,IPOUT_QUEUE
        call    queueSize
        ret

notsis101:
        cmp     rbx,102
        jnz     notsis102

        ;  102 : return length of IPIN QUEUE
        mov     rbx,IPIN_QUEUE
        call    queueSize
        ret

notsis102:
        cmp     rbx,103
        jnz     notsis103

        ;  103 : return length of NET1OUT QUEUE
        mov     rbx,NET1OUT_QUEUE
        call    queueSize
        ret

notsis103:
        cmp     rbx,200
        jnz     notsis200

        ; 200 : return num entries in arp table
        movzx   rax,byte [NumARP]
        ret

notsis200:
        cmp     rbx,201
        jnz     notsis201

        ; 201 : return arp table size
        mov     rax,20 ; ARP_TABLE_SIZE
        ret

notsis201:
        cmp     rbx,202
        jnz     notsis202

        ; 202 - read the requested table entry
        ; into a temporary buffer
        ; ecx holds the entry number

        mov     rax, rcx
        mov     rcx,14 ; ARP_ENTRY_SIZE
        mul     rcx

        mov     rcx,[rax + ARPTable]
        mov     [ARPTmp],ecx
        mov     rcx,[rax + ARPTable+4]
        mov     [ARPTmp+4],ecx
        mov     rcx,[rax + ARPTable+8]
        mov     [ARPTmp+8],ecx
        mov     cx,[rax + ARPTable+12]
        mov     [ARPTmp+12],cx
        ret

notsis202:
        cmp     rbx,203
        jnz     notsis203

        ; 203 - return IP address
        mov     rax,[ARPTmp]
        ret

notsis203:
        cmp     rbx,204
        jnz     notsis204

        ; 204 - return MAC high dword
        mov     rax,[ARPTmp+4]
        ret

notsis204:
        cmp     rbx, 205
        jnz     notsis205

        ; 205 - return MAC ls word
        movzx   rax,word [ARPTmp+8]
        ret

notsis205:
        cmp     rbx,206
        jnz     notsis206

        ; 206 - return status word
        movzx   rax,word [ARPTmp+10]
        ret

notsis206:
        cmp     rbx,207
        jnz     notsis207

        ; 207 - return ttl word
        movzx   rax,word [ARPTmp+12]
        ret

notsis207:
        cmp     rbx,2
        jnz     notsis2

        ;  2 : return number of IP packets received
        mov     rax,[ip_rx_count]
        ret

notsis2:
        cmp     rbx,3
        jnz     notsis3

        ;  3 : return number of packets transmitted
        mov     rax,[ip_tx_count]
        ret

notsis3:
        cmp     rbx,4
        jnz     notsis4

        ;  4 : return number of received packets dumped
        mov     rax,[dumped_rx_count]
        ret

notsis4:
        cmp     rbx,5
        jnz     notsis5

        ;  5 : return number of arp packets received
        mov     rax,[arp_rx_count]
        ret

notsis5:
        cmp     rbx,6
        jnz     notsis6

        ;  6 : return status of packet driver
        ;  ( 0 == not active, FFFFFFFF = successful )
        mov     rax,[eth_status]
        ret

notsis6:
        xor     rax,rax
        ret



;***************************************************************************
;   Function
;      stack_get_packet
;
;   Description
;       extracts an IP packet from the NET1 output queue
;       and sends the data to the calling process
;       pointer to data in rdx
;       returns number of bytes read in rax
;
;***************************************************************************
stack_get_packet:
        ; Look for a buffer to tx
        mov     rax,NET1OUT_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      sgp_non_exit            ; Exit if no buffer available

        push    rax                     ; Save buffer number for freeing at end

        push    rdx
        ; convert buffer pointer eax to the absolute address
        mov     rcx,IPBUFFSIZE
        mul     rcx
        add     rax,IPbuffs
        pop     rdx

        push    rax                     ; save address of IP data

        ; Get the address of the callers data
        mov     rdi,[0x6020]
        add     rdi,0x20
        add     rdx,[rdi]
        mov     rdi,rdx

        pop     rax

        mov     rcx,1500           ; should get the actual number of bytes to write
        mov     rsi,rax
        cld
        rep     movsb               ; copy the data across

        ; And finally, return the buffer to the free queue
        pop     rax
        call    freeBuff

        mov     rax,1500
        ret

sgp_non_exit:
        xor     rax,rax
        ret



;***************************************************************************
;   Function
;      stack_insert_packet
;
;   Description
;       writes an IP packet into the stacks receive queue
;       # of bytes to write in rcx
;       pointer to data in rdx
;       returns 0 in rax ok, -1 == failed
;
;***************************************************************************
stack_insert_packet:

        mov     rax,EMPTY_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      sip_err_exit

        push    rax

        ; save the pointers to the data buffer & size
        push    rdx
        push    rcx

        ; convert buffer pointer eax to the absolute address
        mov     rcx,IPBUFFSIZE
        mul     rcx
        add     rax,IPbuffs

        mov     rdx,rax

        ; So, edx holds the IPbuffer ptr

        pop     rcx                     ; count of bytes to send
        mov     rbx,rcx                ; need the length later
        pop     rax                     ; get callers ptr to data to send

        ; Get the address of the callers data
        mov     rdi,[0x6020]
        add     rdi,0x20
        add     rax,[rdi]
        mov     rsi,rax

        mov     rdi,rdx
        cld
        rep     movsb               ; copy the data across

        pop     rbx

        mov     rax,IPIN_QUEUE
        call    queue

        inc     dword [ip_rx_count]

        mov     rax,0
        ret

sip_err_exit:
        mov     rax,0xFFFFFFFF
        ret



;***************************************************************************
;   Function
;      socket_open
;
;   Description
;       find a free socket
;       local port in rbx
;       remote port in rcx
;       remote ip in rdx
;       return socket # in rax, -1 if none available
;
;***************************************************************************
socket_open:
        call    get_free_socket

        mov     r8,0xFFFFFFFF
        cmp     rax,r8
        jz      so_exit

        ; ax holds the socket number that is free. Get real address
        push    rax
        shl     rax,12
        add     rax,sockets

        mov     [rax],dword SOCK_OPEN

        mov     [rax + 12],byte bh      ; Local port ( LS 16 bits )
        mov     [rax + 13],byte bl      ; Local port ( LS 16 bits )
        mov     rbx,[stack_ip]
        mov     [rax + 8],ebx         ; Local IP
        mov     [rax + 20],ch         ; Remote Port ( LS 16 bits )
        mov     [rax + 21],cl         ; Remote Port ( LS 16 bits )
        mov     [rax + 16],edx         ; Remote IP ( in Internet order )
        mov     [rax + 24],dword 0      ; recieved data count

        mov     rsi,[0x6020]
        mov     rbx,[rsi+0x4]
        mov     [rax + 4],ebx         ; save the process ID
        pop     rax      ; Get the socket number back, so we can return it

so_exit:
        ret



;***************************************************************************
;   Function
;      socket_open_tcp
;
;   Description
;       Opens a TCP socket in PASSIVE or ACTIVE mode
;       find a free socket
;       local port in rbx ( intel format )
;       remote port in rcx ( intel format )
;       remote ip in rdx ( in Internet byte order )
;       Socket open mode in rsi  ( SOCKET_PASSIVE or SOCKET_ACTIVE )
;       return socket # in rax, -1 if none available
;
;***************************************************************************
socket_open_tcp:
        call    get_free_socket

        mov     r8,0xFFFFFFFF
        cmp     rax,r8
        jz      so_exit

        ; ax holds the socket number that is free. Get real address
        push    rax
        shl     rax,12
        add     rax,sockets

        mov     [sktAddr],eax
        mov     [rax],dword SOCK_OPEN

        ; TODO - check this works!
        mov     [rax + 72],dword 0     ; Reset the window timer.

        mov     [rax + 12],byte bh      ; Local port ( LS 16 bits )
        mov     [rax + 13],byte bl      ; Local port ( LS 16 bits )
        mov     rbx,[stack_ip]
        mov     [rax + 8], ebx         ; Local IP
        mov     [rax + 20],ch         ; Remote Port ( LS 16 bits )
        mov     [rax + 21],cl         ; Remote Port ( LS 16 bits )
        mov     [rax + 16],edx         ; Remote IP ( in Internet order )
        mov     [rax + 24],dword 0      ; recieved data count

        ; Now fill in TCB state
        mov     rbx,TCB_LISTEN
        cmp     rsi,SOCKET_PASSIVE
        jz      sot_001
        mov     rbx,TCB_SYN_SENT

sot_001:
        mov     [rax + 28],ebx            ; Indicate the state of the TCB

        mov     rsi,[0x6020]
        mov     rcx,[rsi+0x8]
        mov     [rax + 4],ecx         ; save the process ID

        cmp     rbx,TCB_LISTEN
        je      sot_done

        ; Now, if we are in active mode, then we have to send a SYN to the specified remote port


        mov     rax,EMPTY_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      sot_done

        push    rax

        mov     bl,0x02        ; SYN
        mov     rcx,0

        call    buildTCPPacket

        mov     rax,NET1OUT_QUEUE

        mov     rdx,[stack_ip]
        mov     rcx,[ sktAddr ]
        mov     rcx,[ rcx + 16 ]
        cmp     rdx,rcx
        jne     sot_notlocal
        mov     rax,IPIN_QUEUE

sot_notlocal:
        ; Send it.
        pop     rbx
        call    queue

        mov     rsi,[sktAddr]

        ; increment SND.NXT in socket
        add     rsi,48
        call    inc_inet_esi

sot_done:
        pop     rax      ; Get the socket number back, so we can return it

sot_exit:
        ret



;***************************************************************************
;   Function
;      socket_close
;
;   Description
;       socket # in rbx
;       returns 0 for ok, -1 for socket not open (fail)
;
;***************************************************************************
socket_close:
        shl     rbx,12
        add     rbx,sockets
        mov     rax,0xFFFFFFFF         ; assume this operation will fail..
        cmp     [rbx],dword SOCK_EMPTY
        jz      sc_exit

        ; Clear the socket varaibles
        xor     rax,rax
        mov     rdi,rbx
        mov     rcx,SOCKETHEADERSIZE
        cld
        rep     stosb

sc_exit:
        ret



;***************************************************************************
;   Function
;      socket_close_tcp
;
;   Description
;       socket # in rbx
;       returns 0 for ok, -1 for socket not open (fail)
;
;***************************************************************************
socket_close_tcp:
        ; first, remove any resend entries
        pushdu

        mov     rsi,resendQ
        mov     rcx,0

sct001:
        cmp     rcx,NUMRESENDENTRIES
        je      sct003              ; None left
        cmp     [rsi],bl
        je      sct002              ; found one
        inc     rcx
        add     rsi,4
        jmp     sct001

sct002:
        dec     dword [arp_rx_count] ; ************ TEST ONLY!

        mov     [rsi],byte 0xFF
        jmp     sct001

sct003:
        popdu

        shl     rbx,12
        add     rbx,sockets
        mov     [sktAddr],ebx
        mov     rax,0xFFFFFFFF         ; assume this operation will fail..
        cmp     [rbx],dword SOCK_EMPTY
        jz      sct_exit

        ; Now construct the response, and queue for sending by IP
        mov     rax,EMPTY_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      stl_exit

        push    rax

        mov     bl,0x11        ; FIN + ACK
        mov     rcx,0
        mov     rsi,0

        call    buildTCPPacket

        mov     rbx,[sktAddr]

        ; increament SND.NXT in socket
        mov     rsi,48
        add     rsi,rbx
        call    inc_inet_esi


        ; Get the socket state
        mov     rax,[rbx + 28]
        cmp     rax,TCB_LISTEN
        je      destroyTCB
        cmp     rax,TCB_SYN_SENT
        je      destroyTCB
        cmp     rax,TCB_SYN_RECEIVED
        je      sct_finwait1
        cmp     rax,TCB_ESTABLISHED
        je      sct_finwait1

        ; assume CLOSE WAIT
        ; Send a fin, then enter last-ack state
        mov     rax, TCB_LAST_ACK
        mov     [rbx + 28],eax
        xor     rax,rax
        jmp     sct_send

sct_finwait1:
        ; Send a fin, then enter finwait2 state
        mov     rax,TCB_FIN_WAIT_1
        mov     [rbx + 28],eax
        xor     rax,rax

sct_send:
        mov     rax,NET1OUT_QUEUE

        mov     rdx,[stack_ip]
        mov     rcx,[ sktAddr ]
        mov     rcx,[ rcx + 16 ]
        cmp     rdx,rcx
        jne     sct_notlocal
        mov     rax,IPIN_QUEUE

sct_notlocal:
        ; Send it.
        pop     rbx
        call    queue
        jmp     sct_exit

destroyTCB:
        pop     rax
        ; Clear the socket varaibles
        xor     rax,rax
        mov     rdi,rbx
        mov     rcx,SOCKETHEADERSIZE
        cld
        rep     stosb

sct_exit:
        ret



;***************************************************************************
;   Function
;      socket_poll
;
;   Description
;       socket # in rbx
;       returns count in rax.
;
;***************************************************************************
socket_poll:
        shl     rbx,12
        add     rbx,sockets
        mov     rax,[rbx + 24]

        ret



;***************************************************************************
;   Function
;      socket_status
;
;   Description
;       socket # in rbx
;       returns TCB state in rax.
;
;***************************************************************************
socket_status:
        shl     rbx,12
        add     rbx,sockets
        mov     rax,[rbx + 28]

        ret



;***************************************************************************
;   Function
;      socket_read
;
;   Description
;       socket # in rbx
;       returns # of bytes remaining in rax, data in bl
;
;***************************************************************************
socket_read:
        shl     rbx,12
        add     rbx,sockets
        mov     rax,[rbx + 24]         ; get count of bytes
        mov     rcx,1
        test    rax,rax
        jz      sr2

        dec     rax
        mov     rsi,rbx            ; esi is address of socket
        mov     [rbx + 24],rax         ; store new count
        movzx   rbx,byte [rbx + SOCKETHEADERSIZE]  ; get the byte
        add     rsi,SOCKETHEADERSIZE
        mov     rdi,rsi
        inc     rsi

        mov     rcx,(SOCKETBUFFSIZE - SOCKETHEADERSIZE) / 4
        cld
        rep     movsq
        xor     rcx, rcx

sr1:
        jmp     sor_exit

sr2:
        xor     bl,bl

sor_exit:
        ret



;***************************************************************************
;   Function
;      socket_write
;
;   Description
;       socket in ebx
;       # of bytes to write in rcx
;       pointer to data in rdx
;       returns 0 in rax ok, -1 == failed ( invalid socket, or
;       could not queue IP packet )
;
;***************************************************************************
socket_write:
        ; First, find the address of the socket descriptor
        shl     rbx,12
        add     rbx,sockets         ; rbx = address of actual socket

        mov     rax,0xFFFFFFFF
        ; If the socket is invalid, return with an error code
        cmp     [rbx],dword SOCK_EMPTY
        je      sw_exit


        mov     rax,EMPTY_QUEUE
        call    dequeue
        cmp     ax,NO_BUFFER
        je      sw_exit

        ; Save the queue entry number
        push    rax

        ; save the pointers to the data buffer & size
        push    rdx
        push    rcx

        ; convert buffer pointer eax to the absolute address
        mov     rcx,IPBUFFSIZE
        mul     rcx
        add     rax,IPbuffs

        mov     rdx,rax

        ; So, ebx holds the socket ptr, rdx holds the IPbuffer ptr

        ; Fill in the IP header ( some data is in the socket descriptor)
        mov     rax,[rbx + 8]
        mov     [rdx + 12],eax      ; source IP
        mov     rax,[rbx + 16]
        mov     [rdx + 16],eax      ; Destination IP

        mov     al,0x45
        mov     [rdx],al         ; Version, IHL
        xor     al,al
        mov     [rdx + 1],al     ; Type of service

        pop     rax                   ; Get the UDP data length
        push    rax

        add     rax, 20 + 8           ; add IP header and UDP header lengths
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
        mov     al,17
        mov     [rdx + 9],al

        ; Checksum left unfilled
        xor     ax,ax
        mov     [rdx + 10],ax

        ; Fill in the UDP header ( some data is in the socket descriptor)
        mov     ax,[rbx + 12]
        mov     [rdx + 20],ax

        mov     ax,[rbx + 20]
        mov     [rdx + 20 + 2],ax

        pop     rax
        push    rax

        add     rax,8
        mov     [rdx + 20 + 4],ah
        mov     [rdx + 20 + 5],al

        ; Checksum left unfilled
        xor     ax,ax
        mov     [rdx + 20 + 6],ax

        pop     rcx                  ; count of bytes to send
        mov     rbx,rcx            ; need the length later
        pop     rax                  ; get callers ptr to data to send

        ; Get the address of the callers data
        mov     rdi,[0x6020]
        add     rdi,0x20
        add     rax,[rdi]
        mov     rsi,rax

        mov     rdi,rdx
        add     rdi,28
        cld
        rep     movsb               ; copy the data across

        ; we have rdx as IPbuffer ptr.
        ; Fill in the UDP checksum
        ; First, fill in pseudoheader
        mov     rax,[rdx + 12]
        mov     [pseudoHeader],eax
        mov     rax,[rdx + 16]
        mov     [pseudoHeader+4],eax
        mov     ax,0x1100            ; 0 + protocol
        mov     [pseudoHeader+8],ax
        add     rbx,8
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
        mov     [checkSize2],ax      ; was eax!! mjh 8/7/02

        call    checksum

        ; store it in the UDP checksum ( in the correct order! )
        mov     ax,[checkResult]

        ; If the UDP checksum computes to 0, we must make it 0xffff
        ; (0 is reserved for 'not used')
        cmp     ax,0
        jne     sw_001
        mov     ax,0xffff

sw_001:
        mov     [rdx + 20 + 6],ah
        mov     [rdx + 20 + 7],al

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

        ; Check destination IP address.
        ; If it is the local host IP, route it back to IP_RX

        pop     rbx
        mov     rax,NET1OUT_QUEUE

        mov     rcx,[ rdx + 16]
        mov     rdx,[stack_ip]
        cmp     rdx,rcx
        jne     sw_notlocal
        mov     rax,IPIN_QUEUE

sw_notlocal:
        ; Send it.
        call    queue

        xor     rax,rax

sw_exit:
        ret



;***************************************************************************
;   Function
;      socket_write_tcp
;
;   Description
;       socket in rbx
;       # of bytes to write in rcx
;       pointer to data in rdx
;       returns 0 in eax ok, -1 == failed ( invalid socket, or
;       could not queue IP packet )
;
;***************************************************************************
socket_write_tcp:
        ; First, find the address of the socket descriptor
        shl     rbx, 12
        add     rbx, sockets         ; ebx = address of actual socket

        mov     [sktAddr], ebx

        mov     rax, 0xFFFFFFFF
        ; If the socket is invalid, return with an error code
        cmp     [rbx], dword SOCK_EMPTY
        je      swt_exit

        ; If the sockets window timer is nonzero, do not queue packet
        ; TODO - done
        cmp     [rbx + 72], dword 0
        jne     swt_exit

        mov     rax, EMPTY_QUEUE
        call    dequeue
        cmp     ax, NO_BUFFER
        je      swt_exit

        push    rax

        mov     bl, 0x10        ; ACK

        ; Get the address of the callers data
        mov     rdi,[0x6020]
        add     rdi,0x20
        add     rdx,[rdi]
        mov     rsi, rdx

        pop     rax
        push    rax

        push    rcx
        call    buildTCPPacket
        pop     rcx

        ; Check destination IP address.
        ; If it is the local host IP, route it back to IP_RX

        pop     rbx
        push    rcx
        mov     rax,NET1OUT_QUEUE

        mov     rdx,[stack_ip]
        mov     rcx,[ sktAddr ]
        mov     rcx,[ rcx + 16 ]
        cmp     rdx,rcx
        jne     swt_notlocal
        mov     rax,IPIN_QUEUE

swt_notlocal:
        pop     rcx

        push    rbx                 ; save ipbuffer number

        call    queue

        mov     rsi,[sktAddr]

        ; increament SND.NXT in socket
        ; Amount to increment by is in rcx
        add     rsi,48
        call    add_inet_esi

        pop     rbx

        ; Copy the IP buffer to a resend queue
        ; If there isn't one, dont worry about it for now
        mov     rsi,resendQ
        mov     rcx,0

swt003:
        cmp     rcx,NUMRESENDENTRIES
        je      swt001              ; None found
        cmp     [rsi],byte 0xFF
        je      swt002              ; found one
        inc     rcx
        add     rsi,4
        jmp     swt003

swt002:
        push    rbx

        ; OK, we have a buffer descriptor ptr in rsi.
        ; resend entry # in rcx
        ;  Populate it
        ;  socket #
        ;  retries count
        ;  retry time
        ;  fill IP buffer associated with this descriptor

        mov     rax,[sktAddr]
        sub     rax,sockets
        shr     rax,12             ; get skt #
        mov     [rsi],al
        mov     [rsi + 1],byte TCP_RETRIES
        mov     [rsi + 2],word TCP_TIMEOUT

        inc     rcx
        ; Now get buffer location, and copy buffer across. argh! more copying,,
        mov     rdi,resendBuffer - IPBUFFSIZE
swt002a:
        add     rdi,IPBUFFSIZE
        loop    swt002a

        ; we have dest buffer location in rdi
        pop     rax
        ; convert source buffer pointer eax to the absolute address
        mov     rcx,IPBUFFSIZE
        mul     rcx
        add     rax,IPbuffs
        mov     rsi,rax

        ; do copy
        mov     rcx,IPBUFFSIZE
        cld
        rep     movsb

        inc     dword [arp_rx_count] ; ************ TEST ONLY!

swt001:
        xor     rax,rax

swt_exit:
        ret



; Below, the main network layer source code is included
;

include "QUEUE.INC"
include "IP.INC"
include "TCP.INC"
include "UDP.INC"
include "ETHERNET.INC"
