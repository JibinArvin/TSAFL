#!/bin/bash
# export export CXX=/workdir/tsafl/tool/TSAFL/CUR-clang-fast++
export ConFile_PATH=config.txt
export AFL_NO_UI
export AFL_SKIP_BIN_CHECK
${BUILD_DIR}/tsafl -i intputs -o findings_dir -m 200 ./pbz2 -f -k -p4 -S16 -d @@⏎                        