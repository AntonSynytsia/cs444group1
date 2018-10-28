# CS 444 - Group1

Anton Synytsia<br/>
Eytan Brodsky<br/>
David Jansen<br/>

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
To use latest version of latex, prepend to your path:
```bash
PATH=/usr/local/apps/tex_live/current/bin/x86_64-linux:$PATH
```

### Compressing
Navigate to git repository, for example:
```
cd /scratch/fall2018/group1/
```

Execute command:
```
tar -cvjSf cs444group1.tar.bz2 *
```

### Generating Table
Navigate to git repository and execute the following command:
```
./ThirdParty/latex-git-log --width=8.5 --lang=en > log.tex

```
Then copy the content within log.tex into your writeup
