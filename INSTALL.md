This is a brief installation guide. A C compiler is generally required for the installation. In these instructions, we use the “clang” compiler, which is available on Linux, macOS and Windows.

Under Linux and macOS, compile the file in the terminal with the following command:
clang -m64 localtalk.c -o localtalk

After successful compilation, you can open the file with:
./localtalk
execute the file. 
The program requires 1 argument when running. Use:
./localtalk -s
to send data from your computer to another computer.
Use the following command:
./localtalk -l
to switch your computer to “listening mode”. This means that after successfully executing the command, you can receive data from another computer on which you executed the first command.

Finally, it is important to emphasize that the two computers must be on the same wifi.
