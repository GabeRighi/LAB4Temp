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
char bigLine[MAX];

void writeToClient()
{
    n = write(csock, bigLine, MAX);
    printf("server: wrote n=%d bytes; ECHO=%s\n", n, bigLine);
    printf("server: ready for next request\n");
}

void specialEndBig()
{
    bigLine[0] = '\x04';
    bigLine[1] = '\0';
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
        strcat(bigLine,"Error lsing file");
        return -1;
    }
    if ((sp->st_mode & 0xF000) == 0x8000) // if (S ISREG())
        bigLine[position++] = '-';
    if ((sp->st_mode & 0xF000) == 0x4000) // if (S ISDIR())
        bigLine[position++] = 'd';
    if ((sp->st_mode & 0xF000) == 0xA000) // if (S ISLNK())
        bigLine[position++] = 'l';
    for (i=8; i >= 0; --i )
    {
        if (sp->st_mode & (1 << i)) // print r|w|x
        {
            bigLine[position++] = t1[i];
        }
        else
        {
            bigLine[position++] = t2[i];
        }
    }
    char format[10];
    format[0] = '\0';
    bigLine[position] = '\0';
    // printf("%4d ",sp->st_nlink); // link count
    sprintf(format,"%4d ",sp->st_nlink);
    strcat(bigLine,format);
    // printf("%4d ",sp->st_gid); // gid
    sprintf(format, "%4d ",sp->st_gid);
    strcat(bigLine,format);
    // printf("%4d ",sp->st_uid); // uid
    sprintf(format,"%4d ",sp->st_uid);
    strcat(bigLine,format);
    // printf("%8d ",sp->st_size); // file size
    sprintf(format,"%8d ",sp->st_size);
    strcat(bigLine,format);

    // print time
    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
    ftime[strlen(ftime)-1] = 0; // kill \n at end
    // printf("%s ",ftime);
    strcat(bigLine,ftime);
    // print name
    // printf("%s", basename(fname)); // print file basename
    strcat(bigLine, " ");
    strcat(bigLine,basename(fname));
    // print > linkname if symbolic file
    if ((sp->st_mode & 0xF000)== 0xA000)
    {
    // use readlink() to read linkname
        strcat(bigLine,"-> ");
        char linkname[256];
        readlink(fname,linkname,256);
        // printf("-> %s", linkname); // print linked name
        strcat(bigLine,linkname);
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
    bigLine[0] = '\0';
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
            strcpy(bigLine,"Dir Changed!");
            // writeToClient(bigLine);
        }
        else
        {
            strcpy(bigLine,"Error Changing Dir!");
            // printf("Error changing DIR\n");
        }
        writeToClient();
        specialEndBig();
        writeToClient();
        return 1;
    }
    else if (!strcmp("pwd", command))
    {
        char buf[256];
        if(getcwd(buf,256))
        {
            strcpy(bigLine,buf);
        }
        else
        {
            strcpy(bigLine,"Error getting CWD!");
        }
        writeToClient();
        specialEndBig();
        writeToClient();
        return 1;
    }
    else if(!strcmp("mkdir",command))
    {
        if(!mkdir(passedPath, 0755))
        {
            strcpy(bigLine,"Dir Created!");
        }
        else
        {
            strcpy(bigLine,"Error Creating Dir.");
        }
        writeToClient();
        specialEndBig();
        writeToClient();
        return 1;
    }
    else if(!strcmp("rmdir",command))
    {
        if(!rmdir(passedPath))
        {
            strcpy(bigLine,"Dir Deleted!");
        }
        else
        {
            strcpy(bigLine,"Could not delete dir!");
        }
        writeToClient();
        specialEndBig();
        writeToClient();
        return 1;
    }
    else if(!strcmp("rm",command))
    {
        if(!unlink(passedPath))
        {
            strcpy(bigLine, "Deleted File!");
        }
        else
        {
            strcpy(bigLine,"Could not delete.");
        }
        writeToClient();
        specialEndBig();
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

        specialEndBig();
        writeToClient();

        return 1;
    }
    else if(!strcmp("get",command))
    {
        struct stat fstat, *sp;
        FILE* fp;
        size_t len = MAX;
        bigLine[0] = '\0';
        char* bigLinePtr = bigLine;
        // passedPath[strlen(passedPath)-1] = '\0';
        if ( (r = lstat(passedPath, &fstat)) < 0)
        {
            printf("COULD NO LSTAT");
            bigLine[0] = '\0';
            writeToClient();
            // strcat(bigLine,"Error lsing file");
            return -1;
        }
        fp = fopen(passedPath,"r");
        printf("passed path: %s\n",passedPath);
        if(fp == NULL)
        {
            printf("FP NULL\n");
            bigLine[0] = '\0';
            writeToClient();
            return -1;
        }
        sprintf(bigLine,"%u",(unsigned)sp->st_size);
        writeToClient();
        while(getline(&bigLinePtr,&len,fp)!= -1)
        {
            writeToClient();
        }
        fclose(fp);

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
            specialEndBig();
            n = write(csock,bigLine,MAX);
            printf("server: wrote n=%d bytes; ECHO=%s\n", n, line);
            printf("server: ready for next request\n");
        }
    }
}