# herocore

## About the project

I don't like current tmnf server controlers (XASECO, TRAKMAN, minicontrol), because they are very slow and hard to scale. Thats why I created herocore.

herocore is a lightweight, high-performance server controller built from the ground up for speed and scalability. Using gRPC, it enables low-latency communication between tmnf server and custom backend services.

## Stack

- [userver](https://userver.tech)

## Makefile

`PRESET` is either `debug`, `release`.

* `make cmake-PRESET` - run cmake configure, update cmake options and source file lists
* `make build-PRESET` - build the service
* `make test-PRESET` - build the service and run all tests
* `make start-PRESET` - build the service, start it in testsuite environment and leave it running
* `make install-PRESET` - build the service and install it in directory set in environment `PREFIX`
* `make` or `make all` - build and run all tests in `debug` and `release` modes
* `make format` - reformat all C++ and Python sources
* `make dist-clean` - clean build files and cmake cache
* `make docker-COMMAND` - run `make COMMAND` in docker environment
* `make docker-clean-data` - stop docker containers


## License

Template of a C++ service that uses [userver framework](https://github.com/userver-framework/userver) is distributed under the [Apache-2.0 License](https://github.com/userver-framework/userver/blob/develop/LICENSE)
and [CLA](https://github.com/userver-framework/userver/blob/develop/CONTRIBUTING.md). Services based on the template may change
the license and CLA.