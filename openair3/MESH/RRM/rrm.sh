
#!/bin/bash

xterm -T CMM -hold -e ./emul_int  &
sleep 1
./rrm -i 3 & #2> log.txt
#sleep 1
#./emul_int

