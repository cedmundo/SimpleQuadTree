# SimpleQuadTree

A very simple and naive implementation of quadtree (using recursive structures), you can draw points
with left mouse button, move query with mouse middle button and resize query with right button.

## Building
First install all dependencies needed to build raylib from source, then fetch code with submodules:
```bash
git clone --recurse-submodules https://github.com/cedmundo/SimpleQuadTree.git
```
Build with CMake:
```bash
mkdir -p cmake-build-debug
cmake -DCMAKE_BUILD_TYPE=debug -B cmake-build-debug
cmake --build cmake-build-debug 
```
Done.