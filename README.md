# alsh
This is the home of "ALegend shell", aka **alsh**

My own unix shell implemented in C.

Currently doesnt support multiple commands using "&".\
Redirection of output(and as a side-effect error redirection) is supported.


## Usage
Compile alsh.c using C compiler. \
Then execute the binary!

#### Type commands right away...

```
alsh> path /bin
alsh> ls -al . > output.txt
alsh> cat output.txt
alsh> rm -f output.txt
```

OR

#### Use a batch file
```
./alsh filename.txt
```
It will read commands line by line from the file and execute them serially.

PS:
### OVERWRITE Path using:
```alsh> Path <dir1> <dir2>```

originally path is set to /bin, which is a set of directories to look in, for the executables of the command you type in the shell.

