
rm build -rf
mkdir -p build

# git submodule init
# git submodule update

# cd "thirdparty/coco"
# git submodule init
# git submodule update

# cd ../..

cd build
cmake3 ..
make