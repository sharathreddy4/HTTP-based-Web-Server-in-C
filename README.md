# HTTP-based-Web-Server
This repo is an assignment to create a HTTP based web server that handles various requests from users. This is done as part of Network Systems course at CU boulder.

## Working:
The web server sourcecode(finalshs.c) consists of different functions 
to complete the different jobs of a web server.
The functions are:

1.The void scan function is use to read the config file.

2.The parse_config_file function is to parse the config file(ws.conf)
  and read the set of parameters like default page, port number, extensions, documentary root

3.The get_content_type function is used to extract the extension based on the content.

4.The send_headers function is to send the status,title,content-length, content type
  of the responses to a web browser, as a consequence to its requests.

5.The send_error function is to send the error display messages to the browser
  such as error 404,400,500.

6.The send_file is used to send the required file to the browser(client)
  based on its request.

7.The process function is used to process the request sent 
  by the client to the server and send back the appropriate response.

8.The main function is used to create a socket and establish a connection with the client, 
  and also terminate the connection after communication is done. The server handles multiple connections
  using fork method here.Pipelining is supported.

## Steps to run the program:

1.Compile and execute the source code, after which a message is displayed that the  
  server is open on a particular port number specified in the config file.

2.Open a web browser and type the url.

3.The required index page is displayed from which other image files can be opened.

4.To check the working of error 400:
  in url, type - localhost:PORT/ERR400.html
  A html page with a form will open and when we try to submit the data 
  using POST , the server gives us 400 error.

5.To check the working of error 500:
  Change the file permissions of secure.txt from read to write only and then
  in url type- localhost:PORT/secure.txt
  The server then gives us 500 error.
