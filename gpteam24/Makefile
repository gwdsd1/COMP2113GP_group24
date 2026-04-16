CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -I.
TARGET = gpteam24_game
SRCS = main.cpp \
       game.cpp \
       maze.cpp \
       miniaudio_imple.cpp \
       music_core.cpp \
       music_game.cpp \
       MusicManager.cpp \
       shooter_game.cpp \
       snake_game.cpp \
       enemy_quiz.cpp
OBJS = $(SRCS:.cpp=.o)

# 确保链接 pthread（如果有线程相关要求）和 miniaudio 依赖
LDFLAGS = -pthread -ldl -lm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

main.o: game.h MusicManager.h
game.o: game.h maze.h MusicManager.h
maze.o: maze.h music_game.h shooter_game.h snake_game.h enemy_quiz.h MusicManager.h
music_game.o: music_game.h MusicManager.h
shooter_game.o: shooter_game.h MusicManager.h
snake_game.o: snake_game.h MusicManager.h
enemy_quiz.o: enemy_quiz.h
MusicManager.o: MusicManager.h
