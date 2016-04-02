;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                 ;;
;;  QUEUE.INC                                                      ;;
;;                                                                 ;;
;;  Buffer queue management for Tensor OS TCP/IP Stack             ;;
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
;       queueInit   Configures the queues to empty
;       dequeue     Removes a buffer pointer from a queue
;       queue       Inserts a buffer pointer into a queue
;       freeBuff    Adds the buffer pointer to the list of free buffers
;       queueSize   Returns the number of entries in a queue
;
;      The various defines for queue names can be found in stack.inc
;
;*******************************************************************


;***************************************************************************
;   Function
;      freeBuff
;
;   Description
;       Adds a buffer number to the beginning of the free list.
;       buffer number in eax  ( ms word zeroed )
;       all other registers preserved
;        This always works, so no error returned
;***************************************************************************
freeBuff:
        push    rbx
        push    rcx
        mov     rbx,EMPTY_QUEUE
        shl     rbx,1
        add     rbx,queues
        cli     ; Ensure that another process does not interfer
        movzx   rcx,word [rbx]
        mov     [rbx],ax
        shl     rax,1
        add     rax,queueList
        mov     [rax],cx
        sti
        pop     rcx
        pop     rbx

        retq


;***************************************************************************
;   Function
;      queueSize
;
;   Description
;       Counts the number of entries in a queue
;       queue number in ebx ( ms word zeroed )
;       Queue size returned in eax
;    This always works, so no error returned
;***************************************************************************
queueSize:
        xor     rax,rax
        shl     rbx,1
        add     rbx,queues
        movzx   rcx,word [rbx]
        cmp     cx,NO_BUFFER
        je      qs_exit

qs_001:
        inc     rax
        shl     rcx,1
        add     rcx,queueList
        movzx   rcx,word [rcx]
        cmp     cx,NO_BUFFER
        je      qs_exit
        jmp     qs_001

qs_exit:
        retq


;***************************************************************************
;   Function
;      queue
;
;   Description
;       Adds a buffer number to the *end* of a queue
;       This is quite quick because these queues will be short
;       queue number in eax ( ms word zeroed )
;       buffer number in ebx  ( ms word zeroed )
;       all other registers preserved
;        This always works, so no error returned
;***************************************************************************
queue:
        push    rbx
        shl     rbx,1
        add     rbx,queueList        ; eax now holds address of queue entry
        mov     [rbx],word NO_BUFFER    ; This buffer will be the last

        cli
        shl     rax,1
        add     rax,queues            ; eax now holds address of queue
        movzx   rbx,word [rax]

        cmp     bx,NO_BUFFER
        jne     qu_001

        pop     rbx
        ; The list is empty, so add this to the head
        mov     [rax],bx
        jmp     qu_exit

qu_001:
        ; Find the last entry
        shl     rbx,1
        add     rbx,queueList
        mov     rax,rbx
        movzx   rbx,word [rbx]
        cmp     bx,NO_BUFFER
        jne     qu_001

        mov     rbx,rax
        pop     rax
        mov     [rbx],ax

qu_exit:
        sti
        retq



;***************************************************************************
;   Function
;      dequeue
;
;   Description
;       removes a buffer number from the head of a queue
;       This is fast, as it unlinks the first entry in the list
;       queue number in eax ( ms word zeroed )
;       buffer number returned in eax ( ms word zeroed )
;       all other registers preserved
;
;***************************************************************************
dequeue:
        push    rbx
        shl     rax,1
        add     rax,queues            ; eax now holds address of queue
        mov     rbx,rax
        cli
        movzx   rax,word [rax]
        cmp     ax,NO_BUFFER
        je      dq_exit
        push    rax
        shl     rax,1
        add     rax,queueList        ; eax now holds address of queue entry
        mov     ax,[rax]
        mov     [rbx],ax
        pop     rax

dq_exit:
        sti
        pop     rbx
        retq


;***************************************************************************
;   Function
;      queueInit
;
;   Description
;       Initialises the queues to empty, and creates the free queue
;       list.
;
;***************************************************************************
queueInit:
        mov     rsi,queues
        mov     rcx,NUMQUEUES
        mov     ax,NO_BUFFER

qi001:
        mov     [rsi],ax
        inc     rsi
        inc     rsi
        loop    qi001

        mov     rsi,queues + ( 2 * EMPTY_QUEUE )

        ; Initialise empty queue list

        xor     ax,ax
        mov     [rsi],ax

        mov     rcx,NUMQUEUEENTRIES - 1
        mov     rsi,queueList

qi002:
        inc     ax
        mov     [rsi],ax
        inc     rsi
        inc     rsi
        loop    qi002

        mov     ax,NO_BUFFER
        mov     [rsi],ax

        retq
