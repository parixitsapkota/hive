extrn syscall;

char(string, i) {
    return *(string + i);
}

putchar(char) {
    syscall(1, 1, &char, 1);
}
