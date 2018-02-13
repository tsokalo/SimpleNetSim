set terminal svg enhanced size 1000 600 fname "Times New Roman" fsize 26 solid
set encoding iso_8859_1

set border 4095 front linetype -1 linewidth 1.000
set ticslevel 0

set ylabel "Priority / Kbits per symbol"  offset 1
set yrange [0:*]
set format y '%0.1f'


set xlabel "Time / seconds"
set xrange [0:*]
set format x '%0.1f'

set grid xtics ytics back lw 1 lc rgb "#AFAFAF"
set key right outside


