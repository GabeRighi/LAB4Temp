/******** C13.2.a: TCP server.c file ********/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#define MAX 256
#define SERVER_HOST "localhost"
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
struct sockaddr_in server_addr, client_addr;
int mysock, csock; // socket descriptors
int r, len, n; // help variables
struct stat mystat, *sp;
char *t1 = "xwrxwrxwr ";
char *t2 = "--------";
char outgoingBuffer[MAX];

void writeToClient()
{
    n = write(csock, outgoingBuffer, MAX);
    printf("server: wrote n=%d bytes; ECHO=%s\n", n, outgoingBuffer);
    printf("server: ready for next request\n");
}

void insertSpecialEndingChar()
{
    outgoingBuffer[0] = '\x04';
    outgoingBuffer[1] = '\0';
}
int lsfile(char *fname)
{
    int position = 0;
    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ( (r = lstat(fname, &fstat)) < 0)
    {
        printf("can't stat %s\n", fname);
        strcat(outgoingBuffer,"Error lsing file");
        return -1;
    }
    if ((sp->st_mode & 0xF000) == 0x8000) // if (S ISREG())
        outgoingBuffer[position++] = '-';
    if ((sp->st_mode & 0xF000) == 0x4000) // if (S ISDIR())
        outgoingBuffer[position++] = 'd';
    if ((sp->st_mode & 0xF000) == 0xA000) // if (S ISLNK())
        outgoingBuffer[position++] = 'l';
    for (i=8; i >= 0; --i )
    {
        if (sp->st_mode & (1 << i)) // print r|w|x
        {
            outgoingBuffer[position++] = t1[i];
        }
        else
        {
            outgoingBuffer[position++] = t2[i];
        }
    }
    char format[10];
    format[0] = '\0';
    outgoingBuffer[position] = '\0';
    // printf("%4d ",sp->st_nlink); // link count
    sprintf(format,"%4d ",sp->st_nlink);
    strcat(outgoingBuffer,format);
    // printf("%4d ",sp->st_gid); // gid
    sprintf(format, "%4d ",sp->st_gid);
    strcat(outgoingBuffer,format);
    // printf("%4d ",sp->st_uid); // uid
    sprintf(format,"%4d ",sp->st_uid);
    strcat(outgoingBuffer,format);
    // printf("%8d ",sp->st_size); // file size
    sprintf(format,"%8d ",sp->st_size);
    strcat(outgoingBuffer,format);

    // print time
    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
    ftime[strlen(ftime)-1] = 0; // kill \n at end
    // printf("%s ",ftime);
    strcat(outgoingBuffer,ftime);
    // print name
    // printf("%s", basename(fname)); // print file basename
    strcat(outgoingBuffer, " ");
    strcat(outgoingBuffer,basename(fname));
    // print > linkname if symbolic file
    if ((sp->st_mode & 0xF000)== 0xA000)
    {
    // use readlink() to read linkname
        strcat(outgoingBuffer,"-> ");
        char linkname[256];
        readlink(fname,linkname,256);
        // printf("-> %s", linkname); // print linked name
        strcat(outgoingBuffer,linkname);
    }
    writeToClient();
    return 1;
    // printf("\n");

}

int lsdir(char *dname)
{
    DIR *dir;
    struct dirent *dp;

    if ((dir = opendir (dname)) == NULL) {
        perror ("Cannot open .");
        exit (1);
    }


    while ((dp = readdir (dir)) != NULL)
    {
       
        char tempString[256];
        tempString[0] = '\0';
        strcat(tempString,dname);
        strcat(tempString,"/");
        strcat(tempString,dp->d_name);
        lsfile(tempString);
        tempString[0] = '\0';
    }
}



int checkForLocalCommand(char* command, char* passedPath)
{
    outgoingBuffer[0] = '\0';
    if(!strcmp("cat",command))
    {
        FILE* fp;
        char* line = NULL;
        size_t len = 256;
        fp = fopen(passedPath,"r");
        if(fp == NULL)
        {
            printf("Error opening file\n");
            return 1;
        }

        while(getline(&line,&len,fp)!= -1)
        {
            printf("%s",line);
        }
        printf("\n");
        fclose(fp);
        if(line)
        {
            free(line);
        }
        return 1;
    }
    else if(!strcmp("cd",command))
    {
        if(!chdir(passedPath))
        {
            strcpy(outgoingBuffer,"Dir Changed!");
            // writeToClient(outgoingBuffer);
        }
        else
        {
            strcpy(outgoingBuffer,"Error Changing Dir!");
            // printf("Error changing DIR\n");
        }
        writeToClient();
        insertSpecialEndingChar();
        writeToClient();
        return 1;
    }
    else if (!strcmp("pwd", command))
    {
        char buf[256];
        if(getcwd(buf,256))
        {
            strcpy(outgoingBuffer,buf);
        }
        else
        {
            strcpy(outgoingBuffer,"Error getting CWD!");
        }
        writeToClient();
        insertSpecialEndingChar();
        writeToClient();
        return 1;
    }
    else if(!strcmp("mkdir",command))
    {
        if(!mkdir(passedPath, 0755))
        {
            strcpy(outgoingBuffer,"Dir Created!");
        }
        else
        {
            strcpy(outgoingBuffer,"Error Creating Dir.");
        }
        writeToClient();
        insertSpecialEndingChar();
        writeToClient();
        return 1;
    }
    else if(!strcmp("rmdir",command))
    {
        if(!rmdir(passedPath))
        {
            strcpy(outgoingBuffer,"Dir Deleted!");
        }
        else
        {
            strcpy(outgoingBuffer,"Could not delete dir!");
        }
        writeToClient();
        insertSpecialEndingChar();
        writeToClient();
        return 1;
    }
    else if(!strcmp("rm",command))
    {
        if(!unlink(passedPath))
        {
            strcpy(outgoingBuffer, "Deleted File!");
        }
        else
        {
            strcpy(outgoingBuffer,"Could not delete.");
        }
        writeToClient();
        insertSpecialEndingChar();
        writeToClient();
        return 1;
    }
    else if(!strcmp("ls",command))
    {
        struct stat mystat, *sp = &mystat;
        int r;
        char *filename, path[1024], cwd[256];
        filename = "./"; // default to CWD
        if (passedPath)
        {
            filename = passedPath;
        }

        if (r = lstat(filename, sp) < 0)
        {
            printf("no such file %s\n", filename);
            exit(1);
        }
        strcpy(path, filename);
        if (path[0] != '/')
        { // filename is relative : get CWD path
            getcwd(cwd, 256);
            strcpy(path, cwd); strcat(path, "/"); strcat(path,filename);
        }
        if (S_ISDIR(sp->st_mode))
            lsdir(path);
        else
            lsfile(path);

        insertSpecialEndingChar();
        writeToClient();

        return 1;
    }
    else if(!strcmp("get",command))
    {
        struct stat fstat, *sp;
        FILE* fp;
        sp = &fstat;
        size_t len = MAX;
        outgoingBuffer[0] = '\0';
        // passedPath[strlen(passedPath)-1] = '\0';
        if ( (r = lstat(passedPath, &fstat)) < 0)
        {
            printf("COULD NO LSTAT");
            outgoingBuffer[0] = '\0';
            writeToClient();
            // strcat(outgoingBuffer,"Error lsing file");
            return -1;
        }
        fp = fopen(passedPath,"r");
        printf("passed path: %s\n",passedPath);
        if(fp == NULL)
        {
            printf("FP NULL\n");
            outgoingBuffer[0] = '\0';
            writeToClient();
            return -1;
        }
        snprintf(outgoingBuffer,MAX,"%u",(unsigned)sp->st_size);
        writeToClient();
        while(fread(outgoingBuffer,1,MAX,fp)) //fread(outgoingBuffer,MAX,1,fp)
        {
            writeToClient();
            printf("BLP: %s\nlen: %u\n",outgoingBuffer,len);
        }
        printf("BLP: %d",ferror(fp));        printf("closing\n");
        fclose(fp);
        return 1;
    }
    else
    {
        return 0;
    }
}
int server_init()
{
    printf("================== server init ======================\n");
    // create a TCP socket by socket() syscall
    printf("1 : create a TCP STREAM socket\n");
    mysock = socket(AF_INET, SOCK_STREAM, 0);

    if (mysock < 0)
    {
        printf("socket call failed\n"); exit(1);
    }
    printf("2 : fill server_addr with host IP and PORT# info\n");
    // initialize the server_addr structure
    server_addr.sin_family = AF_INET; // for TCP/IP
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // This HOST IP
    server_addr.sin_port = htons(SERVER_PORT); // port number 1234

    printf("3 : bind socket to server address\n");
    r = bind(mysock,(struct sockaddr*)&server_addr,sizeof(server_addr));
    if (r < 0)
    {
        printf("bind failed\n"); exit(3);
    }
    printf(" hostname = %s port = %d\n", SERVER_HOST, SERVER_PORT);
    printf("4 : server is listening ....\n");
    listen(mysock, 5); // queue length = 5
    printf("=================== init done =======================\n");
}

int main()
{
    char cwd[256];
    getcwd(cwd,256);
    chroot(cwd);
    char line[MAX];
    char testLine[MAX];
    server_init();
    while(1)
    { // Try to accept a client request
        printf("server: accepting new connection ....\n");
        // Try to accept a client connection as descriptor newsock
        len = sizeof(client_addr);
        csock = accept(mysock, (struct sockaddr *)&client_addr, &len);
        if (csock < 0)
        {
            printf("server: accept error\n"); exit(1);
        }
        printf("server: accepted a client connection from\n");
        printf("---------------------------------------------–\n");
        printf("Clinet: IP=%d port=%d\n",
        inet_ntoa(client_addr.sin_addr.s_addr),
        ntohs(client_addr.sin_port));
        printf("---------------------------------------------–\n");
        // Processing loop: client_sock <== data ==> client
        while(1)
        {
            testLine[0] = '\0';
            n = read(csock, line, MAX);
            if (n==0)
            {
                printf("server: client died, server loops\n");
                close(csock);
                break;
            }
            // show the line string
            printf("server: read n=%d bytes; line=%s\n", n, line);
            strcpy(testLine,line);
            char* partOne = strtok(testLine," ");
            char* partTwo = strtok(NULL, " ");
            printf("Part One: %s\nPart Two: %s\n",partOne,partTwo);
            if(checkForLocalCommand(partOne,partTwo))
            {
                continue;
            }
            // echo line to client
            n = write(csock, line, MAX);
            insertSpecialEndingChar();
            n = write(csock,outgoingBuffer,MAX);
            printf("server: wrote n=%d bytes; ECHO=%s\n", n, line);
            printf("server: ready for next request\n");
        }
    }
}