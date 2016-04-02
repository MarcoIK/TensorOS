;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                 ;;
;;  ETHERNET.INC                                                   ;;
;;                                                                 ;;
;;  Ethernet network layer for Tensor OS                           ;;
;;                                                                 ;;
;;  Version 0.4  22 September 2013                                 ;;
;;                                                                 ;;
;;  This file contains the following:                              ;;
;;      PCI bus scanning for valid devices                         ;;
;;      Table of supported ethernet drivers                        ;;
;;      Code to identify and activate a supported driver           ;;
;;      ARP handler                                                ;;
;;      Driver interface to the IP layer                           ;;
;;      Gateway support                                            ;;
;;                                                                 ;;
;;  Individual driver files are included here                      ;;
;;                                                                 ;;
;;  The PCI bus scanning code was ported from the etherboot        ;;
;;  5.0.6 project. The copyright statement for that code is        ;;
;;                                                                 ;;
;;          GNU GENERAL PUBLIC LICENSE                             ;;
;;             Version 2, June 1991                                ;;
;;                                                                 ;;
;;  remaining parts Copyright 2012 Marco K                         ;;
;;   oldklickers@gmail.com                                         ;;
;;                                                                 ;;
;;  See file COPYING for details                                   ;;
;;                                                                 ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;********************************************************************
;   Interface
;      ethernet_driver   called by stack_handler in stack.inc
;      eth_probe         called by app_stack_handler in stack.inc
;
;********************************************************************

; Some useful information on data structures

;     Ethernet Packet - ARP Request example
;
;   0                   1                   2                   3
;   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
;
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |       Dest   H/W Address                                      |
;   |                    ( 14 byte header )                         |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |                               |     Source     H/W Address    |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |                                                               |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |    Protocol - ARP 08  06      |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |  H/W Type  00           01    |  Protocol Type   08 00        |
;   |                   ( ARP Request packet )                      |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   | HLen    0x06  | PLen    0x04  |    OpCode        00   01      |
;   |               ( 0001 for request, 0002 for reply )            |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   | Source Hardware Address ( MAC Address )                       |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |                               |  Source IP Address            |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |                               | Destination Hardware Address  |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   |                                                               |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;   | Destination IP Address                                        |
;   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

; Include individual drivers source files at this point.
; If you create a new driver, include it below.

include "RTL8029.INC"
include "I8255X.INC"
include "RTL8139.INC"
include "3C59X.INC"
include "SIS900.INC"
include "PCNET32.INC"

; DEBUGGING_STATE enables or disables output of received and transmitted
; data over the serial port
DEBUGGING_ENABLED           equ     1
DEBUGGING_DISABLED          equ     0
DEBUGGING_STATE             equ     DEBUGGING_DISABLED

; PCICards
; ========
; PCI vendor and hardware types for hardware supported by the above drivers
; If you add a driver, ensure you update this datastructure, otherwise the
; card will not be probed.
; Each driver is defined by 4 double words. These are
;   PCIVendorDevice  probeFunction ResetFunction PollFunction transmitFunction
; The last entry must be kept at all zeros, to indicate the end of the list
; As a PCI driver may support more than one hardware implementation, there may
; be several lines which refer to the same functions.
; The first driver found on the PCI bus will be the one used.

PCICARDS_ENTRY_SIZE         equ     20    ; Size of each PCICARDS entry

PCICards:
dd  0x12098086, I8255x_probe, I8255x_reset, I8255x_poll, I8255x_transmit
dd  0x10298086, I8255x_probe, I8255x_reset, I8255x_poll, I8255x_transmit
dd  0x12298086, I8255x_probe, I8255x_reset, I8255x_poll, I8255x_transmit
dd  0x10308086, I8255x_probe, I8255x_reset, I8255x_poll, I8255x_transmit
dd  0x24498086, I8255x_probe, I8255x_reset, I8255x_poll, I8255x_transmit
dd  0x802910ec, rtl8029_probe, rtl8029_reset, rtl8029_poll, rtl8029_transmit
dd  0x12111113, rtl8029_probe, rtl8029_reset, rtl8029_poll, rtl8029_transmit
dd  0x813910ec, rtl8139_probe, rtl8139_reset, rtl8139_poll, rtl8139_transmit
dd  0x590010b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x592010b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x597010b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x595010b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x595110b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x595210b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x900010b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x900110b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x900410b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x900510b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x900610b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x900A10b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x905010b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x905110b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x905510b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x905810b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x905A10b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x920010b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x980010b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x980510b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x764610b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x505510b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x605510b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x605610b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x5b5710b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x505710b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x515710b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x525710b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x656010b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x656210b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x656410b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x450010b7, e3c59x_probe, e3c59x_reset, e3c59x_poll, e3c59x_transmit
dd  0x09001039, SIS900_probe, SIS900_reset, SIS900_poll, SIS900_transmit
dd  0x20001022, pcnet32_probe, pcnet32_reset, pcnet32_poll, pcnet32_xmit
dd  0x26251022, pcnet32_probe, pcnet32_reset, pcnet32_poll, pcnet32_xmit
dd  0x20011022, pcnet32_probe, pcnet32_reset, pcnet32_poll, pcnet32_xmit
; following card is untested
dd  0x70161039, SIS900_probe, SIS900_reset, SIS900_poll, SIS900_transmit
dd  0,0,0,0,0  ; end of list marker, do not remove

; PCI Bus defines
PCI_HEADER_TYPE             equ     0x0e  ;8 bit
PCI_BASE_ADDRESS_0          equ     0x10  ;32 bit
PCI_BASE_ADDRESS_5          equ     0x24  ;32 bits
PCI_BASE_ADDRESS_SPACE_IO   equ     0x01
PCI_VENDOR_ID               equ     0x00  ;16 bit
PCI_BASE_ADDRESS_IO_MASK    equ     0xFFFFFFFC

ETHER_IP                    equ     0x0008      ; Reversed from 0800 for intel
ETHER_ARP                   equ     0x0608      ; Reversed from 0806 for intel
ETHER_RARP                  equ     0x3580
ARP_REQ_OPCODE              equ     0x0100
ARP_REP_OPCODE              equ     0x0200

arp_rx_count:       dd  0
ip_rx_count:        dd  0
dumped_rx_count:    dd  0
ip_tx_count:        dd  0
node_addr:          db  0,0,0,0,0,0
eth_rx_data_len:    dw  0
eth_status:         dd  0
io_addr:            dd  0
hdrtype:            db  0
vendor_device:      dd  0
pci_data:           dd  0
pci_dev:            dd  0
pci_bus:            dd  0

; These will hold pointers to the selected driver functions
drvr_probe:         dd  0
drvr_reset:         dd  0
drvr_poll:          dd  0
drvr_transmit:      dd  0

; These hold the destination Host identity for ARP responses
remote_ip_add:      dd  0
remote_hw_add:      db  0, 0, 0, 0, 0, 0
broadcast_add:      db  0xff,0xff,0xff,0xff,0xff,0xff

; This is used by getMACfromIP
MACAddress:         db  0,0,0,0,0,0
gateway_ip:         db  0, 0, 0, 0
subnet_mask:        dd  0x00ffffff
dns_ip:             dd  0

; The follow is the ARP Table.
; This table must be manually updated and the kernel recompilied if
; changes are made to it.
; ARP_TABLE_SIZE defines the size of the table
; ARP_TABLE_ENTRIES defines the number of entries in the table
; Each entry is 10 bytes: 4 Byte IP address, 6 byte MAC Address,
;                         2 bytes status, 2 bytes TTL ( in seconds )
; Empty entries are filled with zeros
; The TTL field is decremented every second, and is deleted when it
; reaches 0. It is refreshed every time a packet is received
; If the TTL field is 0xFFFF it is a permanent entry and is never deleted
; The status field can be the following values
; 0x0000  entry not used
; 0x0001  entry holds a valid mapping
; 0x0002  entry contains an IP address, awaiting ARP response
; 0x0003  No response received to ARP request.
; The last status value is provided to allow the network layer to delete
; a packet that is queued awaiting an ARP response

ARP_NO_ENTRY                equ 0
ARP_VALID_MAPPING           equ 1
ARP_AWAITING_RESPONSE       equ 2
ARP_RESPONSE_TIMEOUT        equ 3

ARP_ENTRY_SIZE              equ     14          ; Number of bytes per entry
ARP_TABLE_SIZE              equ     20          ; Size of table
ARP_TABLE_ENTRIES           equ     0           ; Inital, hardcoded entries

ARPTable:
times ( ARP_TABLE_SIZE - ARP_TABLE_ENTRIES ) * ARP_ENTRY_SIZE  db 0

NumARP:        db    ARP_TABLE_ENTRIES

;***************************************************************************
;   Function
;      eth_probe
;   Description
;      Searches for an ethernet card. If found, the card is enabled and
;      the ethernet -> IP link established
;
;      This function scans the PCI bus looking for a supported device.
;      ISA bus is currently not supported.
;
;        rax is 0 if no hardware found
;***************************************************************************
eth_probe:
        ; Find a card on the PCI bus, and get it's address
        call    scan_bus                    ; Find the ethernet cards PIC address
        xor     rax,rax
        cmp     [io_addr],eax
        je      ep_00x                      ; Return 0 in eax if no cards found

        call    qword [drvr_probe]          ; Call the drivers probe function

        mov     rax,[io_addr]              ; return a non zero value

ep_00x:
        ret

;***************************************************************************
;   Function
;      ethernet_driver
;
;   Description
;       The ethernet RX and TX handler
;       This is a kernel function, called by stack_handler
;
;***************************************************************************
ethernet_driver:
    ; Do nothing if the driver is inactive
    cmp     [ethernet_active], byte 0
    je      eth_exit

    call    eth_rx
    call    eth_tx

eth_exit:
    ret

;***************************************************************************
;   Function
;      eth_rx
;
;   Description
;      Polls the ethernet card for received data. Extracts if present
;       Depending on the Protocol within the packet:
;         ARP : Pass to ARP_handler. This may result in an ARP reply
;               being tx'ed
;         IP  : Store in an IP buffer
;
;***************************************************************************
eth_rx:
    xor     ax, ax
    mov     [eth_rx_data_len], ax
    call    qword [drvr_poll]       ; Call the drivers poll function

    mov     ax, [eth_rx_data_len]
    cmp     ax, 0
    je      erx_exit

if DEBUGGING_STATE = DEBUGGING_ENABLED
    pushdu
    mov     rax, 0                    ;Indicate that this is a received packet
    mov     cx, [eth_rx_data_len]
    mov     rsi, Ether_buffer
    cmp     word [rsi + 12], ETHER_IP
    jnz     erxd_done
;    cmp     byte [esi + 14 + 9], 0x06  ; TCP
;    jnz     erxd_done
    call    eth_dump
erxd_done:
    popdu
end if

    ; Check the protocol. Call appropriate handler
    mov     rax, Ether_buffer
    add     rax, 12                  ; The address of the protocol word

    mov     ax, [rax]

    cmp     ax, ETHER_ARP
    je      erx_001                  ; It is ARP

    cmp     ax, ETHER_IP
    je      erx_002                  ; It's IP

;    inc     dword [dumped_rx_count]

    jmp     erx_exit               ; If not IP or ARP, ignore

erx_001:
    mov     rax, [arp_rx_count]
    inc     rax
    mov     [arp_rx_count], eax

    ; At this point, the packet is still in the Ether_buffer
    call    arp_handler

    jmp     erx_exit

erx_002:
    mov     rax, [ip_rx_count]
    inc     rax
    mov     [ip_rx_count], eax

    ; Check to see if the MAC address is in our arp table
    ; refresh the arp ttl if so

    mov     rsi, Ether_buffer
    add     rsi, 6

    call    refreshARP

    call    ether_IP_handler

    jmp     erx_exit

erx_exit:
    ret

;***************************************************************************
;   Function
;      eth_tx
;
;   Description
;      Looks at the NET1OUT_QUEUE for data to send.
;      Stores that destination IP in a location used by the tx routine
;      Looks up the MAC address in the ARP table; stores that where
;      the tx routine can get it
;      Get the length of the data. Store that where the tx routine wants it
;      Call tx
;      Places buffer on empty queue when the tx routine finished
;
;***************************************************************************
eth_tx:
    ; Look for a buffer to tx
    mov     rax, NET1OUT_QUEUE
    call    dequeue
    cmp     ax, NO_BUFFER
    je      eth_exit            ; Exit if no buffer available

    push    rax

    ; convert buffer pointer eax to the absolute address
    mov     rcx, IPBUFFSIZE
    mul     rcx
    add     rax, IPbuffs

    ; Extract the destination IP
    ; find the destination IP in the ARP table, get MAC
    ; store this MAC in 'MACAddress'
    mov     rbx, rax               ; Save buffer address
    mov     rdx, [rbx + 16]        ; get destination address

    ; If the destination address is 255.255.255.255,
    ; set the MACAddress to all ones ( broadcast )
    mov     [MACAddress], dword 0xffffffff
    mov     [MACAddress + 4], word 0xffff
    push    r8
    mov     r8,0xffffffff
    cmp     rdx,r8
    pop     r8
    je      etx_send                ; If it is broadcast, just send

    call    getMACfromIP           ; Get the MAC address.

    cmp     rax, ARP_VALID_MAPPING
    jz      etx_send

    ; No valid entry. Are we waiting for a response?
    cmp     rax, ARP_AWAITING_RESPONSE
    jne     etx_001

    ; Re-queue the packet, and exit
    pop     rbx
    mov     rax, NET1OUT_QUEUE
    call    queue
    jmp     etx_exit

etx_001:
    ; HAs the request been sent, but timed out?
    cmp     rax, ARP_RESPONSE_TIMEOUT
    jne     etx_002

    pop     rax
    call    freeBuff
    jmp     etx_exit

etx_002:
    ; There is no entry. Re queue the request, and ask ARP to send a request

    ; IP address is in rdx
    push    rdx
    call    arp_request
    pop     rbx

    ; Add an entry in the ARP table, awaiting response

    cmp     byte [NumARP], ARP_TABLE_SIZE
    je      etx_003            ; We cannot add a new entry in the table

    inc     byte [NumARP]

    movzx   rax, byte [NumARP]
    mov     rcx, ARP_ENTRY_SIZE
    mul     rcx
    sub     rax, ARP_ENTRY_SIZE

    mov     [rax + ARPTable], ebx
    xor     rbx, rbx
    mov     [rax + ARPTable + 4], ebx
    mov     [rax + ARPTable + 8], bx

    ; set the status field up - awaiting response
    mov     cl, 0x00
    mov     [rax + ARPTable + 10], cl
    mov     cl, 0x02
    mov     [rax + ARPTable + 11], cl

    ; Initialise the time to live field - 10s
    mov     cx, 0x000A
    mov     [rax + ARPTable + 12], cx

etx_003:
    pop     rbx                        ; Get the buffer back
    mov     rax, NET1OUT_QUEUE
    call    queue
    jmp     etx_exit

etx_send:
    xor     rcx, rcx
    mov     ch, [rbx+2]
    mov     cl, [rbx+3]          ; ; Size of IP packet to send

    mov     rsi, rbx

    mov     rdi, MACAddress

if DEBUGGING_STATE = DEBUGGING_ENABLED
    pushdu
    mov     cx, 42
    mov     rax, 1                    ; Indicate that this is a tx packet
    call    eth_dump
    popa
end if

    mov     bx, ETHER_IP
    call    qword [drvr_transmit]       ; Call the drivers transmit function

    ; OK, we have sent a packet, so increment the count
    inc     qword [ip_tx_count]

    ; And finally, return the buffer to the free queue
    pop     rax
    call    freeBuff

etx_exit:
    ret

;***************************************************************************
;   Function
;      ether_IP_handler
;
;   Description
;      Called when an IP ethernet packet is received on the ethernet
;      Header + Data is in Ether_buffer[]
;      We just need to get a buffer from the 'free' queue, and
;      store the packet in it, then insert the packet number into the
;      IPRX queue.
;      If no queue entry is available, the packet is silently discarded
;      All registers may be destroyed
;
;***************************************************************************
ether_IP_handler:
    mov     rax, EMPTY_QUEUE
    call    dequeue
    cmp     ax, NO_BUFFER
    je      eiph00x

    ; convert buffer pointer eax to the absolute address
    push    rax
    mov     rcx, IPBUFFSIZE
    mul     rcx
    add     rax, IPbuffs

    mov     rdi, rax

    ; get a pointer to the start of the DATA
    mov     rsi, Ether_buffer + 14

    ; Now store it all away
    mov     rcx, IPBUFFSIZE / 4     ; Copy all of the available
                              ; data across - worse case
    cld
    rep     movsd

    ; And finally, place the buffer in the IPRX queue
    pop     rbx
    mov     rax, IPIN_QUEUE
    call    queue

eiph00x:
    ret

;***************************************************************************
;
;  ARP CODE FOLLOWS
;
;  The ARP code is used by ethernet drivers to translate an destination
;  IP address into an ethernet hardware address. Functions to broadcast
;  requests and handle response are (or will be) here.
;  The IP layer has no knowledge of ARP, as this is a network interface
;  issue
;
;***************************************************************************

;***************************************************************************
;   Function
;      arp_timer
;
;   Description
;      Called every 1s
;      It is responsible for removing expired routes
;      All registers may be destroyed
;
;***************************************************************************
arp_timer:
    ; loop through all the ARP entries, decrementing each one
    ; that doesn't have a TTL of 0xFFFF
    movzx   rax, byte [NumARP]

arp_001:
    cmp     rax, 0
    je      arp_003

    push    rax
    dec     rax
    mov     rcx, ARP_ENTRY_SIZE
    mul     rcx
    cmp     word [ rax + ARPTable + 12], 0xFFFF
    je      arp_002

    cmp     word [ rax + ARPTable + 12], 0
    je      arp_002

    dec     word [rax + ARPTable + 12]

arp_002:
    pop     rax
    dec     rax
    jmp     arp_001

    ; Now, look for entries with a TTL of 0
    ; Valid entries and response timeout entries get removed
    ; awaiting response gets converted into a response timeout, with a
    ; short life time - this allows queued packets to be flushed
arp_003:
    movzx   rdx, byte [NumARP]
    cmp     rdx, 0
    je      arp_exit

    ; EDX holds the # of entries to search through
    mov     rax, 0

arp_005:
    cmp     word [ rax + ARPTable + 12], 0
    jne     arp_004

    ; If it's status code is 0001 or 0003, delete the entry
    cmp     word [rax + ARPTable + 10], 0x0100
    je      arp_007
    cmp     word [rax + ARPTable + 10], 0x0300
    je      arp_007

    ; The only other valid code is 0002 - indicating a
    ; timeout while waiting for a response. Change the
    ; entry to response timed out

    mov     [rax + ARPTable + 10], word 0x0300
    mov     [rax + ARPTable + 12], word 0x000A
    jmp     arp_004

arp_007:
    ; Delete this entry
    mov     rdi, ARPTable
    add     rdi, rax
    mov     rsi, rdi
    add     rsi, ARP_ENTRY_SIZE

    mov     rcx, (ARP_TABLE_SIZE - 1) * ARP_ENTRY_SIZE
    sub     rcx, rax

    rep     movsb

    dec     byte [NumARP]
    jmp     arp_006

arp_004:
    add     rax, ARP_ENTRY_SIZE
arp_006:
    dec     rdx
    cmp     rdx, 0
    jne     arp_005

arp_exit:
    ret

;***************************************************************************
;   Function
;      arp_request
;
;   Description
;      Sends an ARP request on the ethernet
;        The requested IP address is in rdx
;      All registers may be destroyed
;
;***************************************************************************
arp_request:
    mov     rbx, Ether_buffer
    mov     ax, 0x0100
    mov     [rbx], ax
    add     rbx, 2

    mov     ax, 0x0008
    mov     [rbx], ax
    add     rbx, 2

    mov     ax, 0x0406
    mov     [rbx], ax
    add     rbx, 2

    mov     ax, 0x0100
    mov     [rbx], ax
    add     rbx, 2

    mov     rcx, node_addr
    mov     rax, [rcx]
    mov     [rbx], eax
    add     rcx, 4
    add     rbx, 4
    mov     ax, [rcx]
    mov     [rbx], ax
    add     rbx, 2
    mov     rax, [stack_ip]
    mov     [rbx], eax
    add     rbx, 4

    xor     rax, rax
    mov     [rbx], eax
    add     rbx, 4
    mov     [rbx], ax

    add     rbx, 2
    mov     [rbx], rdx

    ; Now, send it!

    ; Pointer to 48 bit destination address in rdi
    ; Type of packet in bx
    ; size of packet in rcx
    ; pointer to packet data in rsi
    mov      rdi, broadcast_add

;if DEBUGGING_STATE = DEBUGGING_ENABLED
;    pushdu
;    mov     rax, 1                    ; Indicate that this is a tx packet
;    mov     rcx, 28
;   mov      rsi, Ether_buffer
;    call    eth_dump
;    popdu
;end if

    mov     bx, ETHER_ARP
    mov     rcx, 28
    mov     rsi, Ether_buffer
    call    qword [drvr_transmit]       ; Call the drivers transmit function
    ret

;***************************************************************************
;   Function
;      arp_handler
;
;   Description
;      Called when an ARP packet is received on the ethernet
;      Header + Data is in Ether_buffer[]
;       It looks to see if the packet is a request to resolve this Hosts
;       IP address. If it is, send the ARP reply packet.
;      This Hosts IP address is in dword [stack_ip]  ( in network format )
;       This Hosts MAC address is in node_addr[6]
;      All registers may be destroyed
;
;***************************************************************************
arp_handler:
    ; Is this a REQUEST?
    ; Is this a request for My Host IP
    ; Yes - So construct a response message.
    ; Send this message to the ethernet card for transmission

    mov     rbx, Ether_buffer

    mov     rdx, rbx
    add     rdx, 20
    mov     ax, [rdx]
    cmp     ax, ARP_REQ_OPCODE      ; Is this a request packet?
    jne     arph_resp            ; No - so test for response

    mov     rdx, rbx
    add     rdx, 38
    mov     rax, [rdx]

    cmp     rax, [stack_ip]         ; Is it looking for my IP address?
    jne     arph_exit            ; No - so quit now

    ; OK, it is a request for my MAC address. Build the frame and send it

    ; Save the important data from the original packet
    ; remote MAC address first
    mov     rcx, remote_hw_add
    mov     rdx, rbx
    add     rdx, 22               ; edx points to Source h/w address
    mov     rax, [rdx]
    mov     [rcx], eax
    add     rdx, 4
    add     rcx, 4
    mov     ax, [rdx]
    mov     [rcx],ax

    ; and also the remote IP address
    add     rdx, 2
    mov     rax,[rdx]
    mov     [remote_ip_add], eax

    ; So now we can reuse the packet. ebx still holds the address of
    ; the header + packet
    ; We dont need the header ( first 14 bytes )

    mov     rdx, rbx
    add     rdx, 20
    mov     ax, ARP_REP_OPCODE
    mov     [rdx], ax
    add     rdx, 2

    mov     rcx, node_addr
    mov     rax, [rcx]
    mov     [rdx], eax
    add     rcx, 4
    add     rdx, 4
    mov     ax, [rcx]
    mov     [rdx], ax
    add     rdx, 2
    mov     rax, [stack_ip]
    mov     [rdx], eax
    add     rdx, 4
    mov     rcx, remote_hw_add
    mov     rax, [rcx]
    mov     [rdx], eax
    add     rcx, 4
    add     rdx, 4
    mov     ax, [rcx]
    mov     [rdx], ax

    add     rdx, 2
    mov     rax, [remote_ip_add]
    mov     [rdx], eax

    ; Now, send it!

    ; Pointer to 48 bit destination address in rdi
    ; Type of packet in bx
    ; size of packet in rcx
    ; pointer to packet data in rsi
    mov     rdi, remote_hw_add

;if DEBUGGING_STATE = DEBUGGING_ENABLED
;    pusha
;    mov     rax, 1                    ; Indicate that this is a tx packet
;    mov     rcx, 28
;   mov      rsi, Ether_buffer + 14
 ;   call    eth_dump
;    popa
;end if

    mov     bx, ETHER_ARP
    mov     rcx, 28
    mov     rsi, Ether_buffer + 14
    call    qword [drvr_transmit]       ; Call the drivers transmit function
    jmp     arph_exit

arph_resp:
    cmp     ax, ARP_REP_OPCODE      ; Is this a replypacket?
    jne     arph_exit               ; No - so quit

    ; This was a reply, probably directed at me.
    ; save the remotes MAC & IP
    mov     rcx, remote_hw_add
    mov     rdx, rbx
    add     rdx, 22               ; edx points to Source h/w address
    mov     rax, [rdx]
    mov     [rcx], eax
    add     rdx, 4
    add     rcx, 4
    mov     ax, [rdx]
    mov     [rcx],ax

    ; and also the remote IP address
    add     rdx, 2
    mov     rax,[rdx]
    mov     [remote_ip_add], eax

    ; Now, add an entry in the table for this IP address if it doesn't exist

    push    rax
    movzx   rax, byte [NumARP]
    mov     rcx, ARP_ENTRY_SIZE
    mul     rcx
    pop     rdx
    movzx   rcx, byte [NumARP]
    cmp     rcx, 0
    je      arph_002

arph_001:
    sub     rax, ARP_ENTRY_SIZE
    cmp     [rax + ARPTable], edx
    loopnz  arph_001                      ; Return back if non match

    jnz     arph_002                   ; None found, add to end

    mov     rcx, [remote_hw_add]
    mov     [rax + ARPTable + 4], ecx
    mov     cx, [remote_hw_add+4]
    mov     [rax + ARPTable + 8], cx

    ; specify the type - a valid entry
    mov     cl, 0x00
    mov     [rax + ARPTable + 10], cl
    mov     cl, 0x01
    mov     [rax + ARPTable + 11], cl

    ; Initialise the time to live field - 1 hour
    mov     cx, 0x0E10
    mov     [rax + ARPTable + 12], cx
    jmp     arph_exit

arph_002:

    cmp     byte [NumARP], ARP_TABLE_SIZE
    je      arph_exit

    inc     byte [NumARP]

    movzx   rax, byte [NumARP]
    mov     rcx, ARP_ENTRY_SIZE
    mul     rcx
    sub     rax, ARP_ENTRY_SIZE

    mov     rcx, [remote_ip_add]
    mov     [rax + ARPTable], ecx
    mov     rcx, [remote_hw_add]
    mov     [rax + ARPTable + 4], ecx
    mov     cx, [remote_hw_add+4]
    mov     [rax + ARPTable + 8], cx

    mov     cl, 0x00
    mov     [rax + ARPTable + 10], cl
    mov     cl, 0x01
    mov     [rax + ARPTable + 11], cl

    ; Initialise the time to live field - 1 hour
    mov     cx, 0x0E10
    mov     [rax + ARPTable + 12], cx

arph_exit:
    ret

; pointer to MAC in esi
refreshARP:
    mov     rbx, [rsi]
    mov     dx, [rsi+4]
    push    rdx
    movzx   rax, byte [NumARP]
    mov     rcx, ARP_ENTRY_SIZE
    mul     rcx
    pop     rdx
    movzx   rcx, byte [NumARP]
    cmp     rcx, 0
    je      rf_exit

rf_001:
    sub     rax, ARP_ENTRY_SIZE
    cmp     [rax + ARPTable+4], ebx

    je      rf_002
    loop    rf_001
    jmp     rf_exit

rf_002:
    cmp     [rax + ARPTable+8], dx
    je      rf_gotone
    loop    rf_001
    jmp     rf_exit

rf_gotone:
    ; Initialise the time to live field - 1 hour
    mov     cx, 0x0E10
    mov     [rax + ARPTable + 12], cx

rf_exit:
    ret

;***************************************************************************
;   Function
;      getMACfromIP
;
;   Description
;       Takes an IP address in edx and scans the ARP table for
;        a matching entry
;       If a match is found, it's MAC address is stored in MACAddress.
;      Otherwise the value 0 is writen to MACAddress
;      rax holds ARP table entry status code ( ARP_ )
;      rbx unchanged
;
;***************************************************************************
getMACfromIP:
    ; first, check destination IP to see if it is on 'this' network.
    ; The test is:
    ; if ( destIP & subnet_mask == stack_ip & subnet_mask )
    ;   desitnation is local
    ; else
    ;  destination is remote, so pass to gateway

    mov     rax, rdx
    and     rax, [subnet_mask]
    mov     rcx, [stack_ip]
    and     rcx, [subnet_mask]
    cmp     rax, rcx
    je      gm0

    mov     rdx, [gateway_ip]
gm0:
    push    rdx
    xor     rax, rax
    mov     [MACAddress], eax
    mov     [MACAddress + 4], ax

    movzx   rax, byte [NumARP]
    mov     rcx, ARP_ENTRY_SIZE
    mul     rcx

    pop     rdx

    movzx   rcx, byte [NumARP]
    cmp     rcx, 0
    je      gm_none
gm1:
    sub     rax, ARP_ENTRY_SIZE
    cmp     [rax + ARPTable], edx
    loopnz  gm1                      ; Return back if non match
    jnz     gm_none                   ; Quit if none found

    ; eax holds index
    mov     rcx, [rax + ARPTable + 4]
    mov     [MACAddress], ecx
    mov     cx, [rax + ARPTable + 8]
    mov     [MACAddress+4], cx

    ; Return the entry status in eax
    mov     ch, [rax + ARPTable + 10]
    mov     cl, [rax + ARPTable + 11]
    movzx   rax, cx
    jmp     gm_exit

gm_none:
    mov     rax, ARP_NO_ENTRY

gm_exit:
    ret

;***************************************************************************
;
;  PCI CODE FOLLOWS
;
;  the following functions provide access to the PCI interface.
;  These functions are used by scan_bus, and also some ethernet drivers
;
;***************************************************************************

;***************************************************************************
;   Function
;      config_cmd
;
;   Description
;       creates a command dword  for use with the PCI bus
;       bus # in rbx
;      devfn in rcx
;       where in rdx
;
;      command dword returned in rax
;       Only rax destroyed
;***************************************************************************
config_cmd:
    push    rcx
    mov     rax, rbx
    shl     rax, 16
    push    r8
    mov     r8, 0x80000000
    or      rax, r8
    pop     r8
    shl     rcx, 8
    or      rax, rcx
    pop     rcx
    or      rax, rdx
    push    r8
    mov     r8, 0xFFFFFFFC
    and     rax, r8
    pop     r8
    ret

;***************************************************************************
;   Function
;      pcibios_read_config_byte
;
;   Description
;       reads a byte from the PCI config space
;       bus # in rbx
;      devfn in rcx
;       where in rdx ( ls 16 bits significant )
;
;      byte returned in al ( rest of rax zero )
;       Only rax/rdx destroyed
;***************************************************************************
pcibios_read_config_byte:
    call    config_cmd
    push    dx
    mov     dx, 0xCF8
    out     dx, eax
    pop     dx

    xor     rax, rax
    and     dx, 0x03
    add     dx, 0xCFC
;   and     dx, 0xFFC
    in      al, dx
    ret

;***************************************************************************
;   Function
;      pcibios_read_config_word
;
;   Description
;       reads a word from the PCI config space
;       bus # in rbx
;      devfn in rcx
;       where in rdx ( ls 16 bits significant )
;
;      word returned in ax ( rest of rax zero )
;       Only rax/rdx destroyed
;***************************************************************************
pcibios_read_config_word:
    call    config_cmd
    push    dx
    mov     dx, 0xCF8
    out     dx, eax
    pop     dx

    xor     rax, rax
    and     dx, 0x02
    add     dx, 0xCFC
;   and     dx, 0xFFC
    in      ax, dx
    ret

;***************************************************************************
;   Function
;      pcibios_read_config_dword
;
;   Description
;       reads a dword from the PCI config space
;       bus # in rbx
;      devfn in rcx
;       where in rdx ( ls 16 bits significant )
;
;      dword returned in rax
;       Only rax/rdx destroyed
;***************************************************************************
pcibios_read_config_dword:
    push    rdx
    call    config_cmd
    push    dx
    mov     dx, 0xCF8
    out     dx, eax
    pop     dx
    xor     rax, rax
    mov     dx, 0xCFC
    in      eax, dx
    pop     rdx
    ret

;***************************************************************************
;   Function
;      pcibios_write_config_byte
;
;   Description
;       write a byte in al to the PCI config space
;       bus # in rbx
;      devfn in rcx
;       where in rdx ( ls 16 bits significant )
;
;       Only rax/rdx destroyed
;***************************************************************************
pcibios_write_config_byte:
    push    ax
    call    config_cmd
    push    dx
    mov     dx, 0xCF8
    out     dx, eax
    pop     dx
    pop     ax

    and     dx, 0x03
    add     dx, 0xCFC
    out     dx, al
    ret

;***************************************************************************
;   Function
;      pcibios_write_config_word
;
;   Description
;       write a word in ax to the PCI config space
;       bus # in rbx
;      devfn in rcx
;       where in rdx ( ls 16 bits significant )
;
;       Only rax/rdx destroyed
;***************************************************************************
pcibios_write_config_word:
    push    ax
    call    config_cmd
    push    dx
    mov     dx, 0xCF8
    out     dx, eax
    pop     dx
    pop     ax

    and     dx, 0x02
    add     dx, 0xCFC
    out     dx, ax
    ret

;***************************************************************************
;   Function
;      delay_us
;
;   Description
;       delays for 30 to 60 us
;
;        I would prefer this routine to be able to delay for
;       a selectable number of microseconds, but this works for now.
;
;       If you know a better way to do 2us delay, pleae tell me!
;***************************************************************************
delay_us:
    push    rax
    push    rcx

    mov     rcx,2

    in      al,0x61
    and     al,0x10
    mov     ah,al
    cld

dcnt1:
    in      al,0x61
    and     al,0x10
    cmp     al,ah
    jz      dcnt1

    mov     ah,al
    loop    dcnt1

    pop     rcx
    pop     rax

    ret

;***************************************************************************
;   Function
;      scan_bus
;
;   Description
;       Scans the PCI bus for a supported device
;        If a supported device is found, the drvr_ variables are initialised
;       to that drivers functions ( as defined in the PCICards table)
;
;        io_addr   holds card I/O space. 32 bit, but only LS 16 bits valid
;        pci_data  holds the PCI vendor + device code
;       pci_dev   holds PCI bus dev #
;       pci_bus   holds PCI bus #
;
;        io_addr will be zero if no card found
;
;***************************************************************************
scan_bus:
    xor     rax, rax
    mov     [hdrtype], al
    mov     [pci_data], eax

    xor     rbx, rbx         ; ebx = bus# 0 .. 255

sb_bus_loop:
    xor     rcx, rcx         ; ecx = devfn# 0 .. 254  ( not 255? )

sb_devf_loop:
    mov     rax, rcx
    and     rax, 0x07

    cmp     rax, 0
    jne     sb_001

    mov     rdx, PCI_HEADER_TYPE
    call    pcibios_read_config_byte
    mov     [hdrtype], al
    jmp     sb_002

sb_001:
    mov     al, [hdrtype]
    and     al, 0x80
    cmp     al, 0x80
    jne     sb_inc_devf

sb_002:
    mov     rdx, PCI_VENDOR_ID
    call    pcibios_read_config_dword
    mov     [vendor_device], eax
    push    r8
    mov     r8,0xffffffff
    cmp     rax,r8
    pop     r8
    je      sb_empty
    cmp     rax, 0
    jne     sb_check_vendor

sb_empty:
    mov     [hdrtype], byte 0
    jmp     sb_inc_devf

sb_check_vendor:
    ; iterate though PCICards until end or match found
    mov     rsi, PCICards

sb_check:
    cmp     [rsi], dword 0
    je      sb_inc_devf                ; Quit if at last entry
    cmp     rax, [rsi]
    je      sb_got_card
    add     rsi, PCICARDS_ENTRY_SIZE
    jmp     sb_check

sb_got_card:
    ; indicate that we have found the card
    mov     [pci_data], eax
    mov     [pci_dev], ecx
    mov     [pci_bus], ebx

    ; Define the driver functions
    push    rax
    mov     rax, [rsi+4]
    mov     [drvr_probe], eax
    mov     rax, [rsi+8]
    mov     [drvr_reset], eax
    mov     rax, [rsi+12]
    mov     [drvr_poll], eax
    mov     rax, [rsi+16]
    mov     [drvr_transmit], eax
    pop     rax

    mov     rdx, PCI_BASE_ADDRESS_0

sb_reg_check:
    call    pcibios_read_config_dword
    mov     [io_addr], eax
    push    r8
    mov     r8, PCI_BASE_ADDRESS_IO_MASK
    and     rax, r8
    pop     r8
    cmp     rax, 0
    je      sb_inc_reg
    mov     rax, [io_addr]
    and     rax, PCI_BASE_ADDRESS_SPACE_IO
    cmp     rax, 0
    je      sb_inc_reg

    mov     rax, [io_addr]
    push    r8
    mov     r8, PCI_BASE_ADDRESS_IO_MASK
    and     rax, r8
    pop     r8
    mov     [io_addr], eax

sb_exit1:
    ret

sb_inc_reg:
    add     rdx, 4
    cmp     rdx, PCI_BASE_ADDRESS_5
    jbe     sb_reg_check

sb_inc_devf:
    inc     rcx
    cmp     rcx, 255
    jb      sb_devf_loop
    inc     rbx
    cmp     rbx, 256
    jb      sb_bus_loop

    ; We get here if we didn't find our card
    ; set io_addr to 0 as an indication
    xor     rax, rax
    mov     [io_addr], eax

sb_exit2:
    ret

;***************************************************************************
;
;  DEBUGGING CODE FOLLOWS
;
;  If debugging data output is not required, ALL code & data below may
;  be removed.
;
;***************************************************************************

if DEBUGGING_STATE = DEBUGGING_ENABLED

;***************************************************************************
;   Function
;      eth_dump
;
;   Description
;       Dumps a tx or rx ethernet packet over the rs232 link
;       This is a debugging routine that seriously slows down the stack.
;       Use with caution.
;
;       Baud rate is 57600, 8n1  com1
;         rax : type (0 == rx, 1 == tx )
;          cx : # of bytes in buffer
;         rsi : address of buffer start
;         rdi : pointer to MACAddress ( tx only )
;
;***************************************************************************
eth_dump:
    pushdu

    ; Set the port to the desired speed
    mov     rbx, 0x3f8                     ; combase

    mov     rdx, rbx
    add     rdx, 3                        ; data format register
    mov     al, 0x80                    ; enable access to divisor latch
    out     dx, al

    mov     rdx, rbx
    add     rdx, 1                        ; interrupt enable register
    mov     al, 0x00                    ; No interruts enabled
    out     dx, al

    mov     rdx, rbx
    mov     al, 0x20 / 16                ; set baud rate to 57600 0x10 =115200
    out     dx, al

    mov     rdx, rbx
    add     rdx, 3                        ; data format register
    mov     al, 0x03                    ; 8 data bits
    out     dx, al

    mov     rdx, rbx
    add     rdx, 4                        ; Modem control register
    mov     al, 0x08                    ; out2 enabled. No handshaking.
    out     dx, al

    mov     rdx, rbx
    add     rdx, 1                        ; interrupt enable register
    mov     al, 0x01                    ; Receive data interrupt enabled,
    out     dx, al

    popdu

    ; First, display the type of the buffer.
    ; If it is a tx buffer, display the macaddress

    pushdu

    cmp     rax, 0
    jne     dd001

    mov     bl, 0x0a
    call    tx_byted
    mov     bl, 0x0d
    call    tx_byted

    ; Output "RX:"
    mov     bl, 'R'
    call    tx_byted
    mov     bl, 'X'
    call    tx_byted
    mov     bl, ':'
    call    tx_byted
    jmp     dump_data

dd001:
    mov     bl, 0x0a
    call    tx_byted
    mov     bl, 0x0d
    call    tx_byted

    ; Output TX: xxxxxxxxxxxx
    mov     bl, 'T'
    call    tx_byted
    mov     bl, 'X'
    call    tx_byted
    mov     bl, ':'
    call    tx_byted
    mov     bl, ' '
    call    tx_byted

    ; Display MAC address
    xor     rax, rax
    mov     al, [rdi]
    shr     al, 4
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    xor     rax, rax
    mov     al, [rdi]
    and     al, 0x0f
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    inc     rdi
    xor     rax, rax
    mov     al, [rdi]
    shr     al, 4
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    xor     rax, rax
    mov     al, [rdi]
    and     al, 0x0f
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    inc     rdi
    xor     rax, rax
    mov     al, [rdi]
    shr     al, 4
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    xor     rax, rax
    mov     al, [rdi]
    and     al, 0x0f
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    inc     rdi
    xor     rax, rax
    mov     al, [rdi]
    shr     al, 4
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    xor     rax, rax
    mov     al, [rdi]
    and     al, 0x0f
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    inc     rdi
    xor     rax, rax
    mov     al, [rdi]
    shr     al, 4
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    xor     rax, rax
    mov     al, [rdi]
    and     al, 0x0f
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    inc     rdi
    xor     rax, rax
    mov     al, [rdi]
    shr     al, 4
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    xor     rax, rax
    mov     al, [rdi]
    and     al, 0x0f
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

dump_data:
    popdu

    ; OK, we come in here with
    ; cx == number of byte to send
    ; esi == buffer start
    ;
dd_000:
    mov     bl, 0x0a
    call    tx_byted
    mov     bl, 0x0d
    call    tx_byted

    mov     rax, 16        ; Number of characters on the line
    mov     rdi, rsi    ; Save first byte position for later

    push    rcx

dd_001:
    push    rax

    ; Print a byte, and a space
    xor     rax, rax
    mov     al, [rsi]
    shr     al, 4
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    xor     rax, rax
    mov     al, [rsi]
    and     al, 0x0f
    mov     bl, [rax + hexchars]
    call    tx_byted ; byte in bl eax ebx edx destroyed

    mov     bl, ' '
    call    tx_byted

    pop     rax

    inc     rsi
    dec     rcx
    cmp     rcx, 0
    je      dd_0011            ; Print the ASCII format

    dec     rax

    cmp     rax, 0
    je      dd_002            ; Print the ASCII format
    jmp     dd_001            ; Print rest of line

dd_0011:
    ; First, complete the 16 bytes of data, by printing spaces
    dec     rax
    cmp     rax, 0
    je      dd_002

    push    rax
    mov     bl, ' '
    call    tx_byted
    mov     bl, ' '
    call    tx_byted
    mov     bl, ' '
    call    tx_byted
    pop     eax
    jmp     dd_0011

dd_002:
    pop     rcx
    mov     rsi, rdi        ; Go back to the start of the line data

    mov     rax, 16

outLineAscii:
    push    rax

    xor     rax, rax
    mov     al, [rsi]
    mov     bl, '.'

    cmp     al, 0x1F
    jle     outAscii
    cmp     al, 0x7e
    jge     outAscii

    mov     bl, al

outAscii:
    call    tx_byted ; byte in bl eax ebx edx destroyed

    pop     rax
    dec     rcx
    inc     rsi
    cmp     rcx, 0
    je      dd_003

    dec     rax
    cmp     rax, 0
    je      dd_003
    jmp     outLineAscii

dd_003:
    cmp     rcx, 0
    je      dd_004
    jmp     dd_000

dd_004:
    ret

;***************************************************************************
;   Function
;      tx_byte
;
;   Description
;       Send a byte in bl out of the com port 1
;       destroys eax, edx
;
;***************************************************************************
tx_byted:
    push    rbx                     ; Save the byte

    mov     rbx, 0x3f8            ; get the com port address

    ; Wait for transmit buffer to empty. This could take 1ms @ 9600baud

    mov     rdx, rbx
    add     rdx, 5

wait_txd:
    in      al, dx                   ; read uart serialisation status
    and     al, 0x40
    cmp     al, 0
    jz      wait_txd                  ; loop until free

    mov     rdx, rbx
    pop     rax                     ; restore the byte to send
    out     dx, al
    ret

; This is used for translating hex to ASCII for display or output
hexchars    db '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'

end if

