;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;   Rd.inc (c) Marco K
;;   License: GPL. See file copying for details.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ramdisk_free_space:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   returns free space in edi
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        push   rax rbx rcx

        mov    eax,fat_table
        xor    rdi,rdi
        mov    rcx,1448000/512
      rdfs1:
        mov    rbx,[rax]
        and    rbx,4095
        cmp    rbx,0
        jne    rdfs2
        add    rdi,512
      rdfs2:
        add    rax,2
        loop   rdfs1

        pop    rcx rbx rax
        ret


expand_filename:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; expand filename with '.' to 11 character
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        push    rsi rdi rbx

        mov     rsi,rax

        mov     rdi,rsp                  ; check for '.' in the name
        add     rdi,24+16
        mov     rax,rdi

        mov     [rax+0],dword '    '
        mov     [rax+4],dword '    '
        mov     [rax+8],dword '    '

      flr1:

        cmp     [rsi],byte '.'
        jne     flr2
        mov     rdi,rax
        add     rdi,7
        jmp     flr3

      flr2:

        mov     bl,[rsi]
        mov     [rdi],bl

      flr3:

        inc     rsi
        inc     rdi

        mov     rbx,rax
        add     rbx,11

        cmp     rdi,rbx
        jbe     flr1

        pop     rbx rdi rsi
        ret


fileread:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  fileread - sys floppy
;
;  eax  points to filename 11 chars
;  ebx  first wanted block       ; 1+ ; if 0 then set to 1
;  ecx  number of blocks to read ; 1+ ; if 0 then set to 1
;  edx  mem location to return data
;  esi  length of filename 12*X
;
;  ret ebx = size or 0xffffffff file not found
;      eax = 0 ok read or other = errormsg
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   ;     push   esi
   ;     mov    esi,eax
   ;     push   dword [esi+11]
   ;     mov    [esi+11],byte 13
   ;     mov    [esi+12],byte 10
   ;     mov    [esi+13],byte 10
   ;     push   esi
   ;     call   sys_msg_board_str
   ;     pop    esi
   ;     pop    dword [esi+11]
   ;     pop    esi

        cmp     rsi,0               ; return ramdisk root
        jne     fr_noroot
        cmp     ebx,224/16
        jbe     fr_do
        mov     rax,5
        mov     rbx,0
        ret
      fr_do:
        mov     rdi,rdx
        dec     rbx
        shl     rbx,9
        mov     rsi,fat_base+512*19
        add     rsi,rbx
        shl     rcx,9
        cld
        rep     movsb
        mov     rax,0 ; ok read
        mov     rbx,0
        ret
     fr_noroot:

        sub     rsp,64
        push    rax rcx rdi
        mov     rdi,rsp
        add     rdi,3*8
        mov     rcx,32
        mov     rax,0
        cld
        rep     stosb
        pop     rdi rcx rax
        call    expand_filename

      frfloppy:

        ; Read starts at block no1

        cmp     rbx,0
        jne     frfl5
        mov     rbx,1
      frfl5:

        ; Return 1+ blocks

        cmp     rcx,0
        jne     frfl6
        mov     rcx,1
      frfl6:

        dec     ebx

        push    rax ; 48 file size
        push    rax ; 40 file size
        push    rbx ; 32 first wanted block
        push    rcx ; 24 amount of 512 blocks to read
        push    rdx ; 16 return data to
        push    rsi
        push    rdi

        mov     edi,fat_base+512*18+512  ;Point at directory
        mov     edx,224 +1

l.20:
        dec     edx
        jnz     l.21
        mov     eax,5                    ; file not found
        mov     ebx,-1


        add     rsp,64+56
        ret

      fd_file_not_found: db 'File not found',13,10,0
      fd_file_found: db 'File found',13,10,0

      l.21:

  ;      mov    esi,eax                  ;Name of file we want
  ;      mov    ecx,11
  ;      cld
  ;      rep    cmpsb                    ;Found the file?
  ;      je     fifound                  ;Yes
  ;      add    cl, 21                   ;Offset to next directory entry
  ;      add    edi, ecx                 ;Advance to next entry

        mov     ecx,[eax]
        cmp     ecx,[edi]
        jne     noflf
        mov     ecx,[eax+4]
        cmp     ecx,[edi+4]
        jne     noflf
        mov     ecx,[eax+7]
        cmp     ecx,[edi+7]
        jne     noflf
        add     edi,11
        jmp     fifound
      noflf:

        add     edi,32

        jmp     l.20

     fifound:

        mov     rbx,[rdi-11+28]          ;file size
        and     rbx,0x1fffff
        mov     [rsp+40],rbx
        mov     [rsp+48],rbx
        mov     rax,[rdi-11+26]
        and     rax,65535
        mov     rdi,rax                  ;edi=cluster

      frnew:

        add     rax,31                   ;bootsector+2*fat+filenames
        shl     rax,9                    ;*512
        add     eax,fat_base             ;image base
        mov     rbx,[rsp+16]
        mov     rcx,512                  ;[rsp+16]

        mov     r8,0
        cmp     [rsp+32],r8         ;Wanted 512 byte block ?
        jne     frfl7

        push    rcx rsi rdi
        mov     rsi,rax
        mov     rdi,rbx
        mov     rcx,512
        cld
        rep     movsb
        pop     rdi rsi rcx
        ; call memmove

        add     [rsp+16],dword 512
        dec     qword [rsp+24]                  ;last wanted cluster ?
        mov     r8,0
        cmp     [rsp+24],r8                     ; 0.80
        je      frnoread
        jmp     frfl8
      frfl7:
        dec     qword [rsp+32]
      frfl8:
        shl     rdi,1                           ;find next cluster from FAT
        add     edi,fat_table                   ; 0xD80000 ;  0x280000
        mov     rax,[rdi]
        and     rax,4095
        mov     rdi,rax
        cmp     rdi,4095                        ;eof  - cluster
        jz      frnoread2

        mov     r8,512
        cmp     [rsp+48],r8                     ;eof  - size
        jb      frnoread
        sub     [rsp+48],dword 512

        jmp     frnew

      frnoread2:

        mov     r8,0
        cmp     [rsp+32],r8                     ; eof without read ?
        je      frnoread

        pop     rdi
        pop     rsi
        pop     rdx
        pop     rcx
        pop     rbx
        pop     rbx     ; ebx <- eax : size of file

        add     rsp,64+8

        mov     rax,6   ; end of file
        ret

      frnoread:

        pop     rdi rsi rdx rcx
        add     rsp,8
        pop     rbx     ; ebx <- eax : size of file
        add     rsp,72
        mov     rax,0
        ret

filedelete:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; filedelete - sys floppy
;
; in:
; eax - filename 11 chars
;
; out:
; eax - 0 = successful, 1 = file not found
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        mov     rbp,1  ; file not found as default

      filedelete_newtry:

        sub     rsp,64
        call    expand_filename

        push    rax rbx rcx rdx rsi rdi

        mov     edi,fat_base+512*18+512  ;Point at directory
        mov     rdx,224 +1

     l20d:
        sub     rdx,1
        cmp     rdx,0
        jnz     l21d
        jmp     frnoreadd
     l21d:
        mov     rsi,rax                  ;Name of file we want
        mov     rcx,11
        cld
        rep     cmpsb                    ;Found the file?
        je      fifoundd                 ;Yes
        add     cl,21                   ;Offset to next directory entry
        add     rdi,rcx                 ;Advance to next entry
        jmp     l20d
     fifoundd:

        mov     [rdi-11],byte 0xE5       ;mark filename deleted
        add     rdi,0xf
        mov     rax,[rdi]
        and     rax,65535
        mov     rdi,rax                  ;edi = cluster

      frnewd:

        shl     rdi,1                    ;find next cluster from FAT
        add     edi,fat_table ; 0x280000
        mov     rax,[rdi]
        mov     [rdi],word 0x0           ;clear fat chain cluster
        and     rax,4095
        mov     rdi,rax
        cmp     edi,dword 4095           ;last cluster ?
        jz      frnoreadd2

        jmp     frnewd

      frnoreadd2:

        pop     rdi rsi rdx rcx rbx rax
        add     rsp,64
        mov     rbp,0       ; file found
        jmp     filedelete_newtry

      frnoreadd:

        pop     rdi rsi rdx rcx rbx rax
        add     rsp,64
        mov     rax,rbp
        ret

filesave:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; filesave - sys floppy
;
; eax points to filename 11 chars
;
;        eax      ; pointer to file name
;        ebx      ; buffer
;        ecx      ; count to write in bytes
;        edx      ; 0 create new , 1 append
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        sub     rsp,64
        call    expand_filename
        cmp     rdx,0
        jnz     fsdel
        pushdu
        call    filedelete
        popdu

      fsdel:

        call    ramdisk_free_space
        cmp     rcx,rdi
        jb      rd_do_save
        add     rsp,64
        mov     rax,1    ; not enough free space
        ret

      rd_do_save:

        push    rax rbx rcx rdx rsi rdi

        mov     edi,fat_base+512*18+512  ;Point at directory
        mov     rdx,224 +1
        ; find an empty spot for filename in the root dir
     l20ds:
        sub     rdx,1
        cmp     rdx,0
        jnz     l21ds
        jmp     frnoreadds
     l21ds:
        cmp     [rdi],byte 0xE5
        jz      fifoundds
        cmp     [rdi],byte 0x0
        jz      fifoundds
        add     rdi,32                   ; Advance to next entry
        jmp     l20ds
     fifoundds:

        push    rdi                      ; move the filename to root dir
        mov     rsi,[rsp+8+40]
        mov     rcx,11
        cld
        rep     movsb
        pop     rdi
        mov     rdx,rdi
        add     rdx,11+0xf               ; edx <- cluster save position
        mov     rbx,[rsp+24]             ; save file size
        mov     [rdi+28],ebx
        mov     [rdi+11],byte 0x20       ; attribute
        mov     rbx,1                    ; first cluster

      frnewds:

        add     rbx,1
        mov     rdi,rbx                  ; find free cluster in FAT
        shl     rdi,1
        add     edi,fat_table ;0x280000
        mov     rax,[rdi]
        and     rax,4095
        cmp     rax,0x0
        jnz     frnewds
        mov     [rdx],bx                 ; save next cluster pos. to prev cl.
        mov     rdx,rdi                  ; next save pos abs mem add
        call    fdc_filesave
        pushdu                          ; move save to floppy cluster
        add     rbx,31
        shl     rbx,9
        add     rbx,fat_base
        mov     rax,[rsp+64+32]
        mov     rcx,512
        call    memmove
        popdu

        mov     rax,[rsp+24]
        cmp     rax,512
        jb      flnsa
        sub     rax,512
        mov     [rsp+24],rax
        mov     rax,[rsp+32]
        add     rax,512
        mov     [rsp+32],rax
        jmp     frnewds

     flnsa:
        mov     [rdi],word 4095          ; mark end of file - last cluster

      frnoreadds:

        pop     rdi rsi rdx rcx rbx rax
        add     rsp,64

        pushdu
        cli
        call    fdc_commitfile
        sti
        popdu

        mov     rax,0
        ret



