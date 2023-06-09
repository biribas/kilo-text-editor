SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.c)

BUILDDIR = build
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))

TARGETDIR = bin
TARGET = kilo

CFLAGS = -Wall -Wextra -pedantic -std=c99 -I./include

$(TARGET): $(OBJECTS)
	@echo "Linking files..."
	@mkdir -p $(TARGETDIR)
	@$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGETDIR)/$(TARGET)
	@echo "Done!"

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo "Building $<"
	@mkdir -p $(BUILDDIR)
	@$(CC) -c $(CFLAGS) $< -o $@

clean:
	@$(RM) -rfv $(BUILDDIR) $(TARGETDIR)

