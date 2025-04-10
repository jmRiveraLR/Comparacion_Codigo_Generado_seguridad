cmake_minimum_required(VERSION 3.10)

# Project name and version
project(Comparacion_Codigo_Generado_seguridad VERSION 1.0)

# Get all `.c` files in the directory
file(GLOB_RECURSE SOURCES "*.c")

# Create the executable
add_executable(Comparacion_Codigo_Generado_seguridad ${SOURCES})