CXX		  := g++
CXX_FLAGS := -Wall -Wextra -std=c++17 -ggdb

BIN		:= bin
SRC		:= src/main.cpp \
lib/served/parameters.cpp \
lib/served/multiplexer.cpp \
lib/served/net/connection.cpp \
lib/served/net/connection_manager.cpp \
lib/served/net/server.cpp \
lib/served/mux/static_matcher.cpp \
lib/served/mux/regex_matcher.cpp \
lib/served/mux/variable_matcher.cpp \
lib/served/request_parser.cpp \
lib/served/status.cpp \
lib/served/request.cpp \
lib/served/uri.cpp \
lib/served/response.cpp \
lib/served/methods_handler.cpp \
lib/served/request_parser_impl.cpp

INCLUDE	:= -Iinclude -Ilib \
 -I/usr/local/include/mongocxx/v_noabi \
 -I/usr/local/include/bsoncxx/v_noabi
LIB		:= lib

LIBRARIES	:= -lboost_system -lpthread \
-lmongocxx -lbsoncxx
EXECUTABLE	:= main
UNITTEST := validateUnitTests

all: $(BIN)/$(EXECUTABLE) $(BIN)/$(UNITTEST)

run: all

$(BIN)/$(EXECUTABLE): $(SRC)
	$(CXX) $(CXX_FLAGS) $(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES)

$(BIN)/$(UNITTEST): src/validateUnitTests.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES)

clean:
	-rm $(BIN)/*
