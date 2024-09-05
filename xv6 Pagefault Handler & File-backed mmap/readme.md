
# Project #5 - xv6 Pagefault Handler & File-backed mmap

## Part 5a: Pagefault Handler
(Based on the assignment from [MIT](https://pdos.csail.mit.edu/6.828/2012/homework/xv6-zero-fill.html))

Before moving on to adding file-backed mappings to our `mmap()` implementation, let's make `xv6` more like a full operating system, such as Linux. Currently, the `mmap()` system call immediately allocates (`kalloc`) and maps (`mappages`) all the physical memory pages required to fulfill the user request. In this part of the project, you'll change this behavior into lazy page allocation, where pages are only allocated when a page fault occurs during access.

### Advantages of Lazy Page Allocation

- Some programs allocate memory but never use it, for example, to implement large sparse arrays. Lazy page allocation allows us to avoid spending time and physical memory allocating and mapping the entire region.
- We can allow programs to map regions larger than the available physical memory. One of the operating system's main goals is to provide the illusion of unlimited resources.

## Part 5b: mmap Part 2: File-Backed Mappings

You can now support file-backed mmap regions. File-backed memory maps are the same as anonymous regions except they are initialized with the contents of the file descriptor. They also write changes made to the memory region back to the file using the `msync()` system call, which you'll also implement in this part.

File-backed memory maps can be useful to avoid the high cost of writing changes to persistent storage. It allows the program to manipulate the mapped file in memory as if it were a byte array and then write multiple changes back at once. However, no changes made to file-backed memory regions are guaranteed to be persistent until `msync()` successfully returns. (Calling `munmap()` does not guarantee durability.)

### Seeking in a file

Here is the `mmap()` prototype again for reference:

```c
void *mmap(void *addr, uint length, int prot, int flags, int fd, int offset);
```

The `offset` argument to `mmap()` describes the offset into the file that the file-backed region should be initialized from and written back to. Currently, `xv6` does not support seeking to another offset in the file. You will have to add this function before proceeding with the rest of this part.

### System Calls: `mmap()`, `munmap()`, and `msync()`

- `mmap()`: In `mmap()`, first determine which type of memory region is being mapped. If the `fd` is valid, create a duplicate file descriptor and save it in your data structure. You will read the contents from the file inside the page fault handler as the program tries to access that page.

- `munmap()`: In addition to your previous implementation, deallocate the duplicated file descriptor and decrement the open count using `fileclose()` before freeing your data structure.

- `msync()`: The `msync()` system call looks like this:
    ```c
    int msync(void *start_addr, int length);
    ```

    You will need to find the corresponding memory region and write the contents of the memory region back to the file descriptor. Use the offset stored in the region.

### Optimizing `msync()`

In `msync()`, the kernel can check the dirty bit of the pages within the mapped region and write only modified pages to disk, increasing performance. You can use the macros defined inside `mmu.h` to extract the dirty bit from the Page Table Entry (PTE). Only write a page to disk if:
- It belongs to a valid mmap region.
- It is mapped into the process's address space.
- The dirty bit is set in the page’s PTE.

### Running Tests

Use the following script to run tests:
```
./test-mmap.sh -c -v
```

If implemented correctly, you should receive a notification that the tests passed.

### Adding Test Files Inside xv6

- To run test files in `xv6`, copy the test files (e.g., `test_1.c`, `test_2.c`) into the `xv6/src` directory.
- Compile `xv6` using the command:
  ```
  make clean && make qemu-nox
  ```

- Once compiled successfully, run the test inside `xv6`:
  ```
  $ xv6test
  ```

### Appendix – Test Options

The script `run-tests.sh` can be used to execute tests. Options include:
- `-h`: Show help message.
- `-v`: Verbose mode.
- `-t n`: Run only test `n`.
- `-c`: Continue even after a test fails.
