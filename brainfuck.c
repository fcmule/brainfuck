#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// The language design requires the memory to be a one-dimensional array of
// at least 30,000 byte cells
#define PROGRAM_DATA_SIZE (1 << 15)

typedef struct {
    uint64_t size;
    uint8_t *buffer;
} FileContent;

typedef struct {
    uint64_t instruction_idx;
    uint64_t data_idx;
    FileContent src_file_content;
    uint8_t *data;
} Program;

typedef enum {
    ProgramState_Done,
    ProgramState_Running
} ProgramState;

static uint64_t get_file_size(FILE *file) {
    if (file == NULL) { return 0; }
    fseek(file, 0, SEEK_END);
    uint64_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

static FileContent read_entire_file(char *file_path) {
    FileContent file_content;
    FILE *file = fopen(file_path, "rb");
    file_content.size = get_file_size(file);
    if (file != NULL) {
        file_content.buffer = malloc(file_content.size);
        // The entire content of the file is loaded into RAM
        fread(file_content.buffer, file_content.size, 1, file);
        fclose(file);
    } else {
        fprintf(stderr, "Could not read file at path: %s\n", file_path);
        file_content.buffer = NULL;
    }
    return file_content;
}

static Program init_program(FileContent src_file_content) {
    Program program;
    program.instruction_idx = 0;
    // The language design requires the memory to be initialized to zero
    program.data = malloc(PROGRAM_DATA_SIZE);
    for (uint64_t i = 0; i < PROGRAM_DATA_SIZE; i++) {
        program.data[i] = 0;
    }
    // The movable data pointer is initialized to point to the leftmost byte of
    // the array
    program.data_idx = 0;
    program.src_file_content = src_file_content;
    return program;
}

static inline char get_current_command(Program *program) {
    return (char)(program->src_file_content.buffer[program->instruction_idx]);
}

static inline bool has_executed_last_command(Program *program) {
    return program->instruction_idx >= program->src_file_content.size;
}

static ProgramState parse_and_execute_command(Program *program) {
    ProgramState state = ProgramState_Running;
    char command = get_current_command(program);

    switch (command) {
        case '>':
            // The language design does not specify what to do if the data
            // index exceeds the data size. Let's suppose is wraps around
            program->data_idx++;
            program->data_idx %= PROGRAM_DATA_SIZE;
            break;
        case '<':
            // The language design does not specify what to do if the data
            // index subceeds the data size. Let's suppose is wraps around
            if (program->data_idx == 0) {
                program->data_idx = PROGRAM_DATA_SIZE;
            }
            program->data_idx--;
            break;
        case '+':
            program->data[program->data_idx]++;
            break;
        case '-':
            program->data[program->data_idx]--;
            break;
        case '.':
            printf("%c", (char)(program->data[program->data_idx]));
            break;
        case ',': {
            // For simplicity, the input stream is stdin
            char c = getchar();
            if (c == EOF) { c = 0; }
            program->data[program->data_idx] = (uint8_t)c;
            break;
        }
        case '[':
            if (program->data[program->data_idx] == 0) {
                uint64_t count = 1;
                while (count) {
                    program->instruction_idx++;
                    if (has_executed_last_command(program)) {
                        fprintf(stderr, "Could not find loop end\n");
                        break;
                    }
                    command = get_current_command(program);
                    count += command == '[';
                    count -= command == ']';
                }
            }
            break;
        case ']':
            if (program->data[program->data_idx] != 0) {
                uint64_t count = 1;
                while (count) {
                    program->instruction_idx--;
                    if (program->instruction_idx < 0) {
                        fprintf(stderr, "Could not find loop start\n");
                        break;
                    }
                    command = get_current_command(program);
                    count += command == ']';
                    count -= command == '[';
                }
            }
            break;
        default:
            break;
    }

    program->instruction_idx++;
    state = has_executed_last_command(program) ? ProgramState_Done : state;

    return state;
}

static void execute(Program *program) {
    ProgramState state = ProgramState_Running;
    while (state != ProgramState_Done) {
        state = parse_and_execute_command(program);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: brainfuck [src_file_path]\n");
        return 1;
    }

    char *src_file_path = argv[1];

    FileContent src_file_content = read_entire_file(src_file_path);
    Program program = init_program(src_file_content);

    execute(&program);

    return 0;
}
