#include<stdlib.h>
#include<unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include<stdbool.h>

#define PROTOCOL  "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

int PORT;
char rootpath[256];
char index1[20];
char index2[20];
char index3[20];
char html[20];
char png[20];
char text[20];
char gif[20];
char key[256];
char value[256];
bool keepalive=false;
void scan(char* input)
{
	//based on the key value, their corresponding values are parsed and initialized
	
	sscanf(input,"%s %s",key,value);
	if(strcmp(key,"Listen") == 0){
		sscanf(value,"%d",&PORT);//port number is extracted.
	}
	else if(strcmp(key,"DocumentRoot") == 0){
		sscanf(value,"\"%s\"",rootpath);
		rootpath[strlen(rootpath)-1] = '/';//directory root path is initialized
	}
	else if(strcmp(key,"DirectoryIndex") == 0){
		sscanf(input,"%s %s %s %s",key,index1,index2,index3);
	}
	else if(strcmp(key,".html") == 0){
		sscanf(value,"%s",html);//all the extensions are extracted.
	}
	else if(strcmp(key,".txt") == 0){
		sscanf(value,"%s",text);
	}
	else if(strcmp(key,".png") == 0){
		sscanf(value,"%s",png);;
	}
	else if(strcmp(key,".gif") == 0){
		sscanf(value,"%s",gif);
	}
}
void parse_config_file()
{	
    static const char filename[] = "ws.conf";
   FILE *file = fopen ( filename, "r" );
   if ( file != NULL )
   {
      char line [ 256 ]; 
	  
      while ( fgets ( line, sizeof(line), file ) != NULL ) 
      {
         if(line[0]!='#' && line[0]!= '\n')//send the non commenting lines to scanning
         {
         	scan(line);
	}
     	
      }
      fclose ( file );
   }
   else
   {
      perror ( filename ); /* why didn't the file open? */
   }
}

char *get_content_type(char *name) {
	//used to extract the extension based on the content.
  char *extension = strrchr(name, '.');
  if (!extension) return NULL;
  if (strcmp(extension, ".html") == 0 || strcmp(extension, ".htm") == 0) 
		return html;
  if (strcmp(extension, ".gif") == 0) 
		return gif;
  if (strcmp(extension, ".png") == 0) 
		return png;
  if (strcmp(extension, ".txt") == 0) 
		return text;
  
  return NULL;
}

void send_headers(FILE *f, int status, char *title, char *extra, char *type, 
                  int length, time_t date) {
  time_t now;
  char timebuf[128];

  fprintf(f, "%s %d %s\r\n", PROTOCOL, status, title);//protocol data and status including title.
  
  now = time(NULL);
  strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
  fprintf(f, "Date: %s\r\n", timebuf);//data field.
  if (extra) 
		fprintf(f, "%s\r\n", extra);
  if (length >= 0) 
		fprintf(f, "Content-Length: %d\r\n", length);//length of the content.
  if(keepalive)
		fprintf(f, "Connection: Keep-Alive\r\n");
  if (type) 
		fprintf(f, "Content-Type: %s\r\n", type);//mime type
  fprintf(f, "\r\n");
}

void send_error(FILE *f, int status, char *title, char *extra, char *text) {
	//corresponding header is formed.
  send_headers(f, status, title, extra, "text/html", -1, -1);
  fprintf(f, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n", status, title);//title is set to match the error.
  fprintf(f, "<BODY><H4>%d %s</H4>\r\n", status, title);//description is written in the body.
  fprintf(f, "%s\r\n", text);
  fprintf(f, "</BODY></HTML>\r\n");//a complete html is rendered to show the error description
}

void send_file(FILE *f, char *path, struct stat *statbuf) {
  char data[4096];
  int n;
//file is opened in read mode.
  FILE *file = fopen(path, "r");
  if (!file) {
	  //if access permissions are violated 500 error is returned.
    send_error(f, 500, "Internal Server Error", NULL, "Cannot allocate memory");
  } else {
	  //checking for the file to be regular
    int length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
    send_headers(f, 200, "OK", NULL, get_content_type(path), length, statbuf->st_mtime);
	//data is written to file
    while ((n = fread(data, 1, sizeof(data), file)) > 0) 
			fwrite(data, 1, n, f);
		//stream is closed.
    fclose(file);
  }
}

int process(FILE *f) {
  char buf[4096];
  char *method;
  char path[1024];
  char *temp_path;
  char temp_path_1[1024];
  char *protocol;
  struct stat statbuf;
  char pathbuf[4096];
  int len;
  char temp[4096];
  //buf contains the data sent from client

  if (!fgets(buf, sizeof(buf), f)) return -1;
  printf("\nURL: %s\n", buf);
  //method, path,protocol contains the named values respectively.
  method = strtok(buf, " ");
  temp_path = strtok(NULL," ");
  strcpy(temp_path_1,rootpath);
  temp_path_1[strlen(temp_path_1)-1] = '\0';
  
  strcat(temp_path_1,temp_path);
  strcat(path,temp_path);

  int i = stat(temp_path_1, &statbuf);

  protocol = strtok(NULL, "\r");
  
  fgets(temp,sizeof(temp),f);
  fgets(temp,sizeof(temp),f);
  //temp contains the keep-alive attribute
   

if(strcmp(temp,"Connection: keep-alive")==0){
	printf("%s",temp);
	keepalive=true;
}


  if (!method || !path || !protocol) return -1;

  fseek(f, 0, SEEK_CUR); // Force change of stream direction

  if (strcasecmp(method, "GET") != 0) {
    send_error(f, 400, "Bad Request", NULL, "Invalid Method");
  }
  else if(strcasecmp(protocol,"HTTP/1.1")!=0){
   send_error(f, 400, "Bad Request", NULL, "Invalid Http Version");
  } 
  //file not found
   else  if ( i < 0) {
    	send_error(f, 404, "Not Found", NULL, "File not found.");
  } 
		//possiblity of it being a directory
    	else if (S_ISDIR(statbuf.st_mode)) {
    		len = strlen(path);
		
		
			//possibility of having no / at the end
    		if (len == 0 || path[len - 1] != '/') {
      			snprintf(pathbuf, sizeof(pathbuf), "Location: %s/",path);
		        send_error(f, 302, "Found", pathbuf, "Directories must end with a slash.");
    		} 
			//case to handle index.html or 	index.htm or index.ws from the current directory
    		else {
	      		snprintf(pathbuf, sizeof(pathbuf), "%s%s%s", rootpath,path,index1);
			printf("PATH: %s\n",pathbuf);
      			if (stat(pathbuf, &statbuf) >= 0) {
        			send_file(f, pathbuf, &statbuf);
      			}
				else{
				snprintf(pathbuf, sizeof(pathbuf), "%s%s%s", rootpath,path,index2);
      				if (stat(pathbuf, &statbuf) >= 0) {
        				send_file(f, pathbuf, &statbuf);
      				}
				else{
					snprintf(pathbuf, sizeof(pathbuf), "%s%s%s",rootpath, path,index3);
      					if (stat(pathbuf, &statbuf) >= 0) {
        					send_file(f, pathbuf, &statbuf);
      					}
						//if not present, the index.html or index.htm or index.ws from the home directory are sent.
					else {        
						snprintf(pathbuf, sizeof(pathbuf), "%s%s", rootpath,index1);
      						if (stat(pathbuf, &statbuf) >= 0) {
        						send_file(f, pathbuf, &statbuf);
      						}
						else{
							snprintf(pathbuf, sizeof(pathbuf), "%s%s", rootpath,index2);
      							if (stat(pathbuf, &statbuf) >= 0) {
        						send_file(f, pathbuf, &statbuf);
      							}
							else{
								snprintf(pathbuf, sizeof(pathbuf), "%s%s", rootpath,index3);
      								if (stat(pathbuf, &statbuf) >= 0) {
        							send_file(f, pathbuf, &statbuf);
      							}
      						}
					}
				}
			 
    			}
  	}
  }
}

//or else the original file is sent.

	else {
		snprintf(pathbuf, sizeof(pathbuf), "%s%s", rootpath,path);
    		send_file(f, pathbuf, &statbuf);
  	}

  return 0;
}

int main(int argc, char *argv[]) {
  int sock;//socket file descriptor
  struct sockaddr_in sin;
  parse_config_file();//it contains logic to parse the conf file and initialize the parameters

  sock = socket(AF_INET, SOCK_STREAM, 0);//stream and network based socket
	//initializing the structure parameters
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(PORT);
  //binding the socket to the address
  bind(sock, (struct sockaddr *) &sin, sizeof(sin));
 //server socket listens at the specified PORT number over the network
  listen(sock, 5);
  printf("HTTP server listening on port %d\n", PORT);

  while (1) {
    int new_sock_for_child;//socket file descriptor for the child process
    FILE *f;

    new_sock_for_child = accept(sock, NULL, NULL);//connection accepted from client
    if (new_sock_for_child < 0) break;
     if(fork() == 0){//only child process will execute this block of code making the parent process go back to accept block function
	close(sock);
    	f = fdopen(new_sock_for_child, "a+");
		//the client data is sent for processing
    	process(f);
	//if connection keep alive is mentioned, this boolean value is activates
	if(keepalive)
		sleep(10);
	//after a span of 10 sec connection is closed.
	fclose(f);
	close(new_sock_for_child);
	}
	//connection is immediately closed by parent and returned back to accept functio
	close(new_sock_for_child);
  }
  return 0;
}

