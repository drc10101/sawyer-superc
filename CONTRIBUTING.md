# Sawyer SuperC

Native MoE inference. Single binary. Zero deps.

The C-powered upgrade to [Sawyer](https://github.com/drc10101/sawyer-network). Same network, same token economics, native speed.

Engine based on [Colibri](https://github.com/JustVugg/colibri) by Vincenzo/JustVugg (Apache 2.0).

## Quick Start

```bash
make
./superc chat
```

## Build

Requires gcc or clang with OpenMP support.

```bash
make              # Build the binary
make test         # Run self-tests
make install      # Install to /usr/local/bin
```

## License

Apache 2.0. See [LICENSE](LICENSE) and [LICENSE-COLIBRI](LICENSE-COLIBRI).