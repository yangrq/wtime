# wtime
  Displays information about the running time of a program on Windows ( like command 'time' on linux )
## Compile
### VS
  Please create a new empty vsproject and add wtime.cpp to the source.
- Notice: require VS2013 or higher.Need c++11 supports.
### MinGW
  Build like this
  > gcc -Os -std=c++11 -static -Wall wtime.cpp -o wtime.exe
