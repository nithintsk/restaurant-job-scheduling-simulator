# the compiler: gcc for C program, define as g++ for C++
  CC = g++

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings
  CFLAGS  = -g -Wall
  LDFLAGS = -lpthread -lrt

  # the build target executable:
  TARGET = Restaurant

  all: $(TARGET)

  $(TARGET): $(TARGET).cpp utility.cpp utility.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(TARGET).cpp utility.cpp

  clean:
	$(RM) $(TARGET)
