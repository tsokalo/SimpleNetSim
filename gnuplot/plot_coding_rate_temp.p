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


set output './Results/coding_rate.svg'
plot "./Results/log.txt" using 1:(($3==0) ? $7 : 1/0) with linespoints ls 1 lw 1 linecolor 1 pt 7 ps 0.3 title "vertex=0",\
"./Results/log.txt" using 1:(($3==1) ? $7 : 1/0) with linespoints ls 1 lw 1 linecolor 2 pt 7 ps 0.3 title "vertex=1",\
"./Results/log.txt" using 1:(($3==2) ? $7 : 1/0) with linespoints ls 1 lw 1 linecolor 3 pt 7 ps 0.3 title "vertex=2",\
"./Results/log.txt" using 1:(($3==3) ? $7 : 1/0) with linespoints ls 1 lw 1 linecolor 4 pt 7 ps 0.3 title "vertex=3",\
"./Results/log.txt" using 1:(($3==4) ? $7 : 1/0) with linespoints ls 1 lw 1 linecolor 5 pt 7 ps 0.3 title "vertex=4",\
"./Results/log.txt" using 1:(($3==5) ? $7 : 1/0) with linespoints ls 1 lw 1 linecolor 6 pt 7 ps 0.3 title "vertex=5",\
"./Results/log.txt" using 1:(($3==6) ? $7 : 1/0) with linespoints ls 1 lw 1 linecolor 7 pt 7 ps 0.3 title "vertex=6",\
"./Results/log.txt" using 1:(($3==7) ? $7 : 1/0) with linespoints ls 1 lw 1 linecolor 8 pt 7 ps 0.3 title "vertex=7"