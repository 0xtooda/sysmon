##? sysmon Makefile — Copyright 2025 sysmon contributors, Apache-2.0

PROGRAM    := sysmon
VERSION    := 0.1.0

CXX        ?= g++
CXXFLAGS   := -std=c++23 -O2 -Wall -Wextra
CXXFLAGS   += -Iinclude
LDFLAGS    :=

ifeq ($(ASAN),1)
CXXFLAGS   += -fsanitize=address,undefined -g
LDFLAGS    += -fsanitize=address,undefined
endif

PREFIX     ?= /usr/local
BINDIR     := $(PREFIX)/bin
SHAREDIR   := $(PREFIX)/share/$(PROGRAM)
DESKTOPDIR := $(PREFIX)/share/applications

SRCDIR  := src
OBJDIR  := obj

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean install uninstall

all: $(PROGRAM)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@printf "  %-8s %s\n" "CXX" "$<"
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(PROGRAM): $(OBJS)
	@printf "  %-8s %s\n" "LD" "$@"
	@$(CXX) $(LDFLAGS) $^ -o $@
	@printf "  %-8s %s\n" "OK" "$(PROGRAM)"
	@printf "  built    successfully\n"

clean:
	@rm -rf $(OBJDIR) $(PROGRAM)
	@printf "  cleaned\n"

install: $(PROGRAM)
	@install -Dm755 $(PROGRAM)       $(DESTDIR)$(BINDIR)/$(PROGRAM)
	@install -Dm644 sysmon.desktop   $(DESTDIR)$(DESKTOPDIR)/$(PROGRAM).desktop
	@install -d                      $(DESTDIR)$(SHAREDIR)/themes
	@install -Dm644 themes/*.theme   $(DESTDIR)$(SHAREDIR)/themes/
	@printf "  installed to %s\n" "$(DESTDIR)$(PREFIX)"

uninstall:
	@rm -f  $(DESTDIR)$(BINDIR)/$(PROGRAM)
	@rm -f  $(DESTDIR)$(DESKTOPDIR)/$(PROGRAM).desktop
	@rm -rf $(DESTDIR)$(SHAREDIR)
	@printf "  uninstalled\n"

-include $(DEPS)
