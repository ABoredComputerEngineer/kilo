Documentation for the text editor kilo building from scratch
## 1. Low level i/p and o/p

The read() and write() functions provide a lower level of i/p than the usual family of printf and scanf functions.
The read() function reads a given number of bytes from the given file ( Denoted by a file descriptor ) into a given buffer.

## 2. STDIN\_FILENO 

It is a "file descriptor". Basically, an integer to denote the position of the opened file in the memory. Generally,
STDIN\_FILENO = 0
STDOUT\_FILENO = 1
STDERR\_FILENO = 2

## 3. termios.h
Header file which gives access to the terminal I/O.
```C
struct termios
```
is a struct which contains all the necessary attributes concerning the I/O attributes of the terminal. See the man page <http://man7.org/linux/man-pages/man3/termios.3.html> for more details.

##4. Setting the right terminal attributes for the text editor
###1. Disabling ECHO: 
We disable ECHO property of `c_oflag` to prevent key presses from displaying on the terminal. Useful for hiding i/p during i/p of passwords.
###2. The ICANON flag:
The ICANON flag belongs to c\_lflag which is used to set ON/OFF canonical mode for terminals. In cannonical mode the terminal processes i/p one line at a time and allows for editing ( i.e. Backspace and such ) of the command. The terminal sends the i/p to the read() function only when line delimiters (NEWLINE, EOF etc.) are encountered.

In non canonical mode, the above cannonical features are all disabled. The i/p is availabe immediately to the program. To control the i/p we use the VMIN and VTIME indices of c\_cc array ( discussed below ).
A more detailed explanation of cannonical and non cannonical can be found on the man page of termios.h.
The ICANON flag is disabled as we need to process the input character by character instead of a string with a newline.

##3. IXON , ISIG and IEXTEN
The IXON flag controls the software i/p flow. The Ctrl+S and Ctrl+Q ( on most machines ) are used to signal the STOP and START of an i/p.
Ctrl+S pauses the terminal i/p. Any input given after the pause will still be recorded in an i/p buffer and is fed back to terminal after we signal START with Ctrl+Q.  This flag is disabled.


The ISIG command controls the transmission of the  interrupt signal ( Ctrl+C ) and the stop signal (Ctrl + Z ) to the terminal. When disabled the program no longer stops when we hit CTRL+C or Ctrl + Z, which allows us to process these inputs.

The IEXTEN flag controls the function of Ctrl + V only when the flag ICANON is set. Ctrl + V allows us to type in a escape character to allow characters like Ctrl + c , Ctrl + Z etc. to be fed as input.


##4. Carriage return ( '\r' ) and Newline ( '\n' )
Carraige return "returns" the i/p cursor to the begining of the line and has the ASCII value 13.
Newline starts a new line starting at the current position of the cursor.

Normally, when we hit the Enter key it produces a newline with the cursor on the begining of the line. However, the Enter key denotes a carriage return ( '\r' ) and so does the combination Ctrl + M. What happens when we hit Enter is that the terminal automatically translated the carriage return ('\r') into a carriage return followed by a newline ( i.e '\r''\n' ) which sets the cursor at the begining of the newline. The newline character ( '\n' ) has ASCII value 10 and is produced by a combination of Ctrl+J. 

To disable this behaviour of the terminal during input,  we set the ICRNL bit of the c\_iflag off. So, when we hit enter during i/p it no longers start a newline with a cursor at the begining. Instead the character '^M' is shown which corresponds to Ctrl+M which is carriage return.

The terminal also does a similar thing in the output side. To disable this behaviour we set the OPOST flag in c\_oflag ( 'o' meaning output ) to off which disables all kinds of output processing. ( The above is just one of the example. )

So, now we need explicitly send the sequence "\r\n" in printf statements for a newline to begin at the leftmost position.

##5. Controlling input in cannonical mode
In non cannonical mode, we control the i/p by using VTIME and VMIN. These are indices of the array c\_cc member of the termios struct. 
VMIN sets the min. amount of bytes that need to be present at the i/p before the i/p is sent to read().
VTIME sets the timeout duration for i/p. The values are given in tenth of a second. 



##List of important escape sequences

Esc : \x1b or ^[ or \E
Up Arrow: <Esc>[A
Down Arrow: <Esc>[B
Right Arrow: <Esc>[C
Left Arrow: <Esc>[D
Page Up: <Esc>[5
Page Down: <Esc>[6
Home : <Esc>[1~ or <Esc>[7~ or <Esc>[H or <Esc>OH 
End: <Esc>[4~ or <Esc>[8~ or <Esc>[F or <Esc>OF 
Delete : <Esc>[3~
Insert : <Esc>[2~
F2  : <Esc>OQ
F3  : <Esc>OR
F4  : <Esc>OS
F5  : <Esc>[15~
F6  : <Esc>[17~
F7  : <Esc>[18~
F9  : <Esc>[20~
F10 : <Esc>[21~
F12 : <Esc>[24~
