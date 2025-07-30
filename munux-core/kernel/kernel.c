// kernel.c

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

volatile uint16_t* video_memory = (uint16_t*)0xB8000;
int cursor_position = 0;

void print_char(char c, int pos) {
    // Escreve o caractere na posição pos da tela com cor padrão (0x07 = cinza claro no fundo preto)
    video_memory[pos] = (uint16_t)c | (0x07 << 8);
}

void print_string(const char* str) {
    int i = 0;
    while (str[i] != '\0') {
        print_char(str[i], cursor_position++);
        i++;
    }
}

void kernel_main() {
    char* video = (char*)0xB8000;
    const char* msg = "Bem-vindo ao Munux kernel!";
    for (int i = 0; msg[i] != 0; i++) {
        video[i*2] = msg[i];
        video[i*2+1] = 0x07;
    }
    while (1) {}
}

