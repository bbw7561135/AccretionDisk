set xlabel "Radius (cm)"
set ylabel "Scale Height (cm)"
set logscale xy

plot for [i=0:100:2] "HvsR.txt" every :1::i::i