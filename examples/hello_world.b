puts(str) {
  extrn putchar, char;
  auto i, c;
  i = 0;
  while ((c = char(str, i)) & c != '*0') {
    putchar(c);
    ++i;
  }
}

main() {
  puts("Hello, world!*n*0");
}
