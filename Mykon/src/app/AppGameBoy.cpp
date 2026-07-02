/****************************************************
 * AppGameBoy.cpp
 * 
 * The Game Boy Application
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "app/AppGameBoy.h"

/**********************
 * Defines
 **********************/
//#define PERF_REPORT
#define REPORT_INTERVAL 60

/**********************
 * Function Prototypes
 **********************/
static mk_err_t send_io_to_sdl( ntfy_app_t8 io_notif );
static mk_err_t load_rom_sd_to_psram( const char *filename, uint8_t **rom, int32_t *size );

/**********************
 * Variables
 **********************/
static constexpr uint32_t emulator_cpu_freq   = 4200000 / 4;
static constexpr uint32_t frames_per_sec      = 60;
static           uint32_t cpu_freq            = 0;
static           uint32_t cycles_per_frame    = 0;
static           uint32_t cycles_in_micro_sec = 0;

static           uint8_t* actv_rom            = nullptr;

/**********************
 * Functions
 **********************/

 /***************************************************
 * GameBoy_setup()
 * 
 * Description: Setup the Game Boy application
 **************************************************/
void GameBoy_setup( )
{
    /* Load ROM from SD card to PSRAM */
    mk_err_t rom_load_err = load_rom_sd_to_psram( GB_POKEMON_RED_ROM, &actv_rom, nullptr );
    if ( rom_load_err != ERR_NONE || actv_rom == nullptr )
    {
        Serial.printf("GameBoy: ROM load failed with error %d\n", rom_load_err);
        return;
    }

    if ( !rom_init(actv_rom) )
    {
        Serial.println("GameBoy: Invalid ROM header");
        return;
    }
    Serial.printf("GameBoy: ROM initialized\n");

    sdl_init();

    gameboy_mem_init();
    if ( mem_get_raw() == nullptr )
    {
        Serial.println("GameBoy: Emulator memory initialization failed");
        return;
    }
    Serial.printf("GameBoy: Memory initialized\n");

    cpu_init();
    Serial.printf("GameBoy: CPU initialized\n");

    cpu_freq = getCpuFrequencyMhz();
    Serial.printf("CPU Freq = %u Mhz\n", cpu_freq);
    cpu_freq *= 1000000;

    cycles_per_frame = cpu_freq / frames_per_sec;
    cycles_in_micro_sec = cpu_freq / 1000000;
    Serial.printf("cycles_per_frame %d cycles_in_micro_sec %d\n", cycles_per_frame, cycles_in_micro_sec);
}

/***************************************************
 * GameBoy_run()
 * 
 * Description: Run the Game Boy application
 **************************************************/
void GameBoy_run( void * pvParameters )
{
    Serial.println("GameBoy: Application Started");

    /* Suspend self on startup */
    vTaskSuspend( NULL );

    /* Local variables*/
    ntfy_app_t8 tsk_notifs = NTFY_NONE;
    bool app_started = false;

    /* Control loop */
    while( 1 )
    {
        /* Check task notifications */
        if ( xTaskNotifyWait( 0, 0, &tsk_notifs, 0 ) == pdTRUE )
        {
            switch( tsk_notifs )
            {
                case NTFY_SETUP:
                    GameBoy_setup();
                    break;

                case NTFY_STRT:
                    app_started = true;
                    break;

                case NTFY_IO_JYSTCK_UP:
                case NTFY_IO_JYSTCK_DOWN:
                case NTFY_IO_JYSTCK_LEFT:
                case NTFY_IO_JYSTCK_RIGHT:
                case NTFY_IO_JYSTCK_CENTER:
                case NTFY_IO_BTN_JYSTCK:
                    send_io_to_sdl( tsk_notifs );
                    break;

                default:
                    break;
            }
        }

        /* GameBoy Started */
        if ( app_started )
        {
            bool screen_updated = false;

            #ifdef PERF_REPORT
            uint32_t loop_start = ESP.getCycleCount();
            uint32_t adjust = 100;
            for (int i = 0; i < 3 && adjust >= 100; ++i) {
                uint32_t adjust_start = ESP.getCycleCount();
                NOP();
                uint32_t adjust_end = ESP.getCycleCount();
                // -1 to compensate nop
                adjust = adjust_end - adjust_start - 1;
            }
            assert(adjust < 100);

            static uint32_t prev_loop_exit = 0;
            static int frames_count = 0;
            /* uint64_t: at ~12M cycles/frame observed (the emulator is
             * currently running ~3x over its 60fps cycle budget), a
             * REPORT_INTERVAL-frame cumulative total can legitimately
             * approach/exceed 1e9. uint32_t left no real headroom for
             * genuine (if slow) performance data. */
            static uint64_t total_cpu = 0;
            static uint64_t total_lcd = 0;
            static uint64_t total_sdl = 0;
            static uint64_t total_timer = 0;
            static uint64_t total_delay = 0;
            static uint64_t total_outside_loop = 0;
            static int sdl_count = 0;
            static uint32_t emulator_cpu_cycle_begin = 0;
            static uint64_t opcode_profile[256];
            static uint64_t opcode_calls[256];
            static int sample_no = 0;
            uint32_t start_bank_switches = mem_get_bank_switches();
            static uint64_t frame_cycles[REPORT_INTERVAL] = {};
            static int bank_switches[REPORT_INTERVAL] = {};
            #endif
            uint32_t start_frame_cycle = ESP.getCycleCount();
            uint32_t emulator_cpu_cycle = 0;

            while (!screen_updated)
            {
            #ifdef PERF_REPORT
                auto pc = cpu_get_pc();
                unsigned char opcode = mem_get_byte(pc);

                uint32_t cpu_start = ESP.getCycleCount();
            #endif

                /* Run one CPU cycle */
                emulator_cpu_cycle = cpu_cycle();

            #ifdef PERF_REPORT
                uint32_t lcd_start = ESP.getCycleCount();
                uint32_t cpu_end = lcd_start;
            #endif
                /* Run one LCD cycle */
                screen_updated = lcd_cycle(emulator_cpu_cycle);


            #ifdef PERF_REPORT
                uint32_t timer_start = ESP.getCycleCount();
                uint32_t lcd_end = timer_start;
            #endif

                /* Run one timer cycle */
                timer_cycle(emulator_cpu_cycle);

            #ifdef PERF_REPORT
                uint32_t timer_end = ESP.getCycleCount();

                /* Guard against unsigned underflow: if the measured delta is
                 * smaller than the calibration overhead (adjust), we're at or
                 * below the instrumentation's noise floor. Clamp to 0 instead
                 * of wrapping around to a huge unsigned value. */
                uint32_t cpu_raw_delta = cpu_end - cpu_start;
                uint32_t cpu_elapsed = (cpu_raw_delta > adjust) ? (cpu_raw_delta - adjust) : 0;
                total_cpu += cpu_elapsed;
                if (cpu_raw_delta > adjust && cpu_elapsed > 1000000) {
                printf("cpu timer seems incorrect:\n    end %u, start %u, adjust %u\n",
                        cpu_end, cpu_start, adjust);
                }

                uint32_t lcd_raw_delta = lcd_end - lcd_start;
                uint32_t lcd_elapsed = (lcd_raw_delta > adjust) ? (lcd_raw_delta - adjust) : 0;
                total_lcd += lcd_elapsed;

                uint32_t timer_raw_delta = timer_end - timer_start;
                uint32_t timer_elapsed = (timer_raw_delta > adjust) ? (timer_raw_delta - adjust) : 0;
                total_timer += timer_elapsed;

                opcode_profile[opcode] += cpu_elapsed;
                opcode_calls[opcode] += 1;
            #endif
            }

            #ifdef PERF_REPORT
            uint32_t sdl_start = ESP.getCycleCount();
            #endif
            /* Update the display */
            xTaskNotifyGive( sdl_get_draw_task_handle( ) );
            #ifdef PERF_REPORT
            uint32_t sdl_end = ESP.getCycleCount();
            uint32_t delay_start = sdl_end;
            #endif

            /* Delay until the next frame */
            uint32_t end_frame_cycle = ESP.getCycleCount();
            uint32_t cycles_delta = end_frame_cycle - start_frame_cycle;

            if (cycles_delta < cycles_per_frame)
            {
                Serial.printf("GameBoy: Frame took %d cycles, delaying for %d cycles\n", cycles_delta, cycles_per_frame - cycles_delta);
                vTaskDelay( pdMS_TO_TICKS( ( cycles_per_frame - cycles_delta ) / cycles_in_micro_sec ) );
                //delayMicroseconds(cycles_delta / cycles_in_micro_sec);
            }

            #ifdef PERF_REPORT
            uint32_t delay_end = ESP.getCycleCount();

            /* Skip on the very first pass: prev_loop_exit is still its
             * static-init value of 0, so loop_start - prev_loop_exit would
             * be the raw cycle count since boot (huge), not a real
             * "outside loop" measurement. */
            if (prev_loop_exit != 0) {
                uint32_t outside_loop_raw_delta = loop_start - prev_loop_exit;
                uint32_t outside_loop_elapsed =
                    (outside_loop_raw_delta > adjust) ? (outside_loop_raw_delta - adjust) : 0;
                total_outside_loop += outside_loop_elapsed;
            }

            uint32_t sdl_raw_delta = sdl_end - sdl_start;
            uint32_t sdl_elapsed = (sdl_raw_delta > adjust) ? (sdl_raw_delta - adjust) : 0;
            total_sdl += sdl_elapsed;

            uint32_t delay_raw_delta = delay_end - delay_start;
            uint32_t delay_elapsed = (delay_raw_delta > adjust) ? (delay_raw_delta - adjust) : 0;
            total_delay += delay_elapsed;

            frame_cycles[frames_count] =
                total_delay + total_timer + total_sdl + total_lcd + total_cpu;
            /* Sanity ceiling only, not a performance target: guards against
             * genuine corruption/wraparound, not against the emulator
             * legitimately running slower than its frame budget. */
            assert(frame_cycles[frames_count] < 100000000000ULL);
            bank_switches[frames_count] = mem_get_bank_switches() - start_bank_switches;

            frames_count += 1;
            sdl_count += 1;
            if (frames_count >= REPORT_INTERVAL) {
                uint64_t min_cycles_per_frame = frame_cycles[0];
                uint64_t max_cycles_per_frame = frame_cycles[0];
                uint64_t avg_cycles_per_frame = 0;
                int total_bank_switches = 0;
                /* i starts at 1: frame_cycles[i - 1] is undefined for i == 0
                 * (reads before the array). Frame 0's per-frame delta is
                 * just frame_cycles[0] itself (delta from an implicit 0). */
                min_cycles_per_frame = frame_cycles[0];
                max_cycles_per_frame = frame_cycles[0];
                total_bank_switches += bank_switches[0];
                for (int i = 1; i < REPORT_INTERVAL; ++i) {
                min_cycles_per_frame =
                    std::min(min_cycles_per_frame, frame_cycles[i] - frame_cycles[i - 1]);
                max_cycles_per_frame =
                    std::max(max_cycles_per_frame, frame_cycles[i] - frame_cycles[i - 1]);
                total_bank_switches += bank_switches[i];
                }
                avg_cycles_per_frame = frame_cycles[REPORT_INTERVAL - 1];
                if (avg_cycles_per_frame > 100000000000ULL) {
                printf("avg_cycles_per_frame look incorrect\n  frame cycles: ");
                for (int j = 0; j < REPORT_INTERVAL; ++j) printf(" %llu,", (unsigned long long)frame_cycles[j]);
                }
                assert(avg_cycles_per_frame < 100000000000ULL);
                avg_cycles_per_frame /= frames_count;

                assert(sdl_count == frames_count);
                printf("sample no: %d\n", sample_no);
                printf("cpu avg: %llu\n", (unsigned long long)(total_cpu / frames_count));
                printf("lcd avg: %llu\n", (unsigned long long)(total_lcd / frames_count));
                printf("sdl avg: %llu\n", (unsigned long long)(total_sdl / sdl_count));
                printf("timer avg: %llu\n", (unsigned long long)(total_timer / frames_count));
                printf("delay avg: %llu\n", (unsigned long long)(total_delay / frames_count));
                printf("outside loop avg: %llu\n", (unsigned long long)(total_outside_loop / frames_count));
                uint64_t host_cycles = total_cpu + total_lcd + total_sdl + total_timer;
                uint32_t emulated_cycles = emulator_cpu_cycle - emulator_cpu_cycle_begin;
                float perf_ratio =
                    ((float)emulator_cpu_freq / cpu_freq) * host_cycles / emulated_cycles;
                printf("emulator/real hardware ratio: %f\n", perf_ratio);
                printf("emulated cycles: %d\n", emulated_cycles);
                printf("average cycles per frame: %llu\n", (unsigned long long)avg_cycles_per_frame);
                printf("min cycles per frame: %llu\n", (unsigned long long)min_cycles_per_frame);
                printf("max cycles per frame: %llu\n", (unsigned long long)max_cycles_per_frame);
                printf("bank switches: %d\n", total_bank_switches);

                int longest_opcode = 0;
                uint64_t opcode_cycles = opcode_profile[0];
                int most_frequent_opcode = 0;
                uint64_t opcode_call_count = opcode_calls[0];
                for (int i = 0; i < 256; ++i) {
                if (opcode_profile[i] > opcode_cycles) {
                    opcode_cycles = opcode_profile[i];
                    longest_opcode = i;
                }
                if (opcode_calls[i] > opcode_call_count) {
                    opcode_call_count = opcode_calls[i];
                    most_frequent_opcode = i;
                }
                }
                uint64_t longest_opcode_calls = opcode_calls[longest_opcode];
                double longest_opcode_avg = longest_opcode_calls
                    ? (double)opcode_cycles / (double)longest_opcode_calls
                    : 0.0;
                uint64_t most_frequent_opcode_cycles = opcode_profile[most_frequent_opcode];
                double most_frequent_opcode_avg = opcode_call_count
                    ? (double)most_frequent_opcode_cycles / (double)opcode_call_count
                    : 0.0;
                printf("longest opcode: %d, took %llu cycles over %llu calls (%.2f avg cycles/call)\n",
                    longest_opcode, (unsigned long long)opcode_cycles,
                    (unsigned long long)longest_opcode_calls, longest_opcode_avg);
                printf("most frequent opcode: %d, called %llu times, took %llu cycles (%.2f avg cycles/call)\n\n",
                    most_frequent_opcode, (unsigned long long)opcode_call_count,
                    (unsigned long long)most_frequent_opcode_cycles, most_frequent_opcode_avg);

                for (int i = 0; i < 256; ++i) {
                opcode_profile[i] = 0;
                opcode_calls[i] = 0;
                }

                frames_count = 0;
                sdl_count = 0;
                emulator_cpu_cycle_begin = emulator_cpu_cycle;
                total_cpu = total_lcd = total_timer = total_sdl = total_delay =
                    total_outside_loop = 0;
                sample_no++;
            }
            prev_loop_exit = ESP.getCycleCount();
            #endif
        }
    }
}

/***************************************************
 * load_rom_sd_to_psram()
 *
 * Description: Load ROM from SD card to PSRAM
 **************************************************/
static mk_err_t load_rom_sd_to_psram( const char *filename, uint8_t **rom, int32_t *size )
{
    /* Local variables */
    File gb_rom;
    int32_t gb_rom_size = 0;
    mk_err_t err = ERR_NONE;

    *rom = nullptr;
    if ( size )
    {
        *size = 0;
    }

    if ( SD_getFile( &gb_rom, filename, &gb_rom_size ) == ERR_NONE )
    {
        Serial.printf("GameBoy: ROM file size %d bytes\n", gb_rom_size);
    }
    else 
    {
        Serial.printf("GameBoy: Error: \"%s\" not found!\r\n", filename );
        return ERR_FILE_NOT_FOUND;
    }

    if ( gb_rom_size <= 0 )
    {
        Serial.printf("Gameboy: Error: Invalid ROM size for \"%s\"\r\n", filename );
        return ERR_GNRL;
    }

    /* Allocate PSRAM memory for the ROM */
    *rom = (uint8_t *)ps_malloc(gb_rom_size);
    if ( *rom == nullptr )
    {
        Serial.printf("GameBoy: Error: Failed to allocate %d bytes for ROM\r\n", gb_rom_size);
        return ERR_GNRL;
    }

    if ( SD_readFile( &gb_rom, *rom, gb_rom_size ) == ERR_NONE )
    {
        Serial.printf("GameBoy: ROM file read complete\n");
    }
    else
    {
        Serial.printf("Gameboy: Error: Failed to read \"%s\"\r\n", filename );
        err = ERR_GNRL;
    }

    if ( size )
    {
        *size = gb_rom_size;
    }

    return err;
}

/***************************************************
 * send_io_to_sdl()
 *
 * Description: Send IO notification to SDL
 **************************************************/
mk_err_t send_io_to_sdl( ntfy_app_t8 io_notif )
{
    /* Local variables */
    Gameboy_Buttons_s gb_buttons;
    mk_err_t err;

    /* Initialize Local Variables */
    err = ERR_NONE;
    memset( &gb_buttons, 0, sizeof( gb_buttons ) );

    /* Map IO Notifcation to Gameboy buttons */
    if ( io_notif >= NTFY_IO_FIRST && io_notif <= NTFY_IO_LAST )
    {
        switch( io_notif )
        {
            case NTFY_IO_JYSTCK_UP:
                gb_buttons.up = 1;
                break;

            case NTFY_IO_JYSTCK_DOWN:
                gb_buttons.down = 1;
                break;

            case NTFY_IO_JYSTCK_LEFT:
                gb_buttons.left = 1;
                break;

            case NTFY_IO_JYSTCK_RIGHT:
                gb_buttons.right = 1;
                break;

            case NTFY_IO_BTN_JYSTCK:
                gb_buttons.start = 1;
                break;

            default:
                err = ERR_GNRL;
        }

        sdl_set_buttons( &gb_buttons );
    }
    else
    {
        err = ERR_GNRL;
    }

    return err;
}