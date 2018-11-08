# CS 444 - Group1

Anton Synytsia<br/>
Eytan Brodsky<br/>
David Jansen<br/>

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

## Assignment 3
### Compiling Writeup
```bash
cd Writeup3/
make

# optional cleanup
make clean
```

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

## Useful Commands
### Latex Version
To use latest version of latex, execute the following command:
```bash
PATH=/usr/local/apps/tex_live/current/bin/x86_64-linux:$PATH
```

### Compressing
Navigate to folder that contains the git repository folder and execute the following command:
```bash
tar cvjf CS444_project#_1.tar.bz2 rep_folder_name
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
