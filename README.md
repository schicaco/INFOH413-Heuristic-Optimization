## Heuristic Optimization assignment.
Adapted by Jérémie Dubois-Lacoste from the ILSLOP implementation
of Tommaso Schiavinotto.



### To compile/clean:
make
make clean



### To run on one instance:

Paremeters : 

-------------------------------------------------------------
Option 1 : to run one specific combinaison of the algorithms 
-------------------------------------------------------------
first : -i <instance file>
second: --<Pivorting rule> (best, first)
third: --<neighborhood> (transpose; exchange, insert)
fourth: --<initial position> (random, cw)

Examples:
./lop -i <instance file> --first --transpose --cw
./lop -i <instance file> --best --exchange --random

-----------------------------------------------------------------------------------
Option 2 : run all combinaisons for II over all files as well as VND over all instances from a folder 
----------------------------------------------------------------------------------- 
./lop -all  

-----------------------------------------------------------
Option 3 : run one algorithm on all instances from a folder
-----------------------------------------------------------

./lop <--algorithm> --folder <folder_name>

Algorithms:
    --ii
    --vnd
    --rii
    --vns

Examples:

    ./lop --ii --folder instances
    ./lop --vns --folder instances




### Best known solutions:
The best known solutions are in best_known/best_known.txt
