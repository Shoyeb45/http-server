# HTTP Server

- HTTP protocol is one of the most important techonology which allows the communication over network.
- This was one of the challenge of codecrafters and I really enjoyed building it and I will make it like a mini express soon.


# Run the server 


1. Ensure you have `cmake` installed locally, also make sure to configure `vcpkg`
1. Run `./your_program.sh` to run your program, which is implemented in
   `src/main.cpp`.(first make it executable`chmod +x your_program.sh`)
2. Use curl to make request.
```bash
curl -i http://localhost:4221/
```


# Current features

1. Concurrent Connection
2. Persisten Connection, if sent `Connection: close`, then it will shut the connection.
3. Allowed compression using `gzip` algorithm(implemented using `zlib`)
4. Route you can hit and get desired output:

   1. `/echo/{str}`
   - Send desired string in params, and you will get output like:
   ```bash
   curl -i http://localhost:4221/echo/http-protocol

   Output:
   HTTP/1.1 200 OK
   Content-Type: text/plain
   Content-Length: 13

   http-protocol
   ```

   2. `/user-agent`
   - It will return User agent(The client used to send a request)
   ```bash
   curl -i http://localhost:4221/user-agent

   Ouput:
   HTTP/1.1 200 OK
   Content-Type: text/plain
   Content-Length: 10

   curl/8.5.0
   ```

   3. GET `/files/{file_path}`
   - Read from the file in desired location. You need to give the location in command line argument. (`./your-program.sh --directory /tmp/`)
   - It will read from the given file path if the file exits, else it will return with `404`.
   ```bash
   1. File exists
      curl -i http://localhost:4221/files/hello.txt

      HTTP/1.1 200 OK
      Content-Type: application/octet-stream
      Content-Length: 12

      Hello, World

   2. File does not exist

   curl -i http://localhost:4221/files/world.txt
   
   HTTP/1.1 404 Not Found

   ```

   4. POST `/files/{path}`, Body : {file content}
   - It will write to file with given `path`.

   5. GET `/`
   - It will return `200 Ok`.

# Files

1. [main](./src/main.cpp)
- Initialises a TCP connection and attach to the port
- Using multithreading we can connect to different client and handle each of them separatley while keeping the connection persistent.

2. [Utils](./src/utils/utils.cpp)
- Utitlity function to parse and format the strings.