CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -O2
LDFLAGS = -pthread

SRCDIR = src
OBJDIR = build

TARGETS = $(OBJDIR)/broker $(OBJDIR)/producer $(OBJDIR)/consumer

BROKER_SRC = $(SRCDIR)/broker.cpp
PRODUCER_SRC = $(SRCDIR)/producer.cpp
CONSUMER_SRC = $(SRCDIR)/consumer.cpp

HEADERS = $(SRCDIR)/utils.h

all: $(TARGETS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/broker: $(BROKER_SRC) $(HEADERS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(BROKER_SRC) -o $@ $(LDFLAGS)

$(OBJDIR)/producer: $(PRODUCER_SRC) $(HEADERS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(PRODUCER_SRC) -o $@ $(LDFLAGS)

$(OBJDIR)/consumer: $(CONSUMER_SRC) $(HEADERS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(CONSUMER_SRC) -o $@ $(LDFLAGS)


clean:
	rm -rf $(OBJDIR)

.PHONY: all clean