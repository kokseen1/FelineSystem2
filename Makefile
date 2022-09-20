EMXX     := em++
CXX      := g++
EMPPFLAGS := -DASSETS=\"assets/\"
CPPFLAGS := -DASSETS=\"build/apps/assets/\"
EMXXFLAGS := -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH -sUSE_ZLIB=1 -sUSE_SDL_MIXER=1 -fdeclspec 
CXXFLAGS := -w
LDFLAGS  := -LC:/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -lz
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
OBJ_DIR_LOCAL  := $(OBJ_DIR)/local
OBJ_DIR_WASM  := $(OBJ_DIR)/wasm
APP_DIR  := $(BUILD)/apps
TARGET_LOCAL   := a.exe
TARGET_WASM   := index.html
INCLUDE  := -Iinclude/ -Iinclude/asmodean/ -IC:/x86_64-w64-mingw32/include
SRC      :=                       \
   $(wildcard src/asmodean/*.cpp) \
   $(wildcard src/*.cpp)          \

OBJECTS_LOCAL  := $(SRC:%.cpp=$(OBJ_DIR_LOCAL)/%.o)
OBJECTS_WASM  := $(SRC:%.cpp=$(OBJ_DIR_WASM)/%.o)
DEP = $(<:%.cpp=$(OBJ_DIR)/%.d)

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES := $(OBJECTS:.o=.d)

MKDIR = if not exist "$(@D)" mkdir "$(@D)"

all: local wasm

wasm: $(APP_DIR)/$(TARGET_WASM)

local: $(APP_DIR)/$(TARGET_LOCAL)

$(OBJ_DIR_LOCAL)/%.o: %.cpp
	@$(MKDIR)
	$(CXX) -o $@ -c $< $(CXXFLAGS) $(INCLUDE) -MMD -MF $(DEP) $(CPPFLAGS)

$(OBJ_DIR_WASM)/%.o: %.cpp
	@$(MKDIR)
	$(EMXX) -o $@ -c $< $(EMXXFLAGS) $(CXXFLAGS) $(INCLUDE) -MMD -MF $(DEP) $(EMPPFLAGS)

$(APP_DIR)/$(TARGET_LOCAL): $(OBJECTS_LOCAL)
	@$(MKDIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

$(APP_DIR)/$(TARGET_WASM): $(OBJECTS_WASM)
	@$(MKDIR)
	$(EMXX) -o $@ $^ $(EMXXFLAGS) $(CXXFLAGS)

$(OBJ_DIR)/%.d:
	@$(MKDIR)

-include $(DEPENDENCIES)

.PHONY: all
