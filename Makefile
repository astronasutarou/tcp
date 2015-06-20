CC = g++
OPT= -Wall -std=c++11 -O2

ALL: test_client test_server

%: %.cc tcp.h
	$(CC) -o $@ $(OPT) $<
