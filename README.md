# C_Web
A simple web server written in C

##Why

I learned a little about sockets in one of my classes and thought making a web server would be neat. Then I found <a href="https://github.com/fekberg/GoHttp">GoHttp</a> which made me realize that I just needed to interpret text to read requests and made me actually start making one. I did reference GoHttp a little, but I tried to mainly make this version myself.

##Compiling

At the moment there is no make file, but compilation is a simple command.

Compilation:

    gcc -Wall -o C_Web main.c

##Command Line Options

    Command Options
    Use '-d' to run as a daemon
    Use '-p <number>' to specifiy a port (51717 is the default)
    Use 'r <dir>' to specify the www root directory
    Use '-help' to see this output
