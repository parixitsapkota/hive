main() {
  auto index, temp;
  index = 0;
  loop:
  if (index >= 20) {
    goto end_loop;
  }
    temp = index;
    index = temp + 1;
  goto loop;
  end_loop:
  return index;
}
