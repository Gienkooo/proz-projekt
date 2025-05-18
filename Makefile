CXX = mpic++
CXXFLAGS = -std=c++17 -Wall -pthread -g
LDFLAGS = 

TARGET = projekt

SOURCES = main.cpp ProcessLogic.cpp ResourceManager.cpp MessageHandler.cpp

OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)


%.o: %.cpp %.h types.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

main.o: main.cpp ProcessLogic.h types.h
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

ProcessLogic.o: ProcessLogic.cpp ProcessLogic.h MessageHandler.h ResourceManager.h ClockManager.h types.h
	$(CXX) $(CXXFLAGS) -c ProcessLogic.cpp -o ProcessLogic.o

ResourceManager.o: ResourceManager.cpp ResourceManager.h MessageHandler.h ClockManager.h types.h
	$(CXX) $(CXXFLAGS) -c ResourceManager.cpp -o ResourceManager.o

MessageHandler.o: MessageHandler.cpp MessageHandler.h ClockManager.h types.h ProcessLogic.h
	$(CXX) $(CXXFLAGS) -c MessageHandler.cpp -o MessageHandler.o

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	mpirun -np 5 ./$(TARGET) # Defaulting to 5 as per N_PROCESSES_DEFAULT

run3: $(TARGET)
	mpirun -np 3 ./$(TARGET)

.PHONY: all clean run
