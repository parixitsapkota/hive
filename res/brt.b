/* show return __call_main */
extrn putchar, char;

__putstr(str) {
    auto i, c;
    i = 0;
    while (c = char(str, i)) {
        putchar(c);
        ++i;
    }
}

__pint(num, base, is_signed) {
    auto digits;
    digits = "0123456789abcdef";

    if (is_signed && num < 0) {
        putchar('-');
        num = -num;
    }

    if (num / base) {
        __pint(num / base, base, 0);
    }

    putchar(char(digits, num % base));
}

/*
$w
$s
$$
$&
$*
*/

__call_main() {
    extrn main;
    auto ret;

    ret = main();

    if (ret != 0) {
        __putstr("process terminated abnormally with exit code ");
        __pint(ret, 10, 1);
        putchar('*n');
    }

    return (ret);
}
