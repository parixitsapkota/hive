puts(str) {
  extrn putchar, char;
  auto i, c;
  i = 0;
  while (c = char(str, i)) {
    putchar(c);
    ++i;
  }
}

main() {
  puts("Hello, world!*n");
}
