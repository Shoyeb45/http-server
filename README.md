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

