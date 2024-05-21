# Charcoal
A stress test tool for Minecraft servers. Tested on versions 1.8.X to 1.18.X.

This tool was created to experiment with C and sockets, so it may not be the best (very buggy), yet it _seems_ to work.

## Screenshots
<img src="https://i.imgur.com/4bkapS3.png" width="400">

<img src="https://i.imgur.com/wcEStE7.png" width="400">

## Usage
Compile the code first
```
gcc main.c -o charcoal
```

&nbsp;

```
./charcoal -i address [-p port] [-t thread number] [-m mode] [-c message] [-v protocol version] [-x proxy file] [-k proxy type]
```
`-i address`: Specify the server address

`-p port`: (_optional_, _default_=25565) Server port

`-t thread number`: (_optional_, _default_=10) Number of threads to use

`-m mode`: (_optional_, _default_=flood) Specify the bot mode (*flood* or *stay*)

`-c message`: (_optional_) Set the message that bots should send

`-v protocol version`: (_optional_, _default_=47/1.8.X): The server version (you can use either the protocol version or version number)

`-x proxy file`: (_optional_) Proxy file

`-k proxy type`: (_optional_, _default_=https) Proxy type (https or socks5)

## Disclaimer
I am not affiliated with Minecraft or Microsoft in any way. Please use Charcoal responsibly and only for authorized purposes.