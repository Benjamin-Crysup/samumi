IF NOT EXIST builds MKDIR builds
IF EXIST builds\bin_x64_win32 RMDIR /S /Q builds\bin_x64_win32
IF EXIST builds\obj_x64_win32 RMDIR /S /Q builds\obj_x64_win32
IF EXIST builds\inc_x64_win32 RMDIR /S /Q builds\inc_x64_win32

MKDIR builds\bin_x64_win32
MKDIR builds\obj_x64_win32
MKDIR builds\inc_x64_win32

COPY includes_com\*.h builds\inc_x64_win32
COPY includes_samumi\*.h builds\inc_x64_win32
FOR %%v in (source_com\*.cpp) DO g++ -mthreads %%v -g -O0 -Wall -Ibuilds\inc_x64_win32 -c -o builds\obj_x64_win32\%%~nv.o
FOR %%v in (source_com_any_win32\*.cpp) DO g++ -mthreads %%v -g -O0 -Wall -Ibuilds\inc_x64_win32 -c -o builds\obj_x64_win32\%%~nv.o
FOR %%v in (source_com_x64_any\*.cpp) DO g++ -mthreads %%v -g -O0 -Wall -Ibuilds\inc_x64_win32 -c -o builds\obj_x64_win32\%%~nv.o
FOR %%v in (source_samumi\*.cpp) DO g++ -mthreads %%v -g -O0 -Wall -Ibuilds\inc_x64_win32 -c -o builds\obj_x64_win32\%%~nv.o
g++ -mthreads -o builds\bin_x64_win32\samumi.exe builds\obj_x64_win32\*.o -g -O0 -Wall -Ibuilds\inc_x64_win32 -lz -static-libgcc -static-libstdc++ -static -lpthread
