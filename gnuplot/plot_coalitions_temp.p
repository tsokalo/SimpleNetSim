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


set output './Results/coalitions.svg'
plot "/home/tsokalo/workspace/ns-allinone-3.25/ns-3.25/build/scratch/Results/log.txt" every 2 using ($1/1000):(($3==0) ? $7 : 1/0) with linespoints ls 1 lw 1 linecolor 1 pt 7 ps 0.3 title "vertex=0",\
"/home/tsokalo/workspace/ns-allinone-3.25/ns-3.25/build/scratch/Results/log.txt" every 2 using ($1/1000):(($3==1) ? $7 : 1/0) with linespoints ls 1 lw 1 linecolor 2 pt 7 ps 0.3 title "vertex=1"