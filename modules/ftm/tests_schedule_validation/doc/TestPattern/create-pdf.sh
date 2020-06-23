#!/bin/bash
if grep pos= $1 ; then
  neato -n -Tpdf -o ${1//.dot/.pdf1} $1;
else
  neato -Tpdf -o ${1//.dot/.pdf1} $1;
fi
gs -o ${1//.dot/.pdf} -sDEVICE=pdfwrite -dColorConversionStrategy=/sRGB -dProcessColorModel=/DeviceRGB ${1//.dot/.pdf1}
rm ${1//.dot/.pdf1}


# do this for all dot file in folder:
# for i in *.dot; do ./create-pdf.sh $i ; done
