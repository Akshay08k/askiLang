echo "Compiling..."

g++ -std=c++20 main.cpp
sleep 1
./a.out askiLang.al
./out
echo "Exit Code : "$?

echo "Done!"