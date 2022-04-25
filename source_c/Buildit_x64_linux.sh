mkdir -p builds
rm -rf builds/bin_x64_linux
rm -rf builds/obj_x64_linux
rm -rf builds/inc_x64_linux

mkdir builds/bin_x64_linux
mkdir builds/obj_x64_linux
mkdir builds/inc_x64_linux

cp includes_com/*.h builds/inc_x64_linux/
cp includes_samumi/*.h builds/inc_x64_linux/
for v in source_com/*.cpp; do g++ -pthread $v -g -O0 -Wall -Ibuilds/inc_x64_linux -c -o builds/obj_x64_linux/$(basename $v .cpp).o; done;
for v in source_com_any_linux/*.cpp; do g++ -pthread $v -g -O0 -Wall -Ibuilds/inc_x64_linux -c -o builds/obj_x64_linux/$(basename $v .cpp).o; done;
for v in source_com_x64_any/*.cpp; do g++ -pthread $v -g -O0 -Wall -Ibuilds/inc_x64_linux -c -o builds/obj_x64_linux/$(basename $v .cpp).o; done;
for v in source_samumi/*.cpp; do g++ -pthread $v -g -O0 -Wall -Ibuilds/inc_x64_linux -c -o builds/obj_x64_linux/$(basename $v .cpp).o; done;
g++ -pthread -o builds/bin_x64_linux/samumi builds/obj_x64_linux/*.o -g -O0 -Wall -Ibuilds/inc_x64_linux -lz -ldl

