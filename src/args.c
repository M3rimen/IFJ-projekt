// main function arguments handler

typedef struct Args {
    char* src_file_path;
} Args;

Args handle_args(int argc, char* argv[]) {
    Args args;
    if (argc < 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        exit(1);
    }
    args.src_file_path = argv[1];
    return args;
}