base [50];

display(n) {
    extrn putchar;
    auto i, t;
    i = 0;
    while (i < n) {
        if (base[i]) {
            putchar('#');
        } else {
            putchar('.');
        }
        t = i;
        i = t + 1;
    }
    putchar('*n');
}

next(n) {
    auto i, state, t;
    state = (base[0] << 1) | base[1];
    i = 2;
    while (i < n) {
        state = ((state << 1) | base[i]) & 7;
        base[i - 1] = (110 >> state) & 1;
        t = i + 1;
        i = t;
    }
}

main() {
    auto n, i, t;
    n = 50;

    base[n - 2] = 1;
    display(n);
    i = 0;
    while (i < n - 3) {
        next(n);
        display(n);
        t = i + 1;
        i = t;
    }
}
