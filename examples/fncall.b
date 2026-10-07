/*
 *
 * amd64 function call convention :
 *
 * arg0   : [rdi]
 * arg1   : [rsi]
 * arg2   : [rdx]
 * arg3   : [r10]
 * arg4   : [r8]
 * arg5   : [r9]
 * arg6   : [rsp - 8]
 * arg-n  : [rsp - n*8]
 *
 */

add(a, b) { return a + b; }

main() {
  extrn hello, good;
  return add(1, 9);
}
