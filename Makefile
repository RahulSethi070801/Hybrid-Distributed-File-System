CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
# Target and source files
TARGET = Daemon
SRCS = main.cpp utils.cpp HyDFSMessage.cpp Node.cpp FileMetaData.cpp HyDFS.cpp ConsistentHashRing.cpp Cache.cpp 
OBJS = $(SRCS:.cpp=.o)

# Rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) ./stats/*.txt

cfiles:
	rm -rf ./files/A* ./files/wireless* ./files/introducer/hydfs/* ./files/remote/hydfs/* ./files/fa24-cs425*