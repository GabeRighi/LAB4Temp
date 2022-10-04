/******** C13.2.b: TCP client.c file TCP ********/
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
#define SERVER_PORT 1234
struct sockaddr_in server_addr;
int sock, r;
struct stat mystat, *sp;
char *t1 = "xwrxwrxwr ";
char *t2 = " ";

int lsfile(char *fname)
{
    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ( (r = lstat(fname, &fstat)) < 0)
    {
        printf("can't stat %s\n", fname);
        printf("ERRNO: %d\n",errno);
        exit(1);
    }
    if ((sp->st_mode & 0xF000) == 0x8000) // if (S ISREG())
        printf("%c",'-');
    if ((sp->st_mode & 0xF000) == 0x4000) // if (S ISDIR())
        printf("%c",'d');
    if ((sp->st_mode & 0xF000) == 0xA000) // if (S ISLNK())
        printf("%c",'l');
    for (i=8; i >= 0; --i )
    {
        if (sp->st_mode & (1 << i)) // print r|w|x
            printf("%c", t1[i]);
        else
            printf("%c", t2[i]); // or print
    }
    printf("%4d ",sp->st_nlink); // link count
    printf("%4d ",sp->st_gid); // gid
    printf("%4d ",sp->st_uid); // uid
    printf("%8d ",sp->st_size); // file size
    // print time
    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
    ftime[strlen(ftime)-1] = 0; // kill \n at end
    printf("%s ",ftime);
    // print name
    printf("%s", basename(fname)); // print file basename
    // print > linkname if symbolic file
    if ((sp->st_mode & 0xF000)== 0xA000)
    {
    // use readlink() to read linkname
        char linkname[256];
        readlink(fname,linkname,256);
        printf("-> %s", linkname); // print linked name
    }
    printf("\n");
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
 
    if(!strcmp("lcat",command))
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
    else if(!strcmp("lcd",command))
    {
        if(!chdir(passedPath))
        {
            printf("Changed DIR\n");
        }
        else
        {
            printf("Error changing DIR\n");
        }
        return 1;
    }
    else if (!strcmp("lpwd", command))
    {
        char buf[256];
        if(getcwd(buf,256))
        {
            printf("CWD: %s\n", buf);
        }
        else
        {
            printf("Error getting CWD\n");
        }
        return 1;
    }
    else if(!strcmp("lmkdir",command))
    {
        if(!mkdir(passedPath, 0755))
        {
            printf("File created!\n");
        }
        else
        {
            printf("Erorr creating file\n");
        }
        return 1;
    }
    else if(!strcmp("lrmdir",command))
    {
        if(!rmdir(passedPath))
        {
            printf("DIR deleted\n");
        }
        else
        {
            printf("Error deleting file\n");
        }
        return 1;
    }
    else if(!strcmp("lrm",command))
    {
        if(!unlink(passedPath))
        {
            printf("File deleted\n");
        }
        else
        {
            printf("Error deleting file\n");
        }
        return 1;
    }
    else if(!strcmp("lls",command))
    {
        struct stat mystat, *sp = &mystat;
        int r;
        char *filename, path[1024], cwd[256];
        filename = "./"; // default to CWD
        printf("P:%s\n",passedPath);
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

        return 1;
    }
    else
    {
        return 0;
    }
}
int client_init()
{

printf("======= clinet init ==========\n");
printf("1 : create a TCP socket\n");

sock = socket(AF_INET, SOCK_STREAM, 0);
//<Test
if (sock<0)
{
    printf("socket call failed\n"); exit(1);
}
printf("2 : fill server_addr with server’s IP and PORT#\n");

server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // localhost
server_addr.sin_port = htons(SERVER_PORT); // server port number

printf("3 : connecting to server ....\n");

r = connect(sock,(struct sockaddr*)&server_addr, sizeof(server_addr));
if (r < 0)
{
    printf("connect failed\n"); exit(3);
}
printf("4 : connected OK to\n");
printf("-------------------------------------------------------\n");
printf("Server hostname=%s PORT=%d\n", SERVER_HOST, SERVER_PORT);
printf("-------------------------------------------------------\n");
printf("========= init done ==========\n");

}

int main()
{

int n;
char line[MAX], ans[MAX];
client_init();
printf("******** processing loop *********\n");
while (1)
{
    printf("input a line : ");
    bzero(line, MAX); // zero out line[ ]
    fgets(line, MAX, stdin); // get a line from stdin
    line[strlen(line)-1] = 0; // kill \n at end
    if (line[0]==0) // exit if NULL line
        exit(0);
    char* firstPart = strtok(line," ");
    char* secondPart = strtok(NULL," ");
    printf("FIRST:[%s] | SECOND:[%s]\n",firstPart,secondPart);
    if(checkForLocalCommand(firstPart,secondPart))
    {
        continue;
    }
    // Send line to server
    n = write(sock, line, MAX);
    printf("client: wrote n=%d bytes; line=%s\n", n, line);
    // Read a line from sock and show it
    n = read(sock, ans, MAX);
    printf("client: read n=%d bytes; echo=%s\n", n, ans);
}
}