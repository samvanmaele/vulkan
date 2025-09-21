#!/bin/bash
cd "$(dirname "$0")" || exit

glslc triangle.vert -o triangle.vert.spv
glslc triangle.frag -o triangle.frag.spv