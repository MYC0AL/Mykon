// If INTER_MODULE_OPT macro is defined,
// this file is included into cpu.cpp
// to make inter module optimizations possible
#ifndef INTER_MODULE_OPT

#include "mem.h"

#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "interrupt.h"
#include "lcd.h"
#include "mbc.h"
#include "rom.h"
#include "sdl.h"
#include "timer.h"

static unsigned char *mem;
static int DMA_pending = 0;
static int joypad_select_buttons, joypad_select_directions;
static uint32_t bank_switches = 0;

/* Points directly into the ROM buffer (PSRAM) at the start of the
 * currently-selected switchable bank (0x4000-0x7FFF window). Bank
 * switches just repoint this instead of memcpy'ing 16KB out of PSRAM
 * into mem[], which was extremely expensive (PSRAM access latency). */
static const unsigned char *current_bank_ptr = nullptr;

uint32_t mem_get_bank_switches() { return bank_switches; }

void mem_bank_switch(unsigned int n) {
  const unsigned char *b = rom_getbytes();
  bank_switches++;

  current_bank_ptr = &b[n * 0x4000];
}

/* LCD's access to VRAM */
const unsigned char *mem_get_raw() { return mem; }

/* Single point of truth for a "plain" memory read (no DMA lockdown
 * handling), aware of the banked ROM window. */
static inline unsigned char mem_read_raw(unsigned short i) {
  if (i >= 0x4000 && i < 0x8000) {
    return current_bank_ptr[i - 0x4000];
  }
  return mem[i];
}

unsigned char IRAM_ATTR mem_get_byte(unsigned short i) {
  unsigned long elapsed;
  unsigned char mask = 0;

  if (DMA_pending && i < 0xFF80) {
    elapsed = cpu_get_cycles() - DMA_pending;
    if (elapsed >= 160)
      DMA_pending = 0;
    else {
      return mem[0xFE00 + elapsed];
    }
  }

  if (i < 0xFF00) return mem_read_raw(i);

  switch (i) {
    case 0xFF00: /* Joypad */
      if (!joypad_select_buttons) mask = sdl_get_buttons();
      if (!joypad_select_directions) mask = sdl_get_directions();
      return 0xC0 | (0xF ^ mask) |
             (joypad_select_buttons | joypad_select_directions);
      break;
    case 0xFF04:
      return timer_get_div();
      break;
    case 0xFF05:
      return timer_get_counter();
      break;
    case 0xFF06:
      return timer_get_modulo();
      break;
    case 0xFF07:
      return timer_get_tac();
      break;
    case 0xFF0F:
      return interrupt_get_IF();
      break;
    case 0xFF41:
      return lcd_get_stat();
      break;
    case 0xFF44:
      return lcd_get_line();
      break;
    case 0xFF4D: /* GBC speed switch */
      return 0xFF;
      break;
    case 0xFFFF:
      return interrupt_get_mask();
      break;
  }

  return mem[i];
}

unsigned short mem_get_word(unsigned short i) {
  unsigned long elapsed;

  if (DMA_pending && i < 0xFF80) {
    elapsed = cpu_get_cycles() - DMA_pending;
    if (elapsed >= 160)
      DMA_pending = 0;
    else {
      return mem[0xFE00 + elapsed];
    }
  }
  return mem_read_raw(i) | (mem_read_raw(i + 1) << 8);
}

void IRAM_ATTR mem_write_byte(unsigned short d, unsigned char i) {
  unsigned int filtered = 0;

  switch (rom_get_mapper()) {
    case NROM:
      if (d < 0x8000) filtered = 1;
      break;
    case MBC2:
    case MBC3:
      filtered = MBC3_write_byte(d, i);
      break;
    case MBC1:
      filtered = MBC1_write_byte(d, i);
      break;
  }

  if (filtered) return;

  switch (d) {
    case 0xFF00: /* Joypad */
      joypad_select_buttons = i & 0x20;
      joypad_select_directions = i & 0x10;
      break;
    case 0xFF01: /* Link port data */
                 //			fprintf(stderr, "%c", i);
      break;
    case 0xFF04:
      timer_set_div(i);
      break;
    case 0xFF05:
      timer_set_counter(i);
      break;
    case 0xFF06:
      timer_set_modulo(i);
      break;
    case 0xFF07:
      timer_set_tac(i);
      break;
    case 0xFF0F:
      interrupt_set_IF(i);
      break;
    case 0xFF40:
      lcd_write_control(i);
      break;
    case 0xFF41:
      lcd_write_stat(i);
      break;
    case 0xFF42:
      lcd_write_scroll_y(i);
      break;
    case 0xFF43:
      lcd_write_scroll_x(i);
      break;
    case 0xFF45:
      lcd_set_ly_compare(i);
      break;
    case 0xFF46: /* OAM DMA */
      /* Copy bytes from i*0x100 to OAM. The source page can legitimately
       * fall inside the banked ROM window (0x4000-0x7FFF), which is no
       * longer a physical copy in mem[] (see current_bank_ptr /
       * mem_bank_switch) - read through mem_read_raw() byte-by-byte
       * instead of memcpy'ing straight out of mem[] to stay correct. */
      {
        unsigned short src_base = i * 0x100;
        for (unsigned short off = 0; off < 0xA0; ++off) {
          mem[0xFE00 + off] = mem_read_raw(src_base + off);
        }
      }
      DMA_pending = cpu_get_cycles();
      break;
    case 0xFF47:
      lcd_write_bg_palette(i);
      break;
    case 0xFF48:
      lcd_write_spr_palette1(i);
      break;
    case 0xFF49:
      lcd_write_spr_palette2(i);
      break;
    case 0xFF4A:
      lcd_set_window_y(i);
      break;
    case 0xFF4B:
      lcd_set_window_x(i);
      break;
    case 0xFFFF:
      interrupt_set_mask(i);
      return;
      break;
  }

  mem[d] = i;
}

void mem_write_word(unsigned short d, unsigned short i) {
  mem[d] = i & 0xFF;
  mem[d + 1] = i >> 8;
}

void gameboy_mem_init(void) {
  const unsigned char *bytes = rom_getbytes();

  if (bytes == nullptr) {
    return;
  }

  mem = (unsigned char *)calloc(1, 0x10000);
  if (mem == nullptr) {
    return;
  }

  /* Bank 0 (0x0000-0x3FFF) is fixed and stays a real copy in mem[],
   * since it's never bank-switched. Bank 1 (0x4000-0x7FFF, the
   * default/initial switchable bank) is now served via pointer
   * straight into the PSRAM ROM buffer instead of being copied -
   * see current_bank_ptr / mem_bank_switch(). */
  memcpy(&mem[0x0000], &bytes[0x0000], 0x4000);
  current_bank_ptr = &bytes[0x4000];

  mem[0xFF10] = 0x80;
  mem[0xFF11] = 0xBF;
  mem[0xFF12] = 0xF3;
  mem[0xFF14] = 0xBF;
  mem[0xFF16] = 0x3F;
  mem[0xFF19] = 0xBF;
  mem[0xFF1A] = 0x7F;
  mem[0xFF1B] = 0xFF;
  mem[0xFF1C] = 0x9F;
  mem[0xFF1E] = 0xBF;
  mem[0xFF20] = 0xFF;
  mem[0xFF23] = 0xBF;
  mem[0xFF24] = 0x77;
  mem[0xFF25] = 0xF3;
  mem[0xFF26] = 0xF1;
  mem[0xFF40] = 0x91;
  mem[0xFF47] = 0xFC;
  mem[0xFF48] = 0xFF;
  mem[0xFF49] = 0xFF;
}

#endif  // INTER_MODULE_OPT