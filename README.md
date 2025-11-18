# Plan
0. Start program
- read parameters
- return path

1. File input (path)
- check if exists
- check if valid extensions
- read file and save it to string (maybe chunk reading)

2. Loop 
    - state
        - index of currect position in string
        - tree (stack maybe) of tokens

    - token = LexAzer.GetToken(index)
    - Parser.parse(token, tree)

    - SemAzer.check(tree) // variable was declared
    // this will need to check where are we in tree and chcek stuff according to where we are in tree (like if we are after * it should expect number and if there is bracket check brackets)

3. Generate code


# Q

-ch line is a se eaparate tree and then always append it to root or store in array [to one big root tree]

- either the parser (SynA) “pulls” tokens and decides when a tree/statement ends, or the lexer signals logical boundaries like “end of statement/block.” Let’s break it down carefully.

- kedy block???

- tabulka so scopom