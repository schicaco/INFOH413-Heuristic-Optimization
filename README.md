**** Heuristic Optimization assignment.
Adapted by Jérémie Dubois-Lacoste from the ILSLOP implementation
of Tommaso Schiavinotto.



*** To compile/clean:
make
make clean



*** To run on one instance:

Paremeters : 
first : -i <instance file>
second: --<Pivorting rule> (best, first)
third: --<neighborhood> (transpose; exchange, insert)
fourth: --<initial position> (random, cw)

Examples:
./lop -i <instance file> --first --transpose --cw
./lop -i <instance file> --best --exchange --random



*** Best known solutions:
The best known solutions are in best_known/best_known.txt
