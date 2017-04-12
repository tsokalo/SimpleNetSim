set terminal svg enhanced size 800 600 fname "Times New Roman" fsize 22 solid
set encoding iso_8859_1

set border 4095 front linetype -1 linewidth 1.000
set ticslevel 0

set ylabel "Relative difference"  offset 0
set yrange [-1:1]
set format y '%0.2f'

set xlabel "Batch index"

set grid xtics ytics back lw 1 lc rgb "#AFAFAF"
set key right top

set output './Results/stability.svg'
plot "./gnuplot/data.txt" using 1:2 with linespoints ls 1 lw 1 linecolor 1 pt 7 ps 0.3 notitle
