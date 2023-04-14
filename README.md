# hypertext-toy

Thanks to [@therackshack](https://github.com/therackshack) for coming up with an actual name for this.

This is a toy HTTP server written in C using sockets. It isn't fit for any type of production use-case, although it may be somewhat useful as a teaching tool. It is mostly here for me to use to learn about sockets, networking, and HTTP. However, it does exist, and it will serve webpages, so if you need to set up a quick and dirty static HTTP server, this may be useful.

The server currently runs on port 8000 and listens to all addresses by default.

SSL support is currently not planned, but it might be implemented if I either get bored or someone makes a pull request for it.

## Building

You need a POSIX-compliant system in order to run this program.

To build and run, just run
```sh
$ make
$ ./http-server
```

## Generating internal documentation

If you have Doxygen installed, you can easily generate and read internal documentation in many formats! By default, HTML and LaTeX files are generated. It is available in the repositories of many Linux distributions.

To generate documentation, just run
```sh
$ doxygen
```
and you will find documentation present in a subdirectory labeled docs!

## Things currently on the todo list
- [x] Parse % encoding in URIs
- [x] Add MIME types
- [x] Slightly nicer status pages
- [ ] Configuration options (config file and CLI arg overrides)
- [x] Better program structure
