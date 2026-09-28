extrn syscall;

char(string, i) {
    return *(string + i * 8);
}

putchar(char) {
    syscall(1, 1, &char, 1);
}
