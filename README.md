# Unix-Like-OS

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## Table of Contents

- [Unix-Like-OS](#unix-like-os)
  - [Table of Contents](#table-of-contents)
  - [About](#about)
  - [Usage](#usage)
  - [Examples](#examples)

## About

Unix-Like-OS is an operating system inspired by Unix.

## Usage
- Run `fformat` to generate `myDisk.img`:
```
fformat
```
- Create a directory using the `mkdir` command:
```
mkdir [dir]
```
- 
  List all files in the current directory with the `ls` command:
```
ls
```
- Change the current directory using the `cd` command:

```
cd [dir]
```

- Create a file with the `fcreat` command:

```
fcreat [dir]
```
- Close a file using the `fclose` command:

```
fclose [fd]
```
- Open a file with the `fopen` command. If successful, the file ID `fd` will be provided:

```
fopen [dir]
```

- Write bytes to a file with the `fwrite` command:

```
fwrite [fd] [nbytes] [string]
```

- Read bytes from a file with the `fread` command:

```
fread [fd] [nbytes]
```
- Relocate the read-write pointer of a file with the `fseek` command:
```
fseek [fd] [offset] [ptrname]
```
- Delete a file using the `fdelete` command:
```
fdelete [dir]
```
- To exit the file system, run:
```
exit
```

## Examples

![0_初始界面](./img/0_初始界面.png)

![1-0_fformat](./img/1-0_fformat.png)

![1-1_mkdir](./img/1-1_mkdir.png)

![1-2_cd](./img/1-2_cd.png)

![1-3_fcreat](./img/1-3_fcreat.png)

![1-4_fclose](./img/1-4_fclose.png)

![1-5_fopen](./img/1-5_fopen.png)

![1-6_fwrite](./img/1-6_fwrite.png)

![1-7_fread](./img/1-7_fread.png)

![1-8_fseek](./img/1-8_fseek.png)

![1-9_fdelete](./img/1-9_fdelete.png)

![1-10_exit](./img/1-10_exit.png)

![1-11_fmount-compare](./img/1-11_fmount-compare.png)

![1-11_fmount](./img/1-11_fmount.png)
