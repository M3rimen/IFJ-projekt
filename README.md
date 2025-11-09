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

- each line is a separate tree and then always append it to root or store in array

- either the parser (SynA) “pulls” tokens and decides when a tree/statement ends, or the lexer signals logical boundaries like “end of statement/block.” Let’s break it down carefully.

- flow??
```c
Token token;
while ((token = LA_next_token(&lexer)).type != TOKEN_EOF) {
    SynA_feed_token(&parser, token);

    if (SynA_block_complete(&parser)) {
        Tree* tree = SynA_build_tree(&parser);
        SemA_validate(&sem_ctx, tree);
        trees_add(&tree_list, tree);
        SynA_reset_for_next_block(&parser);
    }
}

// Handle any remaining partial tree at EOF
if (SynA_has_partial_tree(&parser)) {
    Tree* last_tree = SynA_build_tree(&parser);
    SemA_validate(&sem_ctx, last_tree);
    trees_add(&tree_list, last_tree);
}
```