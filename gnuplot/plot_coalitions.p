set terminal svg enhanced size 800 600 fname "Times New Roman" fsize 26 solid
set encoding iso_8859_1

set border 4095 front linetype -1 linewidth 1.000
set ticslevel 0

set ylabel "Coalition size"  offset 1
set yrange [0:*]
set ytics 1
set format y '%0.0f'


set xlabel "Time / seconds"
set xrange [1:*]
set format x '%0.1f'

set grid xtics ytics back lw 1 lc rgb "#AFAFAF"
set key right bottom


