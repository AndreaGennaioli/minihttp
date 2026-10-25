# minihttp

`minihttp` is a simple HTTP server written in C, built as a learning project to explore TCP sockets and POSIX.

Currently the focus of this project is to create an HTTP server which is compliant with the HTTP/1.0 specification [RFC 1945](https://www.rfc-editor.org/info/rfc1945/).

## Build and run

Requirements: Linux x86-64, gcc, make.

```bash
make
# usage: mhttp PORT
./mhttp 5000
# clean
make clean
```

## Roadmap

This is a vague roadmap for the project:

- [x] [HTTP/0.9](https://www.w3.org/Protocols/HTTP/AsImplemented.html) server that returns a static response
- [ ] Request URI parsing and static file serving
- [ ] HTTP/1.0 request line, headers and status line
- [ ] `GET`, `HEAD` and `POST` methods
- [ ] Conditional `GET`
- [ ] Full HTTP/1.0 compliance
