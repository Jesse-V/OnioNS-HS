#!/bin/sh

echo "Re-styling code...        ----------------------------------------------"

# make the code-style consistent
for f in $(find src/ -type f -name "*.c*" | grep -v "libs"); do
   clang-format-3.6 -style="{BasedOnStyle: chromium, BreakBeforeBraces: Allman, MaxEmptyLinesToKeep: 3}" -i $f
done
# AlignConsecutiveAssignments: true
for f in $(find src/ -type f -name "*.h*" | grep -v "libs"); do
   clang-format-3.6 -style="{BasedOnStyle: chromium, BreakBeforeBraces: Allman, MaxEmptyLinesToKeep: 3}" -i $f
done

echo "Creating manpage...       ----------------------------------------------"

#date style to match Tor's manpage
date=$(date +%m\\/%d\\/%Y)
gzip --best -c src/assets/manpage | sed "s/<DATE>/$date/g" > src/assets/onions-hs.1.gz

echo "Preparing build...        ----------------------------------------------"

export CXX=/usr/bin/clang++
export CC=/usr/bin/clang

mkdir -p build/
cd build
cmake ../src # -DCMAKE_BUILD_TYPE=Debug

echo "Compiling...              ----------------------------------------------"
if (make -j $(grep -c ^processor /proc/cpuinfo)) then
  echo "Static analysis...        ----------------------------------------------"
  cd ..
  cppcheck --enable=all --platform=unix64 --inconclusive src/*
else
  rm -f onions-hs
  cd ..
fi
