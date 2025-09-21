CXX = clang++
OUTPUT       = output/release
OUTPUT_DEBUG = output/debug
OUTPUT_WEB   = output/web

CXXFLAGS_COMMON = -O3 -std=c++23 -Wall -DNDEBUG -I./src -I./shaders -DSDL_MAIN_HANDLED -march=native -flto -fomit-frame-pointer -fno-rtti -ffast-math
CXXFLAGS_COMMON_VOLK = -O3 -Wall -march=native -flto -fomit-frame-pointer
CXXFLAGS_COMMON_DEBUG   = -O0 -g3 -Wall -I./src -I./shaders -DSDL_MAIN_HANDLED -std=c++23

ifeq ($(OS),Windows_NT)
	# windows
	TARGET = $(OUTPUT)/engine.exe
	TARGET_DEBUG = $(OUTPUT_DEBUG)/engine.exe

	VULKAN_LOCATION = -IC:\VulkanSDK\1.4.321.1\Include
	SDL3_LOCATION = -IC:\SDL3-3.2.20\include
	CXXFLAGS = $(CXXFLAGS_COMMON) $(VULKAN_LOCATION) $(SDL3_LOCATION)
	CXXFLAGS_VOLK = $(CXXFLAGS_COMMON_VOLK) $(VULKAN_LOCATION) $(SDL3_LOCATION)
	CXXFLAGS_DEBUG = $(CXXFLAGS_COMMON_DEBUG) $(VULKAN_LOCATION) $(SDL3_LOCATION)

	LDFLAGS = -fuse-ld=lld -LC:\VulkanSDK\1.4.321.1\Lib -LC:\SDL3-3.2.20\lib\x64 -lopengl32 -lGLEW32 -lvulkan-1 -lSDL3 -lSDL3_image -lkernel32 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lversion -luuid -ladvapi32 -lsetupapi -lshell32 -ldinput8
else
    # linux
    TARGET = $(OUTPUT)/engine
	TARGET_DEBUG = $(OUTPUT_DEBUG)/engine

    CXXFLAGS = $(CXXFLAGS_COMMON)
	CXXFLAGS_VOLK = $(CXXFLAGS_COMMON_VOLK)
	CXXFLAGS_DEBUG = $(CXXFLAGS_COMMON_DEBUG)

    LDFLAGS  = -fuse-ld=lld -static-libstdc++ -static-libgcc -lSDL3 -lSDL3_image -lGL -lGLU -lGLEW -lvulkan -ldl -lpthread -lm
endif

OBJS_COMMON = main.o vk_device.o vk_frames.o vk_buffers.o vk_loadGLTF.o vk_command.o vk_sync.o volk.o common.o gl_shader.o gl_loadGLTF.o fix.o
OBJS_DEBUG  = vk_debug.o

.PHONY: all debug clean run run-debug
all: $(TARGET)
debug: $(TARGET_DEBUG)
emcc: $(OUTPUT_WEB)/main.html

$(TARGET): $(addprefix $(OUTPUT)/,$(OBJS_COMMON))
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OUTPUT)/%.o: src/%.cpp | $(OUTPUT)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET_DEBUG): $(addprefix $(OUTPUT_DEBUG)/,$(OBJS_COMMON) $(OBJS_DEBUG))
	$(CXX) $(CXXFLAGS_DEBUG) -o $@ $^ $(LDFLAGS)

$(OUTPUT_DEBUG)/%.o: src/%.cpp | $(OUTPUT_DEBUG)
	$(CXX) $(CXXFLAGS_DEBUG) -c $< -o $@

$(OUTPUT)/volk.o: src/volk.c
	$(CXX) -x c $(CXXFLAGS_VOLK) -c $< -o $@

$(OUTPUT_DEBUG)/volk.o: src/volk.c
	$(CXX) -x c $(CXXFLAGS_VOLK) -c $< -o $@

$(OUTPUT) $(OUTPUT_DEBUG) $(OUTPUT_WEB):
	mkdir -p $@

$(OUTPUT_WEB)/main.html: webgpu.cpp | $(OUTPUT_WEB)
	emcc -O3 -Wall \
	-I. \
	-I/path/to/emscripten/ports/emdawnwebgpu/emdawnwebgpu_pkg/webgpu_cpp/include \
	-DSDL_MAIN_HANDLED -std=c++23 \
	webgpu.cpp \
	-s FORCE_FILESYSTEM=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s USE_SDL=2 \
	-s ASSERTIONS=1 \
	--use-preload-plugins \
	--use-port=emdawnwebgpu \
	-o $@

run: $(TARGET)
	./$(TARGET) $(ARGS)

run-debug: $(TARGET_DEBUG)
	./$(TARGET_DEBUG) $(ARGS)

server:
	cd $(OUTPUT_WEB)
	npx live-server . -o -p 9999

clean:
	rm -rf output