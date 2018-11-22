# CS 444 - Group 1

Anton Synytsia<br/>
Eytan Brodsky<br/>
David Jansen<br/>


## Assignment 4
Refer to the following instructions for compiling the writeup:
```bash
cd Writeup4/
make

# optional cleanup
make clean
```

After compiling, <tt>writeup4.pdf</tt> should appear in <tt>/Result</tt> folder.

Writeup 4 is based on writeup 3 but with added information regarding the solution.


## Concurrency 3
Refer to the following instructions for compiling concurrency, including the writeup:
```bash
cd Concurrency3/
make
```
After compiling, the concurrency executables and <tt>concurrency.pdf</tt> will be in the folder. For convenience, <tt>concurrency.pdf</tt> is also copied to <tt>/Result</tt> folder.

Run this command before pushing git.
```bash
# optional cleanup
make clean
```


## Assignment 3
Refer to the following instructions for compiling the writeup:
```bash
cd Writeup3/
make

# optional cleanup
make clean
```
After compiling, <tt>writeup3.pdf</tt> should appear in <tt>/Result</tt> folder.


## Assignment 2
### Compiling Concurrency
```bash
cd Concurrency2/
make

# optional cleanup
make clean
```

### Compiling Writeup
```bash
cd Writeup2/
make

# optional cleanup
make clean
```


## Assignment 1
### Compiling Concurrency
```bash
cd Concurrency1/
make

# optional cleanup
make clean
```

### Compiling Writeup
```bash
cd Writeup1/
make

# optional cleanup
make clean
```

## Using Qemu Virtual Machine
### Starting
```bash
cd /scratch/fall2018/group1/
./qemu-run.sh
```
### Logging In
```bash
root
```
### Exiting Out
```bash
reboot
```


## Useful Commands
### Latex Version
To use latest version of latex, execute the following command:
```bash
PATH=/usr/local/apps/tex_live/current/bin/x86_64-linux:$PATH
```

### Compressing
Navigate to folder that contains the git repository folder and execute the following command:
```bash
tar cvjf CS444_project#_1.tar.bz2 rep_folder_name --exclude .git
```
### Extracting
```bash
tar xvjf CS444_project#_1.tar.bz2
```

### Generating Table
Navigate to git repository and execute the following command(s):
```bash
chmod +x ./ThirdParty/latex-git-log
./ThirdParty/latex-git-log --width=8.5 --lang=en > log.tex

```
Then, copy the content within log.tex into the writeup.
