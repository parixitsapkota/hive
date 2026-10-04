# libb (internal functions)

The following is a list of functions in the B-library.

```c
c = char(string, i);
// The i-th character of the string is returned.

```

```c
error = chdir(string);
// The path name represented by the string becomes the current directory. A negative number returned indicates an error. 

```

```c
error = chmod(string, mode);
// The file specified by the string has its mode changed to the mode argument. A negative number returned indicates an error.

```

```c
error = chown(string, owner);
// The file specified by the string has its owner changed to the owner argument. A negative number returned indicates an error.

```

```c
error = close(file);
// The open file specified by the file argument is closed. A negative number returned indicates an error.

```

```c
file = creat(string, mode);
// The file specified by the string is either truncated or created in the mode specified depending on its prior existence. In both cases, the file is opened for writing and a file descriptor is returned. A negative number returned indicates an error.

```

```c
ctime(time, date);
// The system time (60-ths of a second) represented in the two-word vector time is converted to a 16-character date in the 8-word vector date. The converted date has the following format: "Mmm dd hh:mm:ss". 

```

```c
execl(string, arg0, arg1, ..., 0);
// The current process is replaced by the execution of the file specified by string. The arg-i strings are passed as arguments. A return indicates an error.

```

```c
execv(string, argv, count);
// The current process is replaced by the execution of the file specified by string. The vector of strings of length count are passed as arguments. A return indicates an error.

```

```c
exit();
// The current process is terminated.

```

```c
error = fork();
// The current process splits into two. The child process is returned a zero. The parent process is returned the process ID of the child. A negative number returned indicates an error.

```

```c
error = fstat(file, status);
// The i-node of the open file designated by file is put in the 20-word vector status. A negative number returned indicates an error.

```

```c
char = getchar();
// The next character from the standard input file is returned. The character `*e' is returned for an end-of-file. 

```

```c
id = getuid();
// The user-ID of the current process is returned.

```

```c
error = gtty(file, ttystat);
// The teletype modes of the open file designated by file is returned in the 3-word vector ttystat. A negative number returned indicates an error.

```

```c
lchar(string, i, char);
// The character char is stored in the i-th character of the string. 

```

```c
error = link(string1, string2);
// The pathname specified by string2 is created such that it is a link to the existing file specified by string1. A negative number returned indicates an error.

```

```c
error = mkdir(string, mode);
// The directory specified by the string is made to exist with the specified access mode. A negative number returned indicates an error.

```

```c
file = open(string, mode);
// The file specified by the string is opened for reading if mode is zero, for writing if mode is not zero. The open file designator is returned. A negative number returned indicates an error.

```

```c
putchar(char);
// The character char is written on the standard output file. 

```

```c
nread = read(file, buffer, count);
// Count bytes are read into the vector buffer from the open file designated by file. The actual number of bytes read are returned. A negative number returned indicates an error.

```

```c
error = seek(file, offset, pointer);
// The I/O pointer on the open file designated by file is set to the value of the designated pointer plus the offset. A pointer of zero designates the beginning of the file. A pointer of one designates the current I/O pointer. A pointer of two designates the end of the file. A negative number returned indicates an error.

```

```c
error = setuid(id);
// The user-ID of the current process is set to id. A negative number returned indicates an error.

```

```c
error = stat(string, status);
// The i-node of the file specified by the string is put in the 20-word vector status. A negative number returned indicates an error.

```

```c
error = stty(file, ttystat);
// The teletype modes of the open file designated by file is set from the 3-word vector ttystat. A negative number returned indicates an error.

```

```c
time(timev);
// The current system time is returned in the 2-word vector timev.

```

```c
error = unlink(string);
// The link specified by the string is removed. A negative number returned indicates an error.

```

```c
error = wait();
// The current process is suspended until one of its child processes terminates. At that time, the child's process-ID is returned. A negative number returned indicates an error.

```

```c
nwrite = write(file, buffer, count);
// Count bytes are written out of the vector buffer on the open file designated by file. The actual number of bytes written are returned. A negative number returned indicates an error.

```

---

Besides the functions available from the library, there is a predefined external vector named argv included with every program. The size of `argv` is `argv[0]+1`. The elements `argv[1] ... argv[argv[0]]` are the parameter strings as passed by the system in the execution of the current process.

From : [Users' Reference to B](https://www.nokia.com/bell-labs/about/dennis-m-ritchie/kbman.html)

---

# libtb (hive-b functions)

The following is a list of functions in the Hive-B-library.

## Mem

```c
ptr = malloc(size);
// returns a size memory block address. 0 returned indicates an error.

```

```c
ptr = calloc(size);
// returns a zero initialized size memory block address. 0 returned indicates an error.

```

```c
ptr = realloc(ptr, size);
// returns a size memory block address with ptr data. 0 returned indicates an error.

```

```c
error = free(ptr, size);
// A negative number returned indicates an error.

```
