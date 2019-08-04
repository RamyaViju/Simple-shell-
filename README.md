# Simple-shell-
README for ush parser. 

Vincent W. Freeh
=====================

This a parser for a csh-like syntax.  It is to be used for the ush
shell project.

There are five files:

 . README	this file
 . Makefile	
 . main.c	driver program that reads and prints commands
 . parse.c	define parsing function and other support functions
 . parse.h

To use in your program, you must include parse.h and call the function
parse(), which return a Pipe data structure.  (See main.c)  The Pipe
contains all the information in a line of input to a shell.

==========================
Author: Ramya Vijayakumar
=========================

Added another file to the above list

 . main.h	a header file for main.c

Typing the command "quit" in the shell terminates the shell
