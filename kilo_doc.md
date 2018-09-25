Documentation for the text editor kilo building from scratch

## 1. STDIN\_FILENO 

It is a "file descriptor". Basically, an integer to denote the position of the opened file in the memory. Generally,
STDIN\_FILENO = 0
STDOUT\_FILENO = 1
STDERR\_FILENO = 2

## 2. termios.h
Header file which gives access to the terminal I/O.
```C
struct termios
```
is a struct which contains all the necessary attributes concerning the I/O attributes of the terminal. See the man page <http://man7.org/linux/man-pages/man3/termios.3.html> for more details.

##3. Setting the right terminal attributes for the text editor
###1. Disabling ECHO: 
We disable ECHO property of `c_oflag` to prevent key presses from displaying on the terminal. Useful for hiding i/p during i/p of passwords.


