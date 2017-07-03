set terminal svg enhanced size 800 600 fname "Times New Roman" fsize 26 solid
set encoding iso_8859_1

set border 4095 front linetype -1 linewidth 1.000
set ticslevel 0

set ylabel "Loss ratio"  offset 0
set yrange [0:1]
set format y '%0.2f'


set xlabel "Time / number of slots"
set xrange [0:*]

set grid xtics ytics back lw 1 lc rgb "#AFAFAF"
set key right top


set output './Results/loss_ratios_7.svg'
plot "./gnuplot/data.txt" using 1:(($2==7&&$3==8) ? $4 : 1/0) with linespoints ls 1 lw 1 linecolor 9 pt 7 ps 0.3 title "edge=<7,8>"