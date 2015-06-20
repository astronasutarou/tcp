# A simple TCP/IP connection in C++
This repository provides a simple way to use TCP/IP connection in C++.

This contains two classes: `tcp::server` and `tcp::client`, which work as a server and a client in TCP/IP connection, respectively.

## How to Use
Copy `tcp.h` in your local directory and just include it. You don't need to link any library. All the classes are defined in the namespace of `tcp`.

There are two sample codes to operate `tcp::server` and `tcp::clinet`, respectively.
- `test_server.cc`
- `test_client.cc`

## Examples
Following is a simple usage of `tcp::server`.
~~~c++
// create a server instance.
tcp::server s(8081);
// or create a server instance which accept a connection only from 127.0.0.1.
tcp::server s(8081, "127.0.0.1");

// start listening
s.listen();

while (1) {
  // accept a connection from a client
  s.accept();
  // read 80 bytes from the client
  s.read(read_buf, 80);
  // send 80 bytes in data_buf to the client
  s.write(data_buf, 80);
}
~~~

Following is a simple usage of `tcp::client`.
~~~c++
// create a client instance which connects to 127.0.0.1
tcp::client c(8081, "127.0.0.1");

// make a connection to a server of 127.0.0.1:8081
c.connect();

// send 80 bytes in data_buf to the server
c.write(data_buf, 80);
// read 80 bytes from the server
c.read(read_buf, 80);
~~~

## License
This is licensed under GPLv2.
