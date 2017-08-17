set terminal svg enhanced size 800 600 fname "Times New Roman" fsize 26 solid
set encoding iso_8859_1

set border 4095 front linetype -1 linewidth 1.000
set ticslevel 0

set ylabel "p_f"  offset 0
set yrange [0:1]
set format y '%0.2f'


set xlabel "Time / number of slots"
set xrange [0:*]

set grid xtics ytics back lw 1 lc rgb "#AFAFAF"
set key right bottom


set output './Results/input_filters_8_8.svg'
plot "./gnuplot/data.txt" every 10 using 1:(($2==8&&$3==5) ? $4 : 1/0) with linespoints ls 1 lw 1 linecolor 6 pt 7 ps 0.3 title "edge=<5,8>",\
"./gnuplot/data.txt" every 10 using 1:(($2==8&&$3==7) ? $4 : 1/0) with linespoints ls 1 lw 1 linecolor 8 pt 7 ps 0.3 title "edge=<7,8>"