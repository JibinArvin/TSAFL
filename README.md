# TSAFL: a thread sensitive Fuzzer
TSAFL is a thread sensitive Fuzzer based on AFL, which use some optimized strategy to imporve the performance of fuzz on concurrent program.
This repository provides the source code of TSAFL.

## Directory Structure
The repository mainly contains three folders: tool, test and evaluation. We briefly introduce each folder under ${ROOT_DIR} (if you install by using docker, the root folder of the artifact would be ${ROOT_DIR}):
- tools: Root directory for PERIOD tool and scripts.
  - AFL-2.52b: the AFL (american fuzzy lop)
  - SVF: Third-party libraries project SVF, which provides interprocedural dependence analysis for LLVM-based languages. SVF is able to perform pointer alias analysis, memory SSA form construction, value-flow tracking for program variables.
  - analysis：There are some shell scipt to call the static_analysis.sh in staticAnalysis.
  - staticAnalysis: The scripts of static analysis, which aim to find key points that used to be instrucmented in compiler proccess and generate various information including all kinds of xml.
  - TSAFL：The TSAFL source code is here, which is most based on the AFL.
    - LLVM_MODE: We have already change the code here to adjust our need. The cur_llvm_pass.so.cc is a llvm-pass for assert instrucment location. Currency-instr.h and Currency-instr.cpp provide the Pin code(instrucment code).
    - tsafl: contain the TSAFL which is builded based on the afl-2.32.
- test: Some simple examples that are easy to understand.
- clang+llvm: Pre-Built Binaries of LLVM 12.0. These binaries include Clang, LLD, compiler-rt, various LLVM tools, etc.

## Tool
We strongly recommend installing and running our tool based on Docker, since the scheduling part is must under root. The docker envoriment could help a lot.
  - Docker: The only requirement is to install Docker (version higher than 18.09.7). You can use the command sudo apt-get install docker.io to install the docker on your Linux machine. (If you have any questions on docker, you can see Docker's Documentation).
  - Operating System: Linux kernel version >= 5.x due to thread scheduling implement.
### Installing 
  - You can build your own image with  `sudo docker build -t tsafl:01 --no-cache ./`
### Running on docker
  You need run the docker in privileged mode!
  `sudo docker run --privileged -it tsafl:01 /bin/bash`

### Build the tool inside the docker
  Since network problem of where I am living, the download of things from github go wrong too many times. I recommend to build the tsafl by hand.
  ```
    ./tool/build_tsafl.sh
    ./tool/ninja_install_static_analysis.sh
  ```
 ## USAGE
  After finishing all preparation, you can find tsafl in "/tool/TSAFL/build/tsafl/tsafl".
  First of all, plz get the static analysis result.
    - Get the bc file from projects. There are several way to get the bc file, but I recommend usign wllvm. If you want to know plz visit https://github.com/travitch/whole-program-llvm. 
      ```
        pip install wllvm
        export LLVM_COMPILER=clang
        cd Target-file
        CC=wllvm ./configure
        make
        extract-bc target-file
      ```
    - Get the static analysis of bc code.
      - move bc code into tool/analysis
      - follow the steps
      ```
        ./static_analysis.sh target # don't add .bc!
        mv -r *.xml ${target-files}/
        mv config.txt ${target-files}/
      ```
     - Build target program with Pin.(Depend on the build system)
       ```
        export ConFile_PATH=config.txt
        export CC=${TSAFL_DIR}/CUR-clang-fast
        export CXX=${TSAFL_DIR}/CUR-clang-fast++
        ./configure
        make
       ```
      - Fuzz it 
        `${BUILD_DIR}/tsafl -i intputs -o findings_dir -m 200 ./target -argumet @@`
