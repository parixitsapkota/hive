main() {
    auto char;
    char = '*e';
    switch (char) {
    case '*0':
        return 1;
    case '*e':
        char = '*0';
        return 0;
    }
}
