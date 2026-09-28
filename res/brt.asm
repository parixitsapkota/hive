global _start, exit
extern __call_main

;; _start : [BRT](b runtime)
_start:
  call __call_main
  call exit

exit:
  mov rdi, rax
  mov rax, 0x3C
  syscall
