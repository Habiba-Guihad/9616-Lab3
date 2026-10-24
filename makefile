CXX      = g++
CXXFLAGS = -Wall -g -std=c++11
TARGET   = myshell
OBJS     = command.o tokenizer.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cc command.h tokenizer.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
