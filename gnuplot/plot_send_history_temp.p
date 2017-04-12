set terminal svg enhanced size 800 250 fname "Times New Roman" fsize 22 solid
set encoding iso_8859_1

set border 4095 front linetype -1 linewidth 1.000
set ticslevel 0

set xlabel "Time"
set xrange [0:*]

set key samplen 1 spacing .9

unset ytics
set boxwidth 1.0 relative
set style fill solid 1.0
set grid xtics ytics back lw 1 lc rgb "#AFAFAF"
set key outside right top


set output './Results/send_history.svg'
plot "./Results/log.txt" using 1:(($2==0) ? $7 : 1/0) with boxes ls 1 lw 1 linecolor 1 title "node=0",\
"./Results/log.txt" using 1:(($2==1) ? $7 : 1/0) with boxes ls 1 lw 1 linecolor 2 title "node=1",\
"./Results/log.txt" using 1:(($2==2) ? $7 : 1/0) with boxes ls 1 lw 1 linecolor 3 title "node=2"