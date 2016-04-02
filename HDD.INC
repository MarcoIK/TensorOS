;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                           ;;
;;  HDD.INC                                                  ;;
;;                                                           ;;
;;  ATA, ATAPI, SATA driver code                             ;;
;;                                                           ;;
;;  Version 0.2  March 28th, 2016                            ;;
;;                                                           ;;
;;  Author: Marco K, oldklickers@gmail.com                   ;;
;;    Credits:                                               ;;
;;          only myself                                      ;;
;;                                                           ;;
;;  See file COPYING for details                             ;;
;;                                                           ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

floppy_driver   db      ?       ; 1=floppy drive, 0=non floppy drive
max_track       dw      ?       ; maximum track (cylinder) number
dev_num         db      0x80    ; drive C:
target_drive    db      ?       ; master/slave drive
BUS_IO_Port     dw      ?       ; primary/secondary ATA Bus (gets always 2 port offset)
device_control  dw      ?       ; references device control registers
sectorcount     db      ?       ; number of sectors to r/w
sectornumber    db      ?       ; CHS, LBA specific
cylinder_low    db      ?       ; Partial Disk Sector address
cylinder_high   db      ?
head_port       db      ?       ; drive number to select
command_port    db      ?       ; command for sending to port
heads           db      ?       ; number of heads
sectors_track   dw      ?       ; sectors per track

; chose LBA 28bit addressing mode, if we do not want
; to violate the maximum LBA value limitation.

; we keep storing the memory buffer in 0:7C00 if we are going
; to work with absolute LBA addressing mode.
; It should be stored in the sector offset from the very beginning
; of the disk -- completely ignoring partition boundaries.
LBA:
       .size            db      16      ; size of packet (16 bytes)
       .zero            db      0       ; always 0
       .sectors         dw      ?       ; number of sectors to transfer (max 127 on some BIOses)
       .lbuffer         dd      ?       ; transfer buffer (16 bit segment:16 bit offset)
       .start           dd      ?       ; starting LBA
       .upper           dd      ?       ; used for upper part of 48 bit LBAs

CHS:
        .head           dd      ?
        .track          dd      ?
        .sector         dd      ?

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                                                       ;
;  Desc.:                                                               ;
;        Identify the actual driver. The PIO mode should be by all      ;
;        ATA-compliant drives as the default data transfer mechanism    ;
;        supported.                                                     ;
;                                                                       ;
;        Input:  dx - primary/secondary Master/Slave port               ;
;                                                                       ;
;        Output: bh - 0=non ATA, 1=HDD exist                            ;
;                bl - HDD drive                                         ;
;                     0 -> ATA drive                                    ;
;                     1 -> ATAPI drive                                  ;
;                     2 -> SATA drive                                   ;
;                                                                       ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

check_identity:                         ; send 0 to I/O port registers
        call    reset_drive             ; make software reset to avoid "hang" errors
        out     dx,al
        inc     dx
        loop    check_identity          ; get port offsets 0x??2-0x??5
    identify:
        mov     dx,[BUS_IO_Port]
        add     dx,5
        mov     al,0xEC                 ; IDENTIFY command
        out     dx,al
        mov     cx,5                    ; read Status register 5 times
    read_status:                        ; delay = 400 ns
        xor     ax,ax
        in      al,dx                   ; read Status port
        loop    read_status
        cmp     al,0                    ; if 0 then drive does not exist
        je      no_support
        call    check_PIO
        test    al,0x80                 ; read until BIY bit is clear
        jnz     identify
    BIY_Bit:
        call    continue_check          ; check LBAmid and LBAhi ports for zero values
        cmp     bx,0
        je      no_support
        test    al,8
        jnz     finished                ; is DRQ bit set?
        test    al,1
        jnz     ERROR                   ; is ERR bit set?
        cmp     cx,0                    ; last attempt?
        je      no_support
        dec     cx
        xor     dx,dx
        jmp     identify
    continue_check:
        push    ax
        push    dx
        sub     dx,3                    ; get the LBAmid port
        in      al,dx
        cmp     al,0                    ; if they are non-zero: drive is not ATA
        jne     non_ATA
        inc     dx                      ; get the LBAhi port
        in      al,dx
        cmp     al,0
        jne     non_ATA                 ; otherwise continue polling
        pop     dx
        pop     ax
        mov     bx,1
        ret
    non_ATA:                            ; non-ATA drive return bx = 0
        pop     dx
        pop     ax
        mov     bx,0
        ret
    no_support:                         ; drive does not exist
        mov     bx,0
        ret
    ERROR:                              ; detected ERR bit
        xor     dx,dx
        jmp     $
    check_PIO:                          ; check for ATAPI or SATA device
        push    ax
        push    dx
        sub     dx,3
        in      al,dx                   ; read status port
        cmp     al,0x14                 ; ATAPI gives 0x14 on offset 4
        je      continue_ATAPI
        cmp     al,0x3C                 ; SATA gives 0x3C on offset 4
        je      continue_SATA
        pop     dx
        pop     ax
        ret
     continue_ATAPI:
        inc     dx
        in      al,dx
        cmp     al,0xEB                 ; see if offset 5 is set to 0xEB
        jne     unknown_device
        pop     dx
        pop     ax
        mov     bl,1                    ; recognized ATAPI drive
        ret
     continue_SATA:
        inc     dx
        in      al,dx
        cmp     al,0xC3                 ; see if offset 5 is set to 0xC3
        jne     unknown_device
        pop     dx
        pop     ax
        mov     bl,2                    ; recognized SATA drive
        ret
     reset_drive:
        mov     dx,[device_control]     ; reset primary bus drive
        in      al,dx
        push    ax
        or      al,4
        out     dx,al                   ; software reset
        pop     ax
        out     dx,al                   ; clear SRST bit after reset
     wait_hdd:
        in      al,dx
        test    al,2
        jnz     wait_hdd                ; wait until drive is ready
        ret
     unknown_device:                    ; device is unsupported yet
        pop     dx
        pop     ax
        jmp     $
     finished:
        mov     bx,0                    ; generic ATA device
        ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                                                       ;
;  Desc.:                                                               ;
;        Reads LBA#X with INT13h Extension. This should only happen     ;
;        after we verified that the LBA mode is supported.              ;
;                                                                       ;
;  Input:  ah - 0x42 function                                           ;
;          dl - Drive number                                            ;
;          ds:si - Disk Address Packet in memory                        ;
;                                                                       ;
;  Output: cy - set if error occurs                                     ;
;          bl - error code                                              ;
;                                                                       ;
;          bl - error code is defined as follows:                       ;
;               0 -> no error.                                          ;
;               1 -> size of packet is invalid.                         ;
;               2 -> offset 1 is not set to 0.                          ;
;               3 -> bad sector (permanently).                          ;
;                                                                       ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

read_HDD_sector:
        push    dx
        mov     dl,byte [ds:si+0]      ; test the first two offset for validation
        mov     dh,byte [ds:si+1]
        test    dl,16
        mov     bl,1
        js      error_ocurred          ; manipulated address packet
        test    dh,0
        mov     bl,2
        js      error_ocurred
        pop     dx
     @@:
        mov     ah,0x42
        int     0x13                    ; read disk
        jnc     LBA_ok
        mov     al,0xE7                 ; if no success try to flush the cache
        mov     dx,[BUS_IO_Port]
        add     dx,5
        out     dx,al
     wait_BSY:
        cmp     bl,3                    ; bl = 2 there was already a flush of the cache
        je      error_ocurred
        in      al,dx                   ; wait until BSY flag is clear
        test    al,0x80
        jns     wait_BSY
        mov     bl,3                    ; try to read again
        jmp     @b
     LBA_ok:
        mov     bx,0                    ; sector content is stored
        ret
     error_ocurred:                     ; see error code in bl
        pop     dx
        ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                                                                       ;
;  Desc.:                                                                               ;
;        Ignoring the wrong values from the FS, we want the actual ones                 ;
;        from the BIOS. This function is only required for CHS mode and we              ;
;        should avoid it, if there are other ways. Furthermore, do not                  ;
;        try to use this with floppy drives or emulated ones (Instead of this,          ;
;        it is recommended to take the default values).                                 ;
;                                                                                       ;
;        It also sets/overrides the header values.                                      ;
;                                                                                       ;
;  Input:  dl - drive number                                                            ;
;          cl - attempts to read                                                        ;
;                                                                                       ;
;  Output: cy - set if failure                                                          ;
;          ah - Disk status code (0x00 if success)                                      ;
;          al - input error                                                             ;
;          bh - 0x00 (floppy drives only)                                               ;
;          bl - Drive type, as follows (floppy drives only):                            ;
;               0x01 - 5.25", 360KB, 40 tracks                                          ;
;               0x02 - 5.25", 1.2MB, 80 tracks                                          ;
;               0x03 - 3.5", 720KB, 80 tracks                                           ;
;               0x04 - 3.5", 1.44MB, 80 tracks                                          ;
;          ch - Bottom 8 bits of maximum track number.                                  ;
;          cl - ttssssss, as follows:                                                   ;
;               tt = top two bits of 10-bit maximum track number.                       ;
;               ssssss = 6-bit maximum sector number.                                   ;
;                                                                                       ;
;          dh - Maximum head number.                                                    ;
;          dl - Number of drives installed.                                             ;
;          es:di - Pointer to the diskette parameter table entry for a floppy drive     ;
;                                                                                       ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

get_drive_param:
        mov     ah,0x08                         ; read drive parameters
        int     0x13
        jnc     .disk_success
        cmp     cx,0                            ; no attempts anymore?
        je      .fail
        loop    get_drive_param
        ret
     .disk_success:                             ; see error code in ax (al=input error,ah=disk read error)
        cmp     bh,0
        jne     .usual
        mov     [floppy_driver],bl              ; Only for ROM, RAM, RFD and floppy drives
     .usual:
        mov     [heads],dh
        mov     byte [sectors_track],cl         ; store output values
        mov     byte [max_track],ch
        push    cx
        and     cl,0x3F                         ; get the bottom 6 bits
        mov     [sectorcount],cl
        pop     cx
        shr     cx,6                            ; get top 10 bits of track number
        mov     byte [max_track+1],cl
        ret
     .fail:
        stc
        ret



identify_device:
        mov     al,1                            ; read Bootsector
        mov     dx,[BUS_IO_Port]
        out     dx,al

        inc     dx
        out     dx,al

        inc     dx
        mov     al,0
        out     dx,al

        inc     dx
        out     dx,al

        inc     dx
        mov     al,0xA0 or 0
        out     dx,al

        inc     dx
        mov     al,0x20
        out     dx,al
        mov     cx,4
     command_send:
        in      al,dx
        test    al,0x80
        jne     retry
        test    al,8
        jne     data_rdy
     retry:
        dec     cx
        jg      command_send
     prior_l:
        in      al,dx
        test    al,0x80
        jne     prior_l
        test    al,0x21
        jne     fail
     data_rdy:
        push    es di
        push    word 0
        pop     es
        mov     di,0xB046
        sub     dx,7
        mov     cx,256
        rep     insw
        or      dx,7
        in      al,dx
        in      al,dx
        in      al,dx
        in      al,dx
        test    al,0x21
        je      done
     fail:
        stc
        ret
     done:
        pop     di es
        ret