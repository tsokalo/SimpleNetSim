set terminal svg enhanced size 800 600 fname "Times New Roman" fsize 26 solid
set encoding iso_8859_1

set border 4095 front linetype -1 linewidth 1.000
set ticslevel 0

set ylabel "Data rate / Mbps"  offset 0
set yrange [0:*]
set format y '%0.2f'


set xlabel "Time / milliseconds"
set xrange [0:*]
set format x '%0.0f'

set grid xtics ytics back lw 1 lc rgb "#AFAFAF"
set key below left


