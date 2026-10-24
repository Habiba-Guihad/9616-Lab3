CXX      = g++
CXXFLAGS = -Wall -g -std=c++11
TARGET   = myshell
OBJS     = command.o tokenizer.o

all: $(TARGET)

$(TARGET): $(OBJS)
<TAB>$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cc command.h tokenizer.h
<TAB>$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
<TAB>rm -f $(TARGET) $(OBJS)## you have command.cc command.h tokenizer.cc tokenizer.h you need compile them and link
## them to one object file called  : "myshell" 
## so we should run make , that will create our object myshell ,
## ./myshell should run your application 
