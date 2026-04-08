CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -I.
TARGET = gpteam24_game
SRCS = main.cpp \
       maze.cpp \
       music_core.cpp \
       music_game.cpp \
       MusicManager.cpp \
       shooter_game.cpp

OBJS = $(SRCS:.cpp=.o)

# 确保链接 pthread（如果有线程相关要求）
LDFLAGS = -pthread

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
