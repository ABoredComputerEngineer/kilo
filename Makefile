$(shell mkdir -p bin)
$(shell mkdir -p test)
CC = gcc
DIR = $(shell pwd)
INC= $(DIR)/include
BIN = $(DIR)/bin
SRC = $(DIR)/src
TST = $(DIR)/tst

CFLAGS = -std=c99 -Wall -Werror
DEBUG = -g3 -gdwarf-2

kilo: $(BIN)/string.o $(BIN)/error.o $(BIN)/main.o
		$(CC) $(BIN)/main.o $(BIN)/error.o $(BIN)/string.o $(DEBUG) -o $(BIN)/kilo
$(BIN)/string.o: $(SRC)/string.c $(INC)/kilo_string.h
		$(CC) $(CFLAGS) $(DEBUG) -c $(SRC)/string.c -I $(INC) -o $(BIN)/string.o
$(BIN)/error.o: $(SRC)/error.c $(INC)/error.h
		$(CC) $(CFLAGS) $(DEBUG) -c $(SRC)/error.c -I $(INC) -o $(BIN)/error.o
$(BIN)/main.o: $(SRC)/kilo.c
		$(CC) $(CFLAGS) $(DEBUG) -c $(SRC)/kilo.c -I $(INC) -o $(BIN)/main.o

