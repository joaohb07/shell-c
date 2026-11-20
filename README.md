# MySh — Custom POSIX Command-Line Shell in C

<p align="left">
  <img alt="Language" src="https://badgen.net/badge/Language/C%20(C11%20%2F%20POSIX)/blue" />
  <img alt="Platform" src="https://badgen.net/badge/OS/Linux%20%26%20macOS/green" />
  <img alt="Build" src="https://badgen.net/badge/Build/GNU%20Make/orange" />
  <a href="https://joaohb07.github.io/documentation/" target="_blank">
    <img alt="Documentation" src="https://badgen.net/badge/Docs/MkDocs%20Material/purple" />
  </a>
</p>

> **A lightweight POSIX-compliant interactive shell implemented in C**, built from scratch using UNIX system calls for process lifecycle management (`fork`, `execvp`, `waitpid`), inter-process communication (`pipe`, `dup2`), and asynchronous signal handling (`SIGINT`, `SIGTSTP`).

---

## Key Features

* **Process Creation & Execution (`fork`, `execvp`, `waitpid`):**
  * Spawns isolated child processes for external binaries resolved via `$PATH`, synchronizing foreground execution and capturing exit/termination statuses.
* **Multi-Stage Pipeline Chaining (`pipe`, `dup2`):**
  * Supports arbitrary command pipelines (`cmd1 | cmd2 | cmd3`) by wiring unidirectional kernel pipe file descriptors between consecutive child `STDOUT_FILENO` and `STDIN_FILENO` streams while preventing descriptor leaks.
* **Built-In Shell Commands (`cd`, `exit`):**
  * Executes directory navigation (`cd`, `cd ~`, `cd <path>`) directly inside the parent shell process via `chdir()` so working directory state persists across commands.
* **Signal Isolation (`signal`):**
  * Traps `SIGINT` (`Ctrl+C`) and `SIGTSTP` (`Ctrl+Z`) in the parent REPL loop so interactive interrupts terminate foreground child jobs without killing the shell session.
* **Dynamic Context-Aware Prompt:**
  * Displays `[MySh] <user>@<hostname>:<cwd>$` with automatic `$HOME` tilde (`~`) path abbreviation.

---

## Build & Usage

### Prerequisites

* **GCC** or **Clang** (POSIX-compatible environment: Linux or macOS)
* **GNU Make**

### Compile & Run

```bash
git clone [https://github.com/joaohb07/shell-c.git](https://github.com/joaohb07/shell-c.git)
cd shell-c
make
./mysh
```
### Example Session

```bash
[MySh] bots@MacBook-Pro:~/Documents/repos/shell-c$ ls -la | grep .c | wc -l
       1
[MySh] bots@MacBook-Pro:~/Documents/repos/shell-c$ cd ..
[MySh] bots@MacBook-Pro:~/Documents/repos$ cd ~
[MySh] bots@MacBook-Pro:~$ exit
```

## Academic Context & Authors

Developed for the **Operating Systems II** course at **São Paulo State University (UNESP)**.

- João Henrique Botelho (@joaohb07)

- João Victor Figueredo

- Paulo Santos