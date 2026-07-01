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
static mk_err_t load_rom_sd_to_psram( const char *filename, uint8_t **rom, int32_t *size );

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
        uint32_t t1 = ESP.getCycleCount();
        /* Check task notifications */
        if ( xTaskNotifyWait( 0, 0, &tsk_notifs, 0 ) == pdTRUE )
        {
            switch( tsk_notifs )
            {
                case NTFY_SETUP:
                    GameBoy_setup();
                    vTaskDelay( pdMS_TO_TICKS( 500 ) );
                    break;

                case NTFY_STRT:
                    app_started = true;
                    break;
            }
        }
        uint32_t t2 = ESP.getCycleCount();
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
            static uint32_t total_cpu = 0;
            static uint32_t total_lcd = 0;
            static uint32_t total_sdl = 0;
            static uint32_t total_timer = 0;
            static uint32_t total_delay = 0;
            static uint32_t total_outside_loop = 0;
            static int sdl_count = 0;
            static uint32_t emulator_cpu_cycle_begin = 0;
            static int opcode_profile[256];
            static int sample_no = 0;
            uint32_t start_bank_switches = mem_get_bank_switches();
            static uint32_t frame_cycles[REPORT_INTERVAL] = {};
            static int bank_switches[REPORT_INTERVAL] = {};
            #endif
            uint32_t start_frame_cycle = ESP.getCycleCount();
            uint32_t emulator_cpu_cycle = 0;

            uint32_t su1, su2, su3, su4;

            uint32_t t3 = ESP.getCycleCount();

            while (!screen_updated)
            {
            #ifdef PERF_REPORT
                auto pc = cpu_get_pc();
                unsigned char opcode = mem_get_byte(pc);

                uint32_t cpu_start = ESP.getCycleCount();
            #endif

                su1 = ESP.getCycleCount();
                /* Run one CPU cycle */
                emulator_cpu_cycle = cpu_cycle();

            #ifdef PERF_REPORT
                uint32_t lcd_start = ESP.getCycleCount();
                uint32_t cpu_end = lcd_start;
            #endif
                su2 = ESP.getCycleCount();
                /* Run one LCD cycle */
                screen_updated = lcd_cycle(emulator_cpu_cycle);


            #ifdef PERF_REPORT
                uint32_t timer_start = ESP.getCycleCount();
                uint32_t lcd_end = timer_start;
            #endif

                su3 = ESP.getCycleCount();
                /* Run one timer cycle */
                timer_cycle(emulator_cpu_cycle);

                su4 = ESP.getCycleCount();

            #ifdef PERF_REPORT
                uint32_t timer_end = ESP.getCycleCount();

                total_cpu += cpu_end - cpu_start - adjust;
                if (cpu_end - cpu_start - adjust > 1000000) {
                printf("cpu timer seems incorrect:\n    end %u, start %u, adjust %u\n",
                        cpu_end, cpu_start, adjust);
                }
                total_lcd += lcd_end - lcd_start - adjust;
                total_timer += timer_end - timer_start - adjust;
                opcode_profile[opcode] += cpu_end - cpu_start - adjust;
            #endif
            }

            uint32_t t4 = ESP.getCycleCount();
            #ifdef PERF_REPORT
            uint32_t sdl_start = ESP.getCycleCount();
            #endif
            /* Update the display */
            xTaskNotifyGive( sdl_get_draw_task_handle( ) );
            #ifdef PERF_REPORT
            uint32_t sdl_end = ESP.getCycleCount();
            uint32_t delay_start = sdl_end;
            #endif

            uint32_t t5 = ESP.getCycleCount();
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

            total_outside_loop += loop_start - prev_loop_exit - adjust;

            total_sdl += sdl_end - sdl_start - adjust;
            total_delay += delay_end - delay_start - adjust;
            frame_cycles[frames_count] =
                total_delay + total_timer + total_sdl + total_lcd + total_cpu;
            assert(frame_cycles[frames_count] < 1000000000);
            bank_switches[frames_count] = mem_get_bank_switches() - start_bank_switches;

            frames_count += 1;
            sdl_count += 1;
            if (frames_count >= REPORT_INTERVAL) {
                uint32_t min_cycles_per_frame = frame_cycles[0];
                uint32_t max_cycles_per_frame = frame_cycles[0];
                uint32_t avg_cycles_per_frame = 0;
                int total_bank_switches = 0;
                for (int i = 0; i < REPORT_INTERVAL; ++i) {
                min_cycles_per_frame =
                    std::min(min_cycles_per_frame, frame_cycles[i] - frame_cycles[i - 1]);
                max_cycles_per_frame =
                    std::max(max_cycles_per_frame, frame_cycles[i] - frame_cycles[i - 1]);
                total_bank_switches += bank_switches[i];
                }
                avg_cycles_per_frame = frame_cycles[REPORT_INTERVAL - 1];
                if (avg_cycles_per_frame > 1000000000) {
                printf("avg_cycles_per_frame look incorrect\n  frame cycles: ");
                for (int j = 0; j < REPORT_INTERVAL; ++j) printf(" %u,", frame_cycles[j]);
                }
                assert(avg_cycles_per_frame < 1000000000);
                avg_cycles_per_frame /= frames_count;

                assert(sdl_count == frames_count);
                printf("sample no: %d\n", sample_no);
                printf("cpu avg: %d\n", total_cpu / frames_count);
                printf("lcd avg: %d\n", total_lcd / frames_count);
                printf("sdl avg: %d\n", total_sdl / sdl_count);
                printf("timer avg: %d\n", total_timer / frames_count);
                printf("delay avg: %d\n", total_delay / frames_count);
                printf("outside loop avg: %d\n", total_outside_loop / frames_count);
                uint32_t host_cycles = total_cpu + total_lcd + total_sdl + total_timer;
                uint32_t emulated_cycles = emulator_cpu_cycle - emulator_cpu_cycle_begin;
                float perf_ratio =
                    ((float)emulator_cpu_freq / cpu_freq) * host_cycles / emulated_cycles;
                printf("emulator/real hardware ratio: %f\n", perf_ratio);
                printf("emulated cycles: %d\n", emulated_cycles);
                printf("average cycles per frame: %d\n", avg_cycles_per_frame);
                printf("min cycles per frame: %d\n", min_cycles_per_frame);
                printf("max cycles per frame: %d\n", max_cycles_per_frame);
                printf("bank switches: %d\n", total_bank_switches);

                int longest_opcode = 0;
                int opcode_cycles = opcode_profile[0];
                for (int i = 0; i < sizeof(opcode_profile) / sizeof(int); ++i) {
                if (opcode_profile[i] > opcode_cycles) {
                    opcode_cycles = opcode_profile[i];
                    longest_opcode = i;
                }
                opcode_profile[i] = 0;
                }
                printf("longest opcode: %d, took %d cycles\n\n", longest_opcode,
                    opcode_cycles);

                frames_count = 0;
                sdl_count = 0;
                emulator_cpu_cycle_begin = emulator_cpu_cycle;
                total_cpu = total_lcd = total_timer = total_sdl = total_delay =
                    total_outside_loop = 0;
                sample_no++;
            }
            prev_loop_exit = ESP.getCycleCount();
            #endif

            uint32_t t6 = ESP.getCycleCount();

            // printf("GameBoy: Loop times: t2-t1 %d, t3-t2 %d, t4-t3 %d, t5-t4 %d, t6-t5 %d\n",
            //                             t2 - t1, t3 - t2, t4 - t3, t5 - t4, t6 - t5);

            printf("GameBoy: Loop times: su2-su1 %d, su3-su2 %d, su4-su3 %d\n",
                                su2-su1,    su3-su2,    su4-su3);
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
