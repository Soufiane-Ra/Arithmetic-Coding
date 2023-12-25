#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX 256
#define whole 4294967296 //2^32
#define half 2147483648
#define quarter 1073741824


// Stores upper and lower bounds that represent the probability range for each character.
typedef struct pair{
    unsigned long low;
    unsigned long high;
}pair;

// Stores CLI arguments.
typedef struct ptargs{
    char* options;
    char* filename;
}ptargs;


// Grows the probability range for "character 'c' appearing after character 'pc'" by 1 and offsets the following probability ranges related to 'pc' by 1, this function is used to dynamically create the Markov conditional probability table as we compress the file.
void change_p(pair* tabp,int c,int pc){
    (*(tabp+pc*(MAX+1)+c)).high= (*(tabp+pc*(MAX+1)+c)).high+1;
    for (int i=c+1;i<MAX+1;i++){
        (*(tabp+pc*(MAX+1)+i)).low = (*(tabp+pc*(MAX+1)+i)).low+1;
        (*(tabp+pc*(MAX+1)+i)).high= (*(tabp+pc*(MAX+1)+i)).high+1;
    }
}

// Converts chunks of 8 binary digits stored as characters in the 'bits' string ('001011....') into bytes (charachters) and writes them to the output file. The number of bytes written to the output file is tracked using the 'cc'.

void bitabit(FILE* fptr,char* byte,char* bits,unsigned long* cc){
    int i = strlen(byte);
    int j = strlen(bits);
    unsigned char b;
    long l;
    while (j>0){
        if (i == 8){
            l = strtol(byte, 0, 2);
            b = l & 0xffl;
            fwrite(&b, 1, 1, fptr);
            *(cc)=*(cc)+1;
            i=0;
        }
        *(byte+i)=*(bits+strlen(bits)-j);
        i++;
        j--;
    }
    if (i == 8){
        l = strtol(byte, 0, 2);
        b = l & 0xffl;
        fwrite(&b, 1, 1, fptr);
        *(cc)=*(cc)+1;
        i=0;
    }
    *(byte+i)='\0';
}

// Adds zeros to the end of a 'byte', used when dealing with partial bytes at the end of a compressed file.

void padding (FILE* fptr,char* byte,unsigned long* cc){
    int i = strlen(byte);
    while (i<8){
        *(byte+i)='0';
        i++;
    }
    unsigned char b;
    long l;
    l = strtol(byte, 0, 2);
    b = l & 0xffl;
    fwrite(&b, 1, 1, fptr);
    *(cc)=*(cc)+1;

}

// Factorial multiplication.

unsigned long long pawaw(int i,int j){
    if(j != 0){
        return i*pawaw(i,j-1);
    }
    return 1;
}

// Return a string that contains binary digits of lenght s+1 (1000...s times or 0111...s times).

char* bstring(short x,int s){
    char* string = malloc((s+2)*sizeof(char));
    *(string)=(char)x;
    if (x){
        *(string)=49;
        for(int i=1;i<s+1;i++){
            *(string+i)=48;
        }
    }else{
        *(string)=48;
        for(int i=1;i<s+1;i++){
            *(string+i)=49;
        }
    }
    *(string+s+1)='\0';
    return string;
}

// Rounded division operation (towards zero).

unsigned long long roundiv(unsigned long long a ,unsigned long long b){
    return a/b ;
}

// Compresses the file using arithmetic coding.

unsigned long encode(pair* tabp,char* in,char* out){
    unsigned long cc=0; // Counter to keep track of the total number of bytes written to the output file.
    char* byte = malloc(sizeof(char)*8); // A buffer that stores the current chunk of 8 bits waiting to be written to the output file.
    FILE* fptr; // Pointer to input.
    FILE* fptrout; // Pointer to output file.
    if ((fptr = fopen(in,"r")) == NULL){
        printf("Error! opening file \n");
        exit(1);
    }
    if ((fptrout = fopen(out,"w")) == NULL){
        printf("Unable to open a temporary file to write!!\n");
        fclose(fptrout);
        exit(1);
    }
    int pci=fgetc(fptr); // Check if file is empty.
    if (pci==EOF){
        free(byte);
        fclose(fptr);
        fclose(fptrout);
        return cc;
    }
    unsigned char pc=(unsigned char)pci; // Stores the previous character, used to determine which Markov probability ranges to use.
    fwrite(&pc,1,1,fptrout); // writes first character to output file uncompressed
    cc++;
    int c = fgetc(fptr); // Stores the current character to be encoded.
    if(c==EOF){
        free(byte);
        fclose(fptr);
        fclose(fptrout);
        return cc;
    }

    // Narrows down the range 'w'='b-a' that will be used to represent the compressed data until 'b'<1/2 (we write 0 to the output file) or 'a'>=1/2 (we write 1 to the output file), then we rescale 'w' ('b'>=1/2 and 'a'<1/2) to limit the data needed to represent the probility range.

    unsigned long long a= 0;
    unsigned long long b= whole;
    unsigned long long w=0;
    int s=0; // used to represent chunks of similar bits.
    char* str; // stores the output as binary characters (011010...).
    for(int i=0;;i++){
        w=b-a;
        b=a+roundiv(w*(*(tabp+pc*(MAX+1)+c)).high,(*(tabp+pc*(MAX+1)+MAX)).high);
        a=a+roundiv(w*(*(tabp+pc*(MAX+1)+c)).low,(*(tabp+pc*(MAX+1)+MAX)).high);
        while(a>half || b<half){
            if (b<half){
                // 0111...s times.
                str = bstring(0,s);
                // write bits to file.
                bitabit(fptrout,byte,str,&cc);
                s=0;
                //rescale 'w' to limit the data usage.
                a=2*a;
                b=2*b;
            }else{
                //1000...s times.
                str = bstring(1,s);
                // write bits to file.
                bitabit(fptrout,byte,str,&cc);
                s=0;
                //rescale 'w' to limit the data usage.
                a=2*(a-half);
                b=2*(b-half);
            }
            free(str);
        }
        // Special case: reduces the number of iterations needed to get to 'b'<1/2 or 'a'>=1/2 and speeds up compression.
        while(a>quarter && b<(unsigned long long)3*quarter){
            s=s+1;
            a=2*(a-quarter);
            b=2*(b-quarter);
        }
        // EOF
        if (c==256){
            break;
        }
        // Grows the probability range for "character 'c' appearing after character 'pc'" by 1.
        change_p(tabp,c,pc);
        // current character 'c' becomes the previous character 'pc'.
        pc=c;
        c = fgetc(fptr);
        if (c==EOF){
            c=256;
        }
    }
    // writting the last bytes to the output file.
    s=s+1;
    if(a<=quarter){
        str=bstring(0,s);
    }else{
        str=bstring(1,s);
    }
    bitabit(fptrout,byte,str,&cc);
    // padding the last byte left with zeros to complete 8 bits.
    padding(fptrout,byte,&cc);
    free(str);
    free(byte);
    fclose(fptr);
    fclose(fptrout);
    // return size of the compressed file.
    return cc;
}

// Decompressing a file that has been previously compressed using the encode() function.

int decode(char* out,char*in){
    // Open input and output files.
    FILE* fptr;
    FILE* fptrout;
    if ((fptr = fopen(out,"r")) == NULL){
        printf("Error! opening file\n");
        exit(1);
    }
    if ((fptrout = fopen(in,"w")) == NULL){
        printf("Unable to open a temporary file to write!!\n");
        fclose(fptrout);
        exit(1);
    }

    // Set initial ranges for all elements in the Markov probability matrix to [n,n+1].
    pair * tabp=calloc(MAX*(MAX+1),sizeof(pair));
    for(int ii=0;ii<MAX;ii++){
        (*(tabp+ii*(MAX+1))).low = 0 ;
        (*(tabp+ii*(MAX+1))).high = 1;
        for (int k = 1;k<MAX+1;k++){
            (*(tabp+ii*(MAX+1)+k)).low = (*(tabp+ii*(MAX+1)+k-1)).high;
            (*(tabp+ii*(MAX+1)+k)).high= (*(tabp+ii*(MAX+1)+k-1)).high + 1;
        }
    }

    // Check if file is empty
    int pci=fgetc(fptr);
    if (pci==EOF){
        fclose(fptr);
        fclose(fptrout);
        free(tabp);
        return 0;
    }

    unsigned char pc=(unsigned char)pci; // Variable used to store the most recently decoded character.
    fwrite(&pc,1,1,fptrout);
    unsigned long long a= 0;
    unsigned long long b= whole;
    // Stores lower and upper bounds of the next probability range
    unsigned long long a0= 0;
    unsigned long long b0= whole;
    unsigned long long w=0;
    unsigned long long z=0; // Stores 32 bits of the compressed data
    int i = 1;
    int c = fgetc(fptr); //used to read 1 byte of the compressed data.
    if (c==EOF){
        fclose(fptr);
        fclose(fptrout);
        free(tabp);
        return 0;
    }
    int precision = 32;
    while (i<=precision){
        if((!!((c << (i-1)%8) & 0x80)) == 1){
            z = z + pawaw(2,precision-i);
        }
        i++;
        if((i-1)%8 == 0){
            c = fgetc(fptr);
        }
    }
    int boo = 0; // Used to checks for EOF character;
    // Finds wich probability range 'z' belongs to and writes the character corresponding to that range to the output file then scales up 'a','b' and 'z' to delete bits that have been decoded then adds new bits to 'z'

    while(1){
        //Finds wich probability range contains 'z'
        for(int j=0;j<MAX+2;j++){
            w=b-a;
            b0=a+roundiv(w*(*(tabp+pc*(MAX+1)+j)).high,(*(tabp+pc*(MAX+1)+MAX)).high);
            a0=a+roundiv(w*(*(tabp+pc*(MAX+1)+j)).low,(*(tabp+pc*(MAX+1)+MAX)).high);
            if (a0<=z && z<b0 ){
                if(j==256){
                    boo=1;
                    break;
                }
                //writes the character corresponding to the range [a0,b0) to the output file
                fprintf(fptrout,"%c",j);
                //changes the Markov probability table to create the table that was used to encode the next letter
                change_p(tabp,j,pc);
                pc=j;
                a=a0;
                b=b0;
                break;
            }
        }
        if (boo){
            // EOF character
            break;
        }

        // delete bits from 'z' that have been decompressed then adds new ones
        while(a>half || b<half){
            if (b<half){
                z=2*z;
                a=2*a;
                b=2*b;
            }else{
                z=2*(z-half);
                a=2*(a-half);
                b=2*(b-half);
            }
            if ( (!!((c << (i-1)%8) & 0x80)) == 1){
                z++;
            }
            i++;
            if((i-1)%8 == 0){
                c = fgetc(fptr);
            }
        }
        while(a>quarter && b<(unsigned long long)3*quarter ){
            z=2*(z-quarter);
            a=2*(a-quarter);
            b=2*(b-quarter);
            if ((!!((c << (i-1)%8) & 0x80)) == 1){
                z=z+1;
            }
            i++;
            if((i-1)%8 == 0){
                c = fgetc(fptr);
            }
        }
    }
    fclose(fptr);
    fclose(fptrout);
    free(tabp);
    return 0;
}

// Function used to set the appropriate flags depending on thecommand-line arguments
// c/d: Compress and decompress, v: verbose mode , p: print logo

void init_bool(unsigned char* bo,char* c ){
    for(int i=0;i<strlen(c);i++){
        if(*(c+i)=='c'){
            *(bo)=1;
            continue;
        }
        if(*(c+i)=='d'){
            *(bo+1)=1;
            continue;
        }
        if(*(c+i)=='v'){
            *(bo+2)=1;
            continue;
        }
        if(*(c+i)=='p'){
            *(bo+3)=1;
            continue;
        }
        free(bo);
        printf("No option : %c \n",*(c+i));
        exit(1);
    }
    if(*(bo)==*(bo+1)){
        free(bo);
        printf("Impossible option configuration\n");
        exit(1);
    }
}

//main() function executed by each thread

void* main0(void* args){
    struct timeval b;
    clock_t s;
    unsigned long time_in_microsb;
    unsigned char* bo=calloc(4,sizeof(unsigned char));
    init_bool(bo,((ptargs*)args)->options);
    if(*(bo+2)){
        s=clock();
        gettimeofday(&b,NULL);
        time_in_microsb = 1000000 * b.tv_sec + b.tv_usec;
    }
    if(*bo){
        FILE* fptr;
        if ((fptr = fopen(((ptargs*)args)->filename,"r")) == NULL){
            printf("Error! opening file");
            exit(1);
        }
        pair * tabpp=calloc(MAX*(MAX+1),sizeof(pair));
        for(int ii=0;ii<MAX;ii++){
            (*(tabpp+ii*(MAX+1))).low = 0 ;
            (*(tabpp+ii*(MAX+1))).high = 1;
            for (int k = 1;k<MAX+1;k++){
                (*(tabpp+ii*(MAX+1)+k)).low = (*(tabpp+ii*(MAX+1)+k-1)).high;
                (*(tabpp+ii*(MAX+1)+k)).high= (*(tabpp+ii*(MAX+1)+k-1)).high + 1;
            }
        }
        fclose(fptr);
        char* out=malloc(strlen(((ptargs*)args)->filename)+4);
        strcpy(out,((ptargs*)args)->filename);
        *(out+strlen(((ptargs*)args)->filename))='a';
        *(out+strlen(((ptargs*)args)->filename)+1)='c';
        *(out+strlen(((ptargs*)args)->filename)+2)='c';
        *(out+strlen(((ptargs*)args)->filename)+3)=0;
        unsigned long cl=encode(tabpp,((ptargs*)args)->filename,out);
        unsigned long nbc=0;
        if (cl!=0){
            nbc++;
        }
        for(int i = 0;i<MAX;i++){
            nbc=nbc+(*(tabpp+i*(MAX+1)+MAX)).low-256;
        }
        if(*(bo+3)){
            printf("__________________________________________________________\n");
            printf("----------------------}**<••~*~••>**{---------------------\n");
            printf("           ______           _________      _________     \n");
            printf("           / /\\ \\          /         |    /         |    \n");
            printf("          / /  \\ \\        /  /-------|   /  /-------|    \n");
            printf("         / /    \\ \\      /  /           /  /             \n");
            printf("        / /      \\ \\    |  |           |  |              \n");
            printf("       / /--------\\ \\   |  |           |  |              \n");
            printf("      / /----------\\ \\   \\  \\           \\  \\             \n");
            printf("     / /            \\ \\   \\  \\-------|   \\  \\-------|    \n");
            printf("    / /              \\ \\   \\_________|    \\_________|    \n");
            printf("----------------------}**<••~*~••>**{---------------------\n");
        }
        if(*(bo+2)){
            struct timeval e;
            gettimeofday(&e,NULL);
            unsigned long time_in_microse = 1000000 * (e.tv_sec%60) + e.tv_usec;
            printf("----------------------------------------------------------\nInput file: %s\nCompressed file: %s\nSize of original file: %ld bytes \nSize of compressed file: %ld bytes\nCompression rate: %.2f%% \n",((ptargs*)args)->filename,out,nbc,cl,(float)(cl)/(nbc)*100.0);
        }
        free(tabpp);
        free(out);
    }else{
        char* in = malloc(strlen(((ptargs*)args)->filename)-1);
        *(in)='d';
        *(in+1)= 0;
        strncat(in,((ptargs*)args)->filename,strlen(((ptargs*)args)->filename)-3);
        decode(((ptargs*)args)->filename,in);
        if(*(bo+3)){
            printf("__________________________________________________________\n");
            printf("----------------------}**<••~*~••>**{---------------------\n");
            printf("           ______           _________    ___________         \n");
            printf("           / /\\ \\          /         |   |          \\ \n");
            printf("          / /  \\ \\        /  /-------|   |  |-----\\  \\  \n");
            printf("         / /    \\ \\      /  /            |  |      \\  \\  \n");
            printf("        / /      \\ \\    |  |             |  |       |  |  \n");
            printf("       / /--------\\ \\   |  |             |  |       |  |  \n");
            printf("      / /----------\\ \\   \\  \\            |  |      /  /    \n");
            printf("     / /            \\ \\   \\  \\-------|   |  |-----/  /     \n");
            printf("    / /              \\ \\   \\_________|   |__________/        \n");
            printf("----------------------}**<••~*~••>**{---------------------\n");
        }
        if(*(bo+2)){
            printf("----------------------------------------------------------\nInput file: %s\nDecompressed file: %s\n",((ptargs*)args)->filename,in);
        }
        free(in);
    }
    if(*(bo+2)){
        struct timeval e;
        clock_t f;
        f=clock();
        gettimeofday(&e,NULL);
        unsigned long time_in_microse = 1000000 * e.tv_sec + e.tv_usec;
        printf("Time:\n\nreal  %ldm%.3fs\nclocks %ldm%.3fs\n----------------------------------------------------------\n",(e.tv_sec-b.tv_sec)/60,(float)((time_in_microse-time_in_microsb)%60000000)/1000000,(f - s)/60000000,((float)((f - s)%60000000)) /1000000);
    }
    free(bo);
    pthread_exit(NULL);
}

// creates threads for compressing and decompressing multiple files at the same time

int main(int argc, char** argv){
    pthread_t threads[argc-2];
    ptargs* args[argc-2];
    for(int i=0;i<argc-2;i++){
        args[i]=malloc(sizeof(ptargs));
        args[i]->options=malloc(strlen(*(argv+1))+1);
        args[i]->filename=malloc(strlen(*(argv+2+i))+1);
        memcpy(args[i]->options,*(argv+1),strlen(*(argv+1))+1);
        memcpy(args[i]->filename,*(argv+2+i),strlen(*(argv+2+i))+1);
    }
    int rc;
    long t;
    for(t=0; t<argc-2; t++){
        rc = pthread_create(&threads[t],NULL,main0,(void*)args[t]);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n",rc);
            exit(-1);
        }
    }
    pthread_exit(NULL);
    for(int i=0;i<argc-2;i++){
        free(args[i]->options);
        free(args[i]->filename);
        free(args[i]);
    }
    return 0;
}