# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/alexey/projects/conGame

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/alexey/projects/conGame/build

# Include any dependencies generated for this target.
include ConRender/CMakeFiles/conrenderlib.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include ConRender/CMakeFiles/conrenderlib.dir/compiler_depend.make

# Include the progress variables for this target.
include ConRender/CMakeFiles/conrenderlib.dir/progress.make

# Include the compile flags for this target's objects.
include ConRender/CMakeFiles/conrenderlib.dir/flags.make

ConRender/CMakeFiles/conrenderlib.dir/conrender.cpp.o: ConRender/CMakeFiles/conrenderlib.dir/flags.make
ConRender/CMakeFiles/conrenderlib.dir/conrender.cpp.o: /home/alexey/projects/conGame/ConRender/conrender.cpp
ConRender/CMakeFiles/conrenderlib.dir/conrender.cpp.o: ConRender/CMakeFiles/conrenderlib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/alexey/projects/conGame/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object ConRender/CMakeFiles/conrenderlib.dir/conrender.cpp.o"
	cd /home/alexey/projects/conGame/build/ConRender && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT ConRender/CMakeFiles/conrenderlib.dir/conrender.cpp.o -MF CMakeFiles/conrenderlib.dir/conrender.cpp.o.d -o CMakeFiles/conrenderlib.dir/conrender.cpp.o -c /home/alexey/projects/conGame/ConRender/conrender.cpp

ConRender/CMakeFiles/conrenderlib.dir/conrender.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/conrenderlib.dir/conrender.cpp.i"
	cd /home/alexey/projects/conGame/build/ConRender && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/alexey/projects/conGame/ConRender/conrender.cpp > CMakeFiles/conrenderlib.dir/conrender.cpp.i

ConRender/CMakeFiles/conrenderlib.dir/conrender.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/conrenderlib.dir/conrender.cpp.s"
	cd /home/alexey/projects/conGame/build/ConRender && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/alexey/projects/conGame/ConRender/conrender.cpp -o CMakeFiles/conrenderlib.dir/conrender.cpp.s

# Object files for target conrenderlib
conrenderlib_OBJECTS = \
"CMakeFiles/conrenderlib.dir/conrender.cpp.o"

# External object files for target conrenderlib
conrenderlib_EXTERNAL_OBJECTS =

ConRender/libconrenderlib.a: ConRender/CMakeFiles/conrenderlib.dir/conrender.cpp.o
ConRender/libconrenderlib.a: ConRender/CMakeFiles/conrenderlib.dir/build.make
ConRender/libconrenderlib.a: ConRender/CMakeFiles/conrenderlib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/alexey/projects/conGame/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libconrenderlib.a"
	cd /home/alexey/projects/conGame/build/ConRender && $(CMAKE_COMMAND) -P CMakeFiles/conrenderlib.dir/cmake_clean_target.cmake
	cd /home/alexey/projects/conGame/build/ConRender && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/conrenderlib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
ConRender/CMakeFiles/conrenderlib.dir/build: ConRender/libconrenderlib.a
.PHONY : ConRender/CMakeFiles/conrenderlib.dir/build

ConRender/CMakeFiles/conrenderlib.dir/clean:
	cd /home/alexey/projects/conGame/build/ConRender && $(CMAKE_COMMAND) -P CMakeFiles/conrenderlib.dir/cmake_clean.cmake
.PHONY : ConRender/CMakeFiles/conrenderlib.dir/clean

ConRender/CMakeFiles/conrenderlib.dir/depend:
	cd /home/alexey/projects/conGame/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/alexey/projects/conGame /home/alexey/projects/conGame/ConRender /home/alexey/projects/conGame/build /home/alexey/projects/conGame/build/ConRender /home/alexey/projects/conGame/build/ConRender/CMakeFiles/conrenderlib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : ConRender/CMakeFiles/conrenderlib.dir/depend

