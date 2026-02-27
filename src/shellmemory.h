#define MEM_SIZE 1000
#define MAX_LINES 1000
#define MAX_LINE_SIZE 100
extern char script_memory[MAX_LINES][MAX_LINE_SIZE];
void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
