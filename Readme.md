<p align="center">
  <img src="./res/b.png" alt="B Language Logo" width="160">
</p>

<h1 align="center">HIVE-BBCC</h1>

<p align="center">
  <a href="https://github.com/parixitsapkota/hive">
    <img alt="GitHub top language" src="https://img.shields.io/github/languages/top/parixitsapkota/hive?colorA=141c1e&colorB=cc9694&style=for-the-badge&logo=c&logoColor=cc9694">
  </a>
  <a href="https://github.com/parixitsapkota/hive/commits/">
    <img alt="GitHub last commit" src="https://img.shields.io/github/last-commit/parixitsapkota/hive?colorA=141c1e&colorB=cc9694&style=for-the-badge&logo=github&logoColor=cc9694">
  </a>
</p>

<p align="center">
  <strong>Hive</strong> is a compiler for the <strong>B programming language</strong> written in <strong>C</strong>, targeting <strong>amd64</strong> (NASM) assembly.<br>
<strong>B</strong> is a typeless systems programming language developed by Ken Thompson and Dennis Ritchie at Bell Labs—the direct predecessor to <strong>C</strong>.
</p>

---

![Banner](./res/hive.png)

## Prerequisites

### Required

- **C Compiler (`CC`)**: Defaults to `clang`.
- **`make`**: Build system used for building, running, and testing.
- **`nasm`**: Netwide Assembler for emitting x86-64 assembly.
- **`gperf`**: Perfect hash generator for O(1) keyword/symbol lookup.

### Optional

- **`clang-format`**: For formatting the C source code.

---

### Build

```sh
make                 # debug build (ASan), writes ./trident and compile_commands.json
make MODE=release    # optimised build
make compdb          # only regenerate compile_commands.json
make clean           # remove build/ and ./trident
make install         # PREFIX=/usr/local by default
```

**Release Build (Linux)**

```sh
make clean all PLATFORM=linux MODE=release
```

---

## Configuration & Notes

> [!NOTE]
> You can override the default C compiler by setting `CC`:
>
> ```bash
> $ make CC=gcc
> ```

> [!WARNING]
> Linux is the primary target platform. Support for other operating systems is experimental.

---

<p align="center">
  <a href="https://github.com/parixitsapkota/hive/blob/main/LICENSE">
    <img alt="GitHub License" src="https://img.shields.io/github/license/parixitsapkota/hive?colorA=141c1e&colorB=cc9694&style=for-the-badge&logo=apache&logoColor=cc9694">
  </a>
</p>

<p align="center">
  <strong>Hive</strong> is licensed under the <strong>Apache 2.0 License</strong>.
</p>
