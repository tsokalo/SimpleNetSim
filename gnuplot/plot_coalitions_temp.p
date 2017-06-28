set terminal svg enhanced size 800 600 fname "Times New Roman" fsize 26 solid
set encoding iso_8859_1

set border 4095 front linetype -1 linewidth 1.000
set ticslevel 0

set ylabel "N_c"  offset 1
set yrange [0:*]
set ytics 1
set format y '%0.0f'


set xlabel "Time / number of slots"
set xrange [0:*]
set format x '%0.0f'

set grid xtics ytics back lw 1 lc rgb "#AFAFAF"
set key right bottom


set output './Results/coalitions_8.svg'
plot "./gnuplot/data.txt" every 10 using ($1):(($2==0) ? $3 : 1/0) with linespoints ls 1 lw 1 linecolor 1 pt 7 ps 0.3 title "vertex=0",\
"./gnuplot/data.txt" every 10 using ($1):(($2==1) ? $3 : 1/0) with linespoints ls 1 lw 1 linecolor 2 pt 7 ps 0.3 title "vertex=1",\
"./gnuplot/data.txt" every 10 using ($1):(($2==2) ? $3 : 1/0) with linespoints ls 1 lw 1 linecolor 3 pt 7 ps 0.3 title "vertex=2",\
"./gnuplot/data.txt" every 10 using ($1):(($2==3) ? $3 : 1/0) with linespoints ls 1 lw 1 linecolor 4 pt 7 ps 0.3 title "vertex=3",\
"./gnuplot/data.txt" every 10 using ($1):(($2==4) ? $3 : 1/0) with linespoints ls 1 lw 1 linecolor 5 pt 7 ps 0.3 title "vertex=4",\
"./gnuplot/data.txt" every 10 using ($1):(($2==5) ? $3 : 1/0) with linespoints ls 1 lw 1 linecolor 6 pt 7 ps 0.3 title "vertex=5",\
"./gnuplot/data.txt" every 10 using ($1):(($2==6) ? $3 : 1/0) with linespoints ls 1 lw 1 linecolor 7 pt 7 ps 0.3 title "vertex=6",\
"./gnuplot/data.txt" every 10 using ($1):(($2==7) ? $3 : 1/0) with linespoints ls 1 lw 1 linecolor 8 pt 7 ps 0.3 title "vertex=7",\
