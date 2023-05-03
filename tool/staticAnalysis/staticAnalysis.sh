#!/bin/bash -e

ROOT_DIR=''

# For Mac
if [ $(command uname) == "Darwin" ]; then
    if ! [ -x "$(command -v greadlink)" ]; then
        brew install coreutils
    fi
    BIN_PATH=$(greadlink -f "$0")
    ROOT_DIR=$(dirname $(dirname $(dirname $BIN_PATH)))
# For Linux
else
    BIN_PATH=$(readlink -f "$0")
    ROOT_DIR=$(dirname $(dirname $(dirname $BIN_PATH)))
fi

NAME=$1
echo "[*] Making static analysis directory"
ANALYSIS_FILE=$(realpath "$1.bc")
DIR=$(dirname $ANALYSIS_FILE)
echo "[*] DIR:" $DIR
echo "[*] ROOT_DIR:" $ROOT_DIR
export PATH=${ROOT_DIR}/tool/SVF/Release-build/bin:$PATH
export PATH=${ROOT_DIR}/clang+llvm/bin:$PATH
export LD_LIBRARY_PATH=${ROOT_DIR}/clang+llvm/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}

pushd $DIR >>/dev/null

echo "** plz make sure have combined all the bc files"

if [ ! -s ./mssa.$NAME ]; then
    echo "[*] Generating mssa.$NAME , CG "
    rm -f ./mssa.$NAME
    $ROOT_DIR/tool/staticAnalysis/analysis.py ./$NAME.bc >./mssa.$NAME
fi

if [ ! -s ./ConConfig.$NAME ]; then
    echo "[*] Change format for instrumentation"
    rm -f ./ConConfig.$NAME
    ${ROOT_DIR}/tool/staticAnalysis/get_aliased_info.py ./mssa.$NAME >./ConConfig.$NAME
fi

if [ ! -s ./dom_write.xml ]; then
    echo "[*] store func info"
    ${ROOT_DIR}/tool/staticAnalysis/get_info_function.py ./mssa.$NAME
fi

export Con_PATH=${DIR}/ConConfig.$NAME
if [ ! -s ./so.$NAME ]; then
    echo "[*] Analyze Sensitive Memory Operations"
    rm -f ./so.$NAME
    opt -load $ROOT_DIR/tool/staticAnalysis/LLVM-PASS/SensitiveOperationsPass/libSensitiveOperationsPass.so -so ./$NAME.bc -o /dev/null >so.$NAME
fi

if [ ! -s ./func_level_action.$NAME ]; then
    echo "[*] Analyze Sensitive Memory Operations"
    rm -f ./func_level_action.$NAME
    opt -load $ROOT_DIR/tool/staticAnalysis/LLVM-PASS/SensitiveOperationsPass_function_level/libSensitiveOperationsPass_function_level.so -so ./$NAME.bc -o /dev/null >func_level_action.$NAME
fi

echo "[*] Reforming passInfo"
${ROOT_DIR}/tool/staticAnalysis/get_passInfo_function.py ./func_level_action.$NAME ./dom_write.xml

# create sensitive.xml
echo "[*] Output sensitive actions"
${ROOT_DIR}/tool/staticAnalysis/out_sensitive_action.py ./finalXml.xml

rm -rf ./ConConfig.$NAME
if [ ! -s ./ConConfig.$NAME ]; then
    echo "[*] Update Memory Group with Sensitive Memory Operations"
    $ROOT_DIR/tool/staticAnalysis/updateMemGroupSo.py -i ./so.$NAME -m ./memGroup.npy >./ConConfig.$NAME
fi
rm -rf ./memGroup.npy

if [ ! -s ./mempair.$NAME ]; then
    echo "[*] Dumps mempairs and save it into .npz file"
    rm -f ./mempair_all.$NAME
    $ROOT_DIR/tool/staticAnalysis/DumpMempair.py -o mempair.$NAME.npz >./mempair.$NAME
fi
rm -rf ./soAndGroup.npz

if [ ! -s ./config.txt ]; then
    echo "[*] Remove config.text"
    rm -f ./config.txt
    $ROOT_DIR/tool/staticAnalysis/out_config.py ./finalXml.xml >./config.txt
fi

echo "[*] generate CFG"
opt -load $ROOT_DIR/tool/staticAnalysis/LLVM-PASS/Generate_CFG_CG/libGenerate_CFG_CGPass.so -so ./$NAME.bc -outdir $DIR -target config.txt -o /dev/null

echo "[*] gendistance start"
$ROOT_DIR/tool/staticAnalysis/gendistance_v2.py -d dot-files -xml ./finalXml.xml

export ConfigTXT=${DIR}/config.txt
echo "[*] Group keys"
opt -load $ROOT_DIR/tool/staticAnalysis/LLVM-PASS/GroupKey/libGroupKey.so -so ./$NAME.bc -o /dev/null >GroupKey.$NAME
unset ConfigTXT

# remove groupKeys.xml
if [ ! -s ./groupKeys.xml ]; then
    echo "[*] remove ./groupKeys.xml."
    rm -f groupKeys.xml 
fi
echo "[*] Generate Group keys xml."
$ROOT_DIR/tool/staticAnalysis/generateGroupKey.py GroupKey.$NAME

ls -lh *$NAME*
popd 2>/dev/null
