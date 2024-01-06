# traceroute

An implementation of [traceroute](https://en.wikipedia.org/wiki/Traceroute), a
program which exceeds the IP TTL at consecutive hops through the network and
extracts information about each hop from the ICMP response.

### Example

Traceroute to the IP of [example.com](https://example.com). Some lines are
redacted or omitted.

```sh
$ sudo ./traceroute 93.184.216.34
traceroute to 93.184.216.34 (64 hops max)
Hop   IP                                                           Time (ms)
---   --                                                           ---------
1     192.168.1.1 (REDACTED)                                       1.277588
2     REDACTED (REDACTED)                                          106.336548
3     152.195.68.131 (ae-65.core1.nyb.edgecastcdn.net)             56.671997
...   ...                                                          ...
6     152.195.68.131 (ae-65.core1.nyb.edgecastcdn.net)             252.158569
7     93.184.216.34                                                131.640991
```

### Build

This has currently only been tested on macOS. Requires `gcc` and makes use of
the Berkeley sockets API.

```sh
$ ./build.sh
```

### Resources
- [traceroute(8)](https://linux.die.net/man/8/traceroute)
- [Apple traceroute.c](https://opensource.apple.com/source/network_cmds/network_cmds-77/traceroute.tproj/traceroute.c.auto.html)
