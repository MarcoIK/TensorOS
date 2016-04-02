is_input:

       push     rdx
       mov      dx,word [midisp]
       in       al,dx
       and      al,0x80
       pop      rdx
       retq


is_output:

       push     rdx
       mov      dx,word [midisp]
       in       al,dx
       and      al,0x40
       pop      rdx
       retq


get_mpu_in:

       push     rdx
       mov      dx,word [mididp]
       in       al,dx
       pop      rdx
       retq


put_mpu_out:

       push     rdx
       mov      dx,word [mididp]
       out      dx,al
       pop      rdx
       retq


setuart:

 su1:
       call     is_output
       cmp      al,0
       jnz      su1
       mov      dx,word [midisp]
       mov      al,0xff
       out      dx,al
 su2:
       mov      dx,word [midisp]
       mov      al,0xff
       out      dx,al
       call     is_input
       cmp      al,0
       jnz      su2
       call     get_mpu_in
       cmp      al,0xfe
       jnz      su2
 su3:
       call     is_output
       cmp      al,0
       jnz      su3
       mov      dx,word [midisp]
       mov      al,0x3f
       out      dx,al

       retq


align 8

sys_midi:

       cmp      [mididp],0
       jnz      sm0
       mov      r8,1
       mov      [rsp+72],r8
       retq
   sm0:

       mov      r8,0
       cmp      rax,1
       mov      [rsp+72],r8
       jnz      smn1
       call     setuart
       retq
   smn1:

       cmp      rax,2
       jnz      smn2
   sm10:
       call     get_mpu_in
       call     is_output
       test     al,al
       jnz      sm10
       mov      al,bl
       call     put_mpu_out
       retq
   smn2:

       retq

