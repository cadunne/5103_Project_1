# 5103 Project 1
### OS Support for Concurrency
#### Connor Dunne, Karel Kalthoff, Jane Kagan

## About

This is a server implemented three different ways:
1. With threads handling each request
2. A single-threaded server polling sockets for each connection using
  a. aio_read
  b. blocking read
3. Using select to check a socket for each connection in turn


## How to Use

Run with `./server <implementation> <port>`

Implementation argument can be
* thread

* polling_aio

* polling_read

* select
