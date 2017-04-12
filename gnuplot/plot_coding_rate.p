set terminal svg enhanced size 800 600 fname "Times New Roman" fsize 22 solid
set encoding iso_8859_1

set border 4095 front linetype -1 linewidth 1.000
set ticslevel 0

set ylabel "Coding rate"  offset 0
set yrange [1:*]
set format y '%0.2f'


set xlabel "Time"
set xrange [0:*]

set grid xtics ytics back lw 1 lc rgb "#AFAFAF"
set key right top


