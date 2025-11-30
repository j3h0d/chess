#include "performance_counters.h"
#include "game_logic.h"
#include "board_definition.h"
#include "jtag_uart.h"

extern uint8_t board[8][8];
extern volatile int *get_vga_control(void);
extern void redraw_board_and_pieces(void);
void jtag_uart_print_performance(const performance_t *p);

void benchmark_logic(void) {
    performance_t p;
    const int loops = 50; // doing the same game logic tests 50 times

    performance_reset();

    for (int i = 0; i < loops; i++) {
        for (int sr = 0; sr < 8; sr++) {
            for (int sf = 0; sf < 8; sf++) {
                uint8_t piece = board[sr][sf];
                if (piece == PIECE_EMPTY) continue;

                for (int dr = 0; dr < 8; dr++) {
                    for (int df = 0; df < 8; df++) {
                        legal_move(sf, sr, df, dr, piece, 1);
                    }
                }
            }
        }
        king_in_check(1);
        king_in_check(0);
        is_checkmate(1);
        is_checkmate(0);
    }

    performance_read(&p);

    jtag_uart_puts("\nLOGIC BENCHMARK:\n");
    jtag_uart_print_performance(&p);
}

void benchmark_render(void) {
    performance_t p;
    const int frames = 10; //drawing the board 10 times

    volatile int *vga = get_vga_control();

    performance_reset();

    for (int i = 0; i < frames; i++) {
        redraw_board_and_pieces();

        vga[VIDEO_DMA_BACKBUFFER] = (int)get_framebuffer();
        vga[VIDEO_DMA_BUFFER] = 0;
    }

    performance_read(&p);

    jtag_uart_puts("\nRENDER BENCHMARK:\n");
    jtag_uart_print_performance(&p);
}

static void print_uint(uint32_t v) { //helper fuction that prints whatever result each counter outputs
    char buf[12]; 
    int i = 0;

    if (v == 0) {
        jtag_uart_putc('0');
        return;
    }

    while (v && i < 11) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i--) {
        jtag_uart_putc(buf[i]);
    }
}

void jtag_uart_print_performance(const performance_t *p) { //main print fuction to print the results

    jtag_uart_puts("cycles: ");    print_uint(p->cycles);    jtag_uart_puts("\n");
    jtag_uart_puts("instret: ");   print_uint(p->ins_ret);   jtag_uart_puts("\n");

    jtag_uart_puts("mem_instr: "); print_uint(p->mem_ins_ret); jtag_uart_puts("\n");
    jtag_uart_puts("ic_miss: ");   print_uint(p->ic_miss);   jtag_uart_puts("\n");
    jtag_uart_puts("dc_miss: ");   print_uint(p->dc_miss);   jtag_uart_puts("\n");

    jtag_uart_puts("ic_stall: ");  print_uint(p->ic_stall);  jtag_uart_puts("\n");
    jtag_uart_puts("dc_stall: ");  print_uint(p->dc_stall);  jtag_uart_puts("\n");
    jtag_uart_puts("hazard: ");    print_uint(p->hazard);    jtag_uart_puts("\n");
    jtag_uart_puts("alu_stall: "); print_uint(p->alu_stall); jtag_uart_puts("\n");

    jtag_uart_puts("\n\n");
}