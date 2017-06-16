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


