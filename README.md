# Toll-Management-System

## Environment

This project has been developed using C++ and mongocxx/mongoc libraries. MongoDb has been used as the database. Development was done on macOS Big Sur platform (MacBook Pro Retina, 13-inch, Mid 2014).

## Setup

1. Download code from the repository.
2. Install mongocxx/mongoc libraries. Follow instructions from https://mongocxx.org/mongocxx-v3/installation/macos/.
3. Copy server and SimpleJSON source files from the links below and place them in a folder named lib in the root.
	* https://github.com/evanugarte/served.git
	* https://raw.githubusercontent.com/nbsdx/SimpleJSON/master/json.hpp

## Build

Build from Makefile as `make all`. This will build 2 executables. **main** for starting the server and testing the REST APIs and **validateUnitTests** for running toll ticket validation tests.

## Run

1. Start the server as `./bin/main` and provide requests using POSTMAN on http://127.0.0.1:5000
2. Run toll ticket validation tests as `./bin/validateUnitTests`.

## References

1. [Working with MongoDB in C++ | mongocxx tutorial](https://www.youtube.com/watch?v=yPoH5cBJzkk)
2. [Creating a Simple REST API in C++ | mongocxx tutorial pt. 2](https://www.youtube.com/watch?v=NC7IGLm69Ts)