#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <ctype.h>
#include <dirent.h>
#include <fstream>

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         10000
#define MESSAGE             "This is the message I'm sending back and forth"
#define QUEUE_SIZE          5
#define MAX_MSG_SZ          10000

using namespace std;


// Determine if the character is whitespace
bool isWhitespace(char c)
{ switch (c)
    {
        case '\r':
        case '\n':
        case ' ':
        case '\0':
            return true;
        default:
            return false;
    }
}

// Strip off whitespace characters from the end of the line
void chomp(char *line)
{
    int len = strlen(line);
    while (isWhitespace(line[len]))
    {
        line[len--] = '\0';
    }
}

// Read the line one character at a time, looking for the CR
// You dont want to read too far, or you will mess up the content
char * GetLine(int fds)
{
    char tline[MAX_MSG_SZ];
    char *line;
    
    int messagesize = 0;
    int amtread = 0;
    while((amtread = read(fds, tline + messagesize, 1)) < MAX_MSG_SZ)
    {
        if (amtread >= 0)
            messagesize += amtread;
        else
        {
            perror("Socket Error is:");
            fprintf(stderr, "Read Failed on file descriptor %d messagesize = %d\n", fds, messagesize);
            exit(2);
        }
        //fprintf(stderr,"%d[%c]", messagesize,message[messagesize-1]);
        if (tline[messagesize - 1] == '\n')
            break;
    }
    tline[messagesize] = '\0';
    chomp(tline);
    line = (char *)malloc((strlen(tline) + 1) * sizeof(char));
    strcpy(line, tline);
    //fprintf(stderr, "GetLine: [%s]\n", line);
    return line;
}

// Change to upper case and replace with underlines for CGI scripts
void UpcaseAndReplaceDashWithUnderline(char *str)
{
    int i;
    char *s;
    
    s = str;
    for (i = 0; s[i] != ':'; i++)
    {
        if (s[i] >= 'a' && s[i] <= 'z')
            s[i] = 'A' + (s[i] - 'a');
        
        if (s[i] == '-')
            s[i] = '_';
    }
    
}


// When calling CGI scripts, you will have to convert header strings
// before inserting them into the environment.  This routine does most
// of the conversion
char *FormatHeader(char *str, const char *prefix)
{
    char *result = (char *)malloc(strlen(str) + strlen(prefix));
    char* value = strchr(str,':') + 1;
    UpcaseAndReplaceDashWithUnderline(str);
    *(strchr(str,':')) = '\0';
    sprintf(result, "%s%s=%s", prefix, str, value);
    return result;
}

// Get the header lines from a socket
//   envformat = true when getting a request from a web client
//   envformat = false when getting lines from a CGI program

void GetHeaderLines(vector<char *> &headerLines, int skt, bool envformat)
{
    // Read the headers, look for specific ones that may change our responseCode
    char *line;
    char *tline;
    
    tline = GetLine(skt);
    while(strlen(tline) != 0)
    {
        if (strstr(tline, "Content-Length") ||
            strstr(tline, "Content-Type"))
        {
            if (envformat)
                line = FormatHeader(tline, "");
            else
                line = strdup(tline);
        }
        else
        {
            if (envformat)
                line = FormatHeader(tline, "HTTP_");
            else
            {
                line = (char *)malloc((strlen(tline) + 10) * sizeof(char));
                sprintf(line, "%s", tline);
            }
        }
        //fprintf(stderr, "Header --> [%s]\n", line);
        
        headerLines.push_back(line);
        free(tline);
        tline = GetLine(skt);
    }
    free(tline);
}

int determineType(string arg) {
    int i = arg.size() - 1;
    stringstream ss;
    while (arg[i] != '.') {
        i--;
    }
    string extention = arg.substr(i);
    if (extention == ".html") {
        return 1;
    } else if (extention == ".jpg") {
        return 2;
    } else if (extention == ".gif") {
        return 3;
    } else {
        return 0;
    }
}

void fileDir(int hSocket, string arg) {
    struct stat filestat;
    string header;
    string body;
    stringstream ss;
    stringstream body_ss;
    
    if(stat(arg.c_str(), &filestat)) {
        cout <<"ERROR in stat\n";
        header = "HTTP/1.0 404 Not Found\r\n\r\n";
        write(hSocket, header.c_str(), strlen(header.c_str())+1);
        return;
    }
    if(S_ISREG(filestat.st_mode)) {
        int typeCode = determineType(arg);
        ss << "HTTP/1.0 200 OK\r\n";
        switch (typeCode) {
            case 0:
                ss << "Content-Type: text/plain\r\n";
                break;
            case 1:
                ss << "Content-Type: text/html\r\n";
                break;
            case 2:
                ss << "Content-Length: " << filestat.st_size << "\r\n";
                ss << "Content-Type: image/jpeg\r\n";
                break;
            case 3:
                ss << "Content-Length: " << filestat.st_size << "\r\n";
                ss << "Content-Type: image/gif\r\n";
                break;
        }
        ss << "\r\n";
        header = ss.str();
        cout << endl << header << endl;
        write(hSocket, header.c_str(), strlen(header.c_str())+1);
        
        if (typeCode == 2 || typeCode == 3) {
            FILE *fp = fopen(arg.c_str(),"rb");
            char *buffer = (char *)malloc(filestat.st_size+1);
            fread(buffer, filestat.st_size, 1,fp);
            write(hSocket,buffer,filestat.st_size);
            free(buffer);
            fclose(fp);
        } else {
            FILE *fp = fopen(arg.c_str(),"r");
            char *buffer = (char *)malloc(filestat.st_size+1);
            fread(buffer, filestat.st_size, 1, fp);
            write(hSocket, buffer, filestat.st_size);
            free(buffer);
            fclose(fp);
        }
    }
    if(S_ISDIR(filestat.st_mode)) {
        header = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";
        write(hSocket, header.c_str(), strlen(header.c_str())+1);
        DIR *dirp;
        struct dirent *dp;
        
        dirp = opendir(arg.c_str());
        string directoryListing;
        stringstream ss;
        
        ss << "<html>\n<body>\n<ul>\n";
        
        bool hasIndex = false;
        stringstream name;
        while ((dp = readdir(dirp)) != NULL) {
            ss << "<li>" << dp->d_name << "</li>\n";
            name << dp->d_name;
            printf("name %s\n", dp->d_name);
            if (name.str() == "index.html") {
                hasIndex = true;
            }
            name.str("");
        }
        ss << "</ul>\n</body>\n</html>";
        directoryListing = ss.str();
        cout << directoryListing << "\n\n";
        if (hasIndex) {
            string newPath = arg + "/index.html";
            FILE *fp = fopen(newPath.c_str(),"r");
            char *buffer = (char *)malloc(filestat.st_size+1);
            fread(buffer, filestat.st_size, 1, fp);
            write(hSocket, buffer, filestat.st_size);
            free(buffer);
            fclose(fp);
        } else {
            write(hSocket, directoryListing.c_str(), strlen(directoryListing.c_str())+1);
        }
        (void)closedir(dirp);
    }
}

void readWrite(int hSocket, char pBuffer[], char* arg) {
    vector<char *> headerLines;
    char buffer[MAX_MSG_SZ];
    char contentType[MAX_MSG_SZ];
    
    // Read the header lines
    GetHeaderLines(headerLines, hSocket , false);
    
    // Now print them out
    for (int i = 0; i < headerLines.size(); i++) {
        printf("%s\n",headerLines[i]);
    }
    
    stringstream ss;
    ss << arg;
    for (int i = 4; !isspace(headerLines[0][i]); i++) {
        ss << headerLines[0][i];
    }
    fileDir(hSocket, ss.str());
}

int main(int argc, char* argv[])
{
    int hSocket,hServerSocket;  /* handle to socket */
    struct hostent* pHostInfo;   /* holds info about a machine */
    struct sockaddr_in Address; /* Internet socket address stuct */
    int nAddressSize=sizeof(struct sockaddr_in);
    char pBuffer[BUFFER_SIZE];
    int nHostPort;

    if(argc < 3)
      {
        printf("\nUsage: server host-port dir\n");
        return 0;
      }
    else
      {
        nHostPort=atoi(argv[1]);
      }
    string path = argv[2];

    printf("\nStarting server");

    printf("\nMaking socket");
    /* make a socket */
    hServerSocket=socket(AF_INET,SOCK_STREAM,0);

    if(hServerSocket == SOCKET_ERROR)
    {
        printf("\nCould not make a socket\n");
        return 0;
    }

    /* fill address struct */
    Address.sin_addr.s_addr=INADDR_ANY;
    Address.sin_port=htons(nHostPort);
    Address.sin_family=AF_INET;

    printf("\nBinding to port %d\n",nHostPort);

    /* bind to a port */
    if(bind(hServerSocket,(struct sockaddr*)&Address,sizeof(Address)) 
                        == SOCKET_ERROR)
    {
        printf("\nCould not connect to host\n");
        return 0;
    }
 /*  get port number */
    getsockname( hServerSocket, (struct sockaddr *) &Address,(socklen_t *)&nAddressSize);
    printf("opened socket as fd (%d) on port (%d) for stream i/o\n",hServerSocket, ntohs(Address.sin_port) );

    printf("\nMaking a listen queue of %d elements\n",QUEUE_SIZE);
    /* establish listen queue */
    if(listen(hServerSocket,QUEUE_SIZE) == SOCKET_ERROR)
    {
        printf("\nCould not listen\n");
        return 0;
    }

    int optval = 1;
    setsockopt (hServerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    for(;;)
    {
        printf("\nWaiting for a connection\n");
        /* get the connected socket */
        hSocket=accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);

        printf("\nGot a connection from %X (%d)\n",
              Address.sin_addr.s_addr,
              ntohs(Address.sin_port));
        
        readWrite(hSocket, pBuffer, argv[2]);
        
        linger lin;
        unsigned int y=sizeof(lin);
        lin.l_onoff=1;
        lin.l_linger=10;
        setsockopt(hSocket,SOL_SOCKET, SO_LINGER,&lin,sizeof(lin));
        
        shutdown(hSocket, SHUT_RDWR);

        printf("\nClosing the socket");
        /* close socket */
        if(close(hSocket) == SOCKET_ERROR)
        {
         printf("\nCould not close socket\n");
         return 0;
        }
    }
}
