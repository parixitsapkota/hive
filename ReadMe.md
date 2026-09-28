<p align=center>
  <img align=center src="./res/b.png" width=200>
</p>

<h1 align=center>TRIDENTC</h1>

<p align=center>
  <a href="https://raw.githubusercontent.com/parixitsapkota/tridentc/refs/heads/main/LICENSE"><img alt="GitHub License" src="https://img.shields.io/github/license/parixitsapkota/tridentc?colorA=141c1e&colorB=cc9694&style=for-the-badge&logo=apache&logoColor=cc9694"></a>
  <a href="https://github.com/parixitsapkota/tridentc"><img alt="GitHub top language" src="https://img.shields.io/github/languages/top/parixitsapkota/tridentc?colorA=141c1e&colorB=cc9694&style=for-the-badge&logo=c&logoColor=cc9694"></a>
  <a href="https://github.com/parixitsapkota/tridentc/commits/"><img alt="GitHub last commit" src="https://img.shields.io/github/last-commit/parixitsapkota/tridentc?colorA=141c1e&colorB=cc9694&style=for-the-badge&logo=github&logoColor=cc9694"></a>
  <p >Trident is an compiler for the <strong>B programming language</strong>, targeting <strong>amd64</strong> assembly. <strong>B</strong> is a typeless systems programming language developed by Ken Thompson and Dennis Ritchie at Bell Labs—the direct predecessor to <strong>C</strong>.</p>
</p>

![Banner](./res/trident.png)

---

## 🛠️ Requirements & Build Guide

### Prerequisites

- **Required** : `clang`, `make`, `nasm`
- **Optional** : `clang-format`, `w64-mingw32`, `gperf`

### Building

To compile the release build on Linux:

```bash
make PLATFORM=linux MODE=release
```

To run example:

```bash
make run EXAMPLE=res/example.b
```

> **TODO:** Add better errors and syntax suger (like =+, =-, =* ....).

> **Note:** If you modify `res/keywords.gperf`, regenerate the header file before building:  
> `gperf -N get_keyword_kind -t res/keywords.gperf > src/keywords.h`

> [!WARNING]
> Linux is the primary target. Support for other platforms (e.g., Windows via MinGW) is experimental!

---

Trident is licensed under the **Apache-2.0 License**. See the [`LICENSE`](https://raw.githubusercontent.com/parixitsapkota/tridentc/refs/heads/main/LICENSE) file for more information.
