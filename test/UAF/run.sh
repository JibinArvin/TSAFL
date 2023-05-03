opt -load-pass-plugin $1 --passes="Instru" ./$1.c -o instrumented.bc
clang++ -g  -o0 ./instrumented.bc toy/build/lib/libFunction.a -o out_massage -lpthread -ldl -rdynamic
./out_massage