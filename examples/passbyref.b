func(a) {
    auto b;
    b = ++*a;
    return b;
}

main() {
    auto a;
    a = 120;
    return func(&a);
}
