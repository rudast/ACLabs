CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -O2
LDFLAGS = -pthread

SRCDIR = src
OBJDIR = build
SOURCES_SERVER = $(SRCDIR)/main_server.cpp $(SRCDIR)/server.cpp
SOURCES_CLIENT = $(SRCDIR)/main_client.cpp $(SRCDIR)/client.cpp
OBJECTS_SERVER = $(addprefix $(OBJDIR)/, $(notdir $(SOURCES_SERVER:.cpp=.o)))
OBJECTS_CLIENT = $(addprefix $(OBJDIR)/, $(notdir $(SOURCES_CLIENT:.cpp=.o)))

TARGET_SERVER = $(OBJDIR)/server
TARGET_CLIENT = $(OBJDIR)/client

all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET_SERVER): $(OBJDIR) $(OBJECTS_SERVER)
	$(CXX) $(OBJECTS_SERVER) -o $(TARGET_SERVER) $(LDFLAGS)

$(TARGET_CLIENT): $(OBJDIR) $(OBJECTS_CLIENT)
	$(CXX) $(OBJECTS_CLIENT) -o $(TARGET_CLIENT) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

.PHONY: all clean