#! /bin/sh

if [ "$#" != "4" ]; then
    echo \
"The following parameters are required:
- file name of the input .gif file (with extension),
- number specifying format of data in the output file (32 or 8),
- color of background (white or black),
- invertion of colours flag (1 or 0), and
- full pathname to xbm2bin.c"
    exit 1
fi

ifn=$1 # file name of the input .gif file (with extension)
dt=$2 # number specifying format of data in the output file (32 or 8)
bkgd=$3 # color of background (white or black)
invf=$4 # invertion of colours flag (1 or 0)"
xbm2bin=$5 # full pathname to xbm2bin.c (it is stored under utils/ of this repo)

ofn=`basename $1 .gif`
if [ ! -f $xbm2bin ]; then
    echo "$xbm2bin does not exist."
    exit 1
fi

convert -coalesce -flop $ifn $ifn.xbm
delays=(`identify -format "%T " $ifn`)

mkdir -p 128x32/
for fn in `ls -tr *.xbm`; do 
    convert $fn -resize 128x32 -background $bkgd -compose Copy \
        -gravity center -extent 128x32 128x32/$fn.128x32.xbm
    rm $fn
done

pushd 128x32/
for fn in `ls -tr *.xbm`; do 
    bn=`basename $fn .xbm`
    sed -i "s/$bn/line/" $fn
    cp $fn line.inc
    gcc \
        -w \
        -I. \
        -DXBM2BIN_U$dt -DXBM2BIN_INVERT=$invf \
        -o xbm2bin \
        $xbm2bin
    ./xbm2bin
    rm xbm2bin line.inc
    echo "// $bn" >$bn.bin
    cat line_bits.bin >>$bn.bin
    rm line_bits.bin
done

oinc="../$ofn-u$dt.inc"
#echo "static u$dt buffer[][$((32 / $dt)) * 128] = {" > $oinc
echo "static struct frames {u$dt buffer[$((32 / $dt))*128]; int delay;} frames[] = {" >$oinc
i=0
for fn in `ls -tr *.bin`; do 
    echo "{" >>$oinc
    cat $fn >>$oinc
    echo "${delays[$((i++))]}0" >>$oinc
    echo "}," >>$oinc
done
echo "};" >>$oinc
popd
rm -rf 128x32/

