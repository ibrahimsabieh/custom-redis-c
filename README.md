# custom-redis-c

A custom Redis server built in C.

Uses an Event Loop to handle multiple concurrent client and multiple commands from clients (using Select(), working on update to switch to libuv).

Binds to defult Redis port 6379

## How to Run

Simply download compiled C file from "New Releases" and run it.

Upon running the server, it will wait for incomming connections to the set port (default 6379)

### 1. Using Netcat

You can use netcat (nc for mac) to send commands by first connecting to the server.

'''
nc localhost 6379
PING
'''
output:
'''
PONG
'''

### 2. Using Redis-cli

You can also run the server using the offical Redis-cli to send commands.

First, you must download the redis-cli. Here is a link on how to do so:

[How to download redis](https://redis.io/docs/latest/operate/oss_and_stack/install/install-redis/)

This shows you how to download redis as a whole including redis-server, you may only want to download the redis-cli.
In that case, here are links on how to do so:

[(Linux) - How to Download redis-cli](https://www.baeldung.com/linux/redis-client-alone-installation)

[(Mac) - How to Download redis-cli](https://medium.com/nioya/how-to-install-redis-cli-on-mac-os-x-via-homebrew-without-redis-installation-46d9178ceaa8)

After redis-cli is installed and you have the server running you can simply send commands using the redis-cli.

```
redis-cli PING
```
output:
```
PONG
```

## Building from Source

Building from source is simple. Just clone repo and run gcc on server.c

```
git clone https://github.com/ibrahimsabieh/custom-redis-c.git
cd custom-redis-c
gcc -o server server.c
```

This should result in an executable file. Running it will start up the server.
