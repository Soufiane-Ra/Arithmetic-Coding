# Arithmetic Coding Compression Program

This program implements arithmetic coding compression and decompression algorithms using Markov chains in C. It takes command line arguments to specify whether to compress or decompress one or more files. Additionally, it can display a logo and run in verbose mode.

## Compilation
To compile the program, run the following command:

```
gcc -o ac_comp ac_comp.c
```
## Usage

The program accepts several command line arguments:

   - `c`: specifies compression mode
  
   - `d`: specifies decompression mode
  
   - `v`: enables verbose mode (provides information about runtime and compression rate)
  
   - `p`: displays the logo
  
   - `file(s)`: names of one or more files to be processed

For example, to compress a single file named input.txt or multiple files, you would run:

```
./ac_comp c input.txt input1.txt input2.txt .....
```
Which outputs the compressed files: `input.txtacc`, `input1.txtacc`, `input2.txtacc` . . . , and to decompress these files use the command:

```
./ac_comp d input.txtacc input1.txtacc input2.txtacc 
```
Which outputs the decompressed files: `dinput.txt`, `dinput1.txt`, `dinput2.txt` . . . , and to get the runtime and the compression or print a logo use `v` and `p`:

```
./ac_comp cvp Test.txt
```
Outputs:
```
__________________________________________________________
----------------------}**<••~*~••>**{---------------------
           ______           _________      _________     
           / /\ \          /         |    /         |    
          / /  \ \        /  /-------|   /  /-------|    
         / /    \ \      /  /           /  /             
        / /      \ \    |  |           |  |              
       / /--------\ \   |  |           |  |              
      / /----------\ \   \  \           \  \             
     / /            \ \   \  \-------|   \  \-------|    
    / /              \ \   \_________|    \_________|    
----------------------}**<••~*~••>**{---------------------
----------------------------------------------------------
Input file: Test.txt
Compressed file: Test.txtacc
Size of original file: 1048576 bytes 
Size of compressed file: 443962 bytes
Compression rate: 42.34% 
Time:

real  0m0.923s
clocks 0m0.918s
----------------------------------------------------------
```
And for decompression:
```
./ac_comp dvp Test.txt
```
Outputs:
```
__________________________________________________________
----------------------}**<••~*~••>**{---------------------
           ______           _________    ___________         
           / /\ \          /         |   |          \ 
          / /  \ \        /  /-------|   |  |-----\  \  
         / /    \ \      /  /            |  |      \  \  
        / /      \ \    |  |             |  |       |  |  
       / /--------\ \   |  |             |  |       |  |  
      / /----------\ \   \  \            |  |      /  /    
     / /            \ \   \  \-------|   |  |-----/  /     
    / /              \ \   \_________|   |__________/        
----------------------}**<••~*~••>**{---------------------
----------------------------------------------------------
Input file: Test.txtacc
Decompressed file: dTest.txt
Time:

real  0m1.447s
clocks 0m1.439s
----------------------------------------------------------
```

