default rel
global syscall

; syscall(call, a1, a2, a3, a4, a5, a6)
syscall:
  mov rax, rdi
  mov rdi, rsi
  mov rsi, rdx
  mov rdx, rcx
  mov r10, r8
  mov r8,  r9
  mov r9,  [rsp + 8]
  syscall
  ret
