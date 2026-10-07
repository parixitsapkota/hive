main() {
  auto c; 
  c = 0;
  {
    auto a, b;
    a = 2;
    b = 3;
    c = a + b;
  }
  return c;
}
