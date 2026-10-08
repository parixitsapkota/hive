num[5];
main() {
    extrn putchar;
    auto i, j, temp;

    num[0] = '2';
    num[1] = '1';
    num[2] = '9';
    num[3] = '7';
    num[4] = '5';

    i = 0;
    while (i < 5) {
        putchar(*(num + i * 8));
        ++i;
    }
    putchar('*n');

    /* Buble sort */
    i = 0;
    while (i < 4) {
        j = i +1;
        while (j < 5) {
            if (num[i] > num[j]) {
                temp = num[i];
                num[i] = num[j];
                num[j] = temp;
            }
            ++j;
        }
        ++i;
    }

    i = 0;
    while (i < 5) {
        putchar(*(num + i * 8));
        ++i;
    }
    putchar('*n');
    return 0;
}
