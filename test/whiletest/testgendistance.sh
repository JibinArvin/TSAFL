BIN_PATH=$(readlink -f "$0")
ROOT_DIR=$(dirname $(dirname $(dirname $BIN_PATH)))
export ROOT_DIR=${ROOT_DIR}
set -eux
echo "start test"
cd $ROOT_DIR/test/whiletest
$ROOT_DIR/tool/staticAnalysis/gendistance.py -d dot-files -xml ./finalXml.xml
