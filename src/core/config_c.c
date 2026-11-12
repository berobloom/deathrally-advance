#include "drally.h"
#include "drally_keyboard.h"
#include "drally_structs_free.h"
#include "drally_structs_fixed.h"
#include "drmemory.h"
#include "drally_net.h"
#include "drally_career.h"

#define STANDARD_COM1       0
#define STANDARD_COM2       1
#define STANDARD_COM3       2
#define STANDARD_COM4       3
#define CUSTOM_COM_PORT     4

enum e_difficulty {
    SPEED_MAKES_ME_DIZZY,
    I_LIVE_TO_RIDE,
    PETROL_IN_MY_VEINS
};

static void CONFIG_DEFAULT(void);
unsigned GET_FILE_SIZE(const char *);
void dRally_System_clean(void);
int rand_watcom106(void);

#define CONFIG_FILE_NAME "dr.cfg"

extern __BYTE__ CONFIG_SOUND_TYPE;
extern __BYTE__ CONFIG_SOUND_IRQ;
extern __BYTE__ CONFIG_SOUND_DMA;
extern __DWORD__ CONFIG_SOUND_ADDR;
extern __DWORD__ ___24cc54h_sfx_volume;
extern __DWORD__ ___24cc58h_msx_volume;
extern kb_control_t ___1a1140h;
extern __DWORD__ ___1a113ch_gp_brake;
extern __DWORD__ ___1a1130h_gp_steer_right;
extern __DWORD__ ___1a1118h_gp_machine_gun;
extern __DWORD__ ___1a111ch_gp_drop_mine;
extern __DWORD__ ___1a1f3ch_counter;
extern __BYTE__ ___185a5ch[];
extern __DWORD__ ___1a1e50h_com_port_standard;
extern __DWORD__ ___1a1120h_gp_turbo_boost;
extern __DWORD__ ___1a1110h_gp_steer_left;
extern __DWORD__ ___1a1164h_gp_accelerate;
extern hof_entry_t ___1a0e28h[10];
extern record_t ___19f750h[6][18];
extern __DWORD__ ___196a90h_modem_dialing;
extern char ___1a1ffch_modem_init_string[0x15];
extern char ___1a201ah_modem_dial_number[0x15];
extern __DWORD__ ___19bd58h_gamepad;
extern __DWORD__ ___196a94h_difficulty;
extern __BYTE__ ___199f42h[];
extern __BYTE__ ___199f41h[];
extern __BYTE__ ___199f3eh[];
extern __BYTE__ ___199f3fh[];
extern __BYTE__ ___199f43h[];
extern __BYTE__ ___199f44h[];
extern __BYTE__ ___199f45h[];

static __BYTE__ ROL_BYTE(__BYTE__ b, int n){

    return (b<<n)|(b>>(8-n));
}

static __BYTE__ ROR_BYTE(__BYTE__ b, int n){

    return (b>>n)|(b<<(8-n));
}

static void CONFIG_DECODE(config_t * p){

	__UNSIGNED__    n, size;
    __BYTE__        key;

    size = sizeof(p->raw);
    key = p->key;
	n = -1;
	while(++n < size){

		p->raw[n] = key+ROL_BYTE(p->raw[n], n%5);
		key += 0xf4;
	}
}

static void CONFIG_ENCODE(config_t * p){

    __UNSIGNED__    n, size;
    __BYTE__        key;

    size = sizeof(p->raw);
    key = p->key = rand_watcom106();
    n = -1;
    while(++n < size){

        p->raw[n] = ROR_BYTE(p->raw[n]-key, n%5);
        key += 0xf4;
    }
}

static int strisprint(const char* s, size_t len){

    for (size_t i = 0; i < len; i++)
    {
        if (!isprint((unsigned char)s[i]))
            return 0;
    }
    return 1;
}

static int CONFIG_IS_ENCODED(config_t* p){

    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 18; j++)
        {
            record_t* rec = &p->track_records[i][j];
            if (!strisprint(rec->name, strlen(rec->name)))
                return 1;
        }
    }

    for (int i = 0; i < 10; i++)
    {
        hof_entry_t* hof = &p->hall_of_fame[i];
        if (!strisprint(hof->name, strlen(hof->name)))
            return 1;
    }
    return 0;
}

static const __BYTE__ CONFIG_INIT[7] = { 0,0,0,0,0,0,0 };
static int CONFIG_ENCODED = 0;

void CONFIG_READ(void){

    FILE *	    fd;
	__DWORD__ 	    file_size;
    __BYTE__        key;
    config_t *  cfg;

    file_size = GET_FILE_SIZE(CONFIG_FILE_NAME);

    if(file_size == 0){
    
        fd = strupr_fopen(CONFIG_FILE_NAME, "wb");
        file_size = fwrite(CONFIG_INIT, 1, sizeof(CONFIG_INIT), fd);
        fclose(fd);
    }

    fd = strupr_fopen(CONFIG_FILE_NAME, "rb");

    if(file_size == sizeof(CONFIG_INIT)){

        fread(&CONFIG_SOUND_TYPE, sizeof(__BYTE__), 1, fd);
        fread(&CONFIG_SOUND_IRQ, sizeof(__BYTE__), 1, fd);
        fread(&CONFIG_SOUND_DMA, sizeof(__BYTE__), 1, fd);
        fread(&CONFIG_SOUND_ADDR, sizeof(__DWORD__), 1, fd);
        CONFIG_SOUND_ADDR = SDL_SwapLE32(CONFIG_SOUND_ADDR);
        fclose(fd);
        CONFIG_DEFAULT();
    }
    else {

        cfg = (config_t *)dRMemory_alloc(sizeof(config_t));
        fread(cfg, 1, sizeof(config_t), fd);
        fclose(fd);

        CONFIG_SOUND_TYPE = cfg->soundtype;
        CONFIG_SOUND_IRQ = cfg->soundirq;
        CONFIG_SOUND_DMA = cfg->sounddma;
        CONFIG_SOUND_ADDR = SDL_SwapLE32(cfg->soundaddr);

        CONFIG_ENCODED = CONFIG_IS_ENCODED(cfg);
        if (CONFIG_ENCODED) {
            CONFIG_DECODE(cfg);
        }
        
        ___24cc58h_msx_volume = SDL_SwapLE32(cfg->msx_volume);
        ___24cc54h_sfx_volume = SDL_SwapLE32(cfg->sfx_volume);
        D(___185a5ch+7*0x1c+0x18) = SDL_SwapLE32(cfg->com_port_option);
        ___196a94h_difficulty = SDL_SwapLE32(cfg->difficulty);
        ___19bd58h_gamepad = SDL_SwapLE32(cfg->gamepad);
        memcpy(___1a201ah_modem_dial_number, cfg->modem_dial_number, sizeof(cfg->modem_dial_number));
        memcpy(___1a1ffch_modem_init_string, cfg->modem_init_string, sizeof(cfg->modem_init_string));
        ___196a90h_modem_dialing = SDL_SwapLE32(cfg->modem_dialing);
        ___1a1e50h_com_port_standard = SDL_SwapLE32(cfg->com_port_standard);
        ___199fa4h_com_port_addr = SDL_SwapLE32(cfg->com_port_addr);
        ___199fa8h_com_port_irq = SDL_SwapLE32(cfg->com_port_irq);
        memcpy(___19f750h, cfg->track_records, sizeof(cfg->track_records));
        for (int i = 0; i < 6; i++)
        {
            for (int j = 0; j < 18; j++)
            {
                record_t* rec = &___19f750h[i][j];
                rec->min = SDL_SwapLE32(rec->min);
                rec->sec = SDL_SwapLE32(rec->sec);
                rec->sec100 = SDL_SwapLE32(rec->sec100);
            }
        }
        memcpy(___1a0e28h, cfg->hall_of_fame, sizeof(cfg->hall_of_fame));
        for (int i = 0; i < 10; i++)
        {
            hof_entry_t* hof = &___1a0e28h[i];
            hof->races = SDL_SwapLE32(hof->races);
            hof->difficulty = SDL_SwapLE32(hof->difficulty);
        }
        ___1a1140h.accelerate = SDL_SwapLE32(cfg->kb_accelerate);
        ___1a1140h.brake = SDL_SwapLE32(cfg->kb_brake);
        ___1a1140h.steer_left = SDL_SwapLE32(cfg->kb_steer_left);
        ___1a1140h.steer_right = SDL_SwapLE32(cfg->kb_steer_right);
        ___1a1140h.turbo_boost = SDL_SwapLE32(cfg->kb_turbo_boost);
        ___1a1140h.machine_gun = SDL_SwapLE32(cfg->kb_machine_gun);
        ___1a1140h.drop_mine = SDL_SwapLE32(cfg->kb_drop_mine);
        ___1a1140h.horn = SDL_SwapLE32(cfg->kb_horn);
        ___1a1164h_gp_accelerate = SDL_SwapLE32(cfg->gp_accelerate);
        ___1a113ch_gp_brake = SDL_SwapLE32(cfg->gp_brake);
        ___1a1110h_gp_steer_left = SDL_SwapLE32(cfg->gp_steer_left);
        ___1a1130h_gp_steer_right = SDL_SwapLE32(cfg->gp_steer_right);
        ___1a1120h_gp_turbo_boost = SDL_SwapLE32(cfg->gp_turbo_boost);
        ___1a1118h_gp_machine_gun = SDL_SwapLE32(cfg->gp_machine_gun);
        ___1a111ch_gp_drop_mine = SDL_SwapLE32(cfg->gp_drop_mine);
        ___1a1f3ch_counter = SDL_SwapLE32(cfg->counter);

        switch(D(___185a5ch+7*0x1c+0x18)){
        case STANDARD_COM1:
            ___199fa4h_com_port_addr = 0x3f8;
            ___199fa8h_com_port_irq = 4;
            ___1a1e50h_com_port_standard = STANDARD_COM1;
            break;
        case STANDARD_COM2:
            ___199fa4h_com_port_addr = 0x2f8;
            ___199fa8h_com_port_irq = 3;
            ___1a1e50h_com_port_standard = STANDARD_COM2;
            break;
        case STANDARD_COM3:
            ___199fa4h_com_port_addr = 0x3e8;
            ___199fa8h_com_port_irq = 4;
            ___1a1e50h_com_port_standard = STANDARD_COM3;
            break;
        case STANDARD_COM4:
            ___199fa4h_com_port_addr = 0x2e8;
            ___199fa8h_com_port_irq = 3;
            ___1a1e50h_com_port_standard = STANDARD_COM4;
            break;
        default:
            break;
        }

        dRMemory_free((__POINTER__)cfg);
    }
}

void CONFIG_WRITE(void){

    FILE *      fd;
    config_t *  cfg;

    cfg = (config_t *)dRMemory_alloc(sizeof(config_t));

    fd = strupr_fopen(CONFIG_FILE_NAME, "rb");
    fread(&cfg->soundtype, sizeof(__BYTE__), 1, fd);
    fread(&cfg->soundirq, sizeof(__BYTE__), 1, fd);
    fread(&cfg->sounddma, sizeof(__BYTE__), 1, fd);
    fread(&cfg->soundaddr, sizeof(__DWORD__), 1, fd);
    fclose(fd);

    cfg->msx_volume = SDL_SwapLE32(___24cc58h_msx_volume);
    cfg->sfx_volume = SDL_SwapLE32(___24cc54h_sfx_volume);
    cfg->com_port_option = SDL_SwapLE32(D(___185a5ch+7*0x1c+0x18));
    cfg->difficulty = SDL_SwapLE32(___196a94h_difficulty);
    cfg->gamepad = SDL_SwapLE32(___19bd58h_gamepad);
    memcpy(cfg->modem_dial_number, ___1a201ah_modem_dial_number, sizeof(cfg->modem_dial_number));
    memcpy(cfg->modem_init_string, ___1a1ffch_modem_init_string, sizeof(cfg->modem_init_string));
    cfg->modem_dialing = SDL_SwapLE32(___196a90h_modem_dialing);
    cfg->com_port_standard = SDL_SwapLE32(___1a1e50h_com_port_standard);
    cfg->com_port_addr = SDL_SwapLE32(___199fa4h_com_port_addr);
    cfg->com_port_irq = SDL_SwapLE32(___199fa8h_com_port_irq);
    memcpy(cfg->track_records, ___19f750h, sizeof(cfg->track_records));
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 18; j++)
        {
            record_t* rec = &cfg->track_records[i][j];
            rec->min = SDL_SwapLE32(rec->min);
            rec->sec = SDL_SwapLE32(rec->sec);
            rec->sec100 = SDL_SwapLE32(rec->sec100);
        }
    }
    memcpy(cfg->hall_of_fame, ___1a0e28h, sizeof(cfg->hall_of_fame));
    for (int i = 0; i < 10; i++)
    {
        hof_entry_t* hof = &cfg->hall_of_fame[i];
        hof->races = SDL_SwapLE32(hof->races);
        hof->difficulty = SDL_SwapLE32(hof->difficulty);
    }
    cfg->kb_accelerate = SDL_SwapLE32(___1a1140h.accelerate);
    cfg->kb_brake = SDL_SwapLE32(___1a1140h.brake);
    cfg->kb_steer_left = SDL_SwapLE32(___1a1140h.steer_left);
    cfg->kb_steer_right = SDL_SwapLE32(___1a1140h.steer_right);
    cfg->kb_turbo_boost = SDL_SwapLE32(___1a1140h.turbo_boost);
    cfg->kb_machine_gun = SDL_SwapLE32(___1a1140h.machine_gun);
    cfg->kb_drop_mine = SDL_SwapLE32(___1a1140h.drop_mine);
    cfg->kb_horn = SDL_SwapLE32(___1a1140h.horn);
    cfg->gp_accelerate = SDL_SwapLE32(___1a1164h_gp_accelerate);
    cfg->gp_brake = SDL_SwapLE32(___1a113ch_gp_brake);
    cfg->gp_steer_left = SDL_SwapLE32(___1a1110h_gp_steer_left);
    cfg->gp_steer_right = SDL_SwapLE32(___1a1130h_gp_steer_right);
    cfg->gp_turbo_boost = SDL_SwapLE32(___1a1120h_gp_turbo_boost);
    cfg->gp_machine_gun = SDL_SwapLE32(___1a1118h_gp_machine_gun);
    cfg->gp_drop_mine = SDL_SwapLE32(___1a111ch_gp_drop_mine);
    cfg->counter = SDL_SwapLE32(___1a1f3ch_counter);

    if (CONFIG_ENCODED) {
        CONFIG_ENCODE(cfg);
    }

    fd = strupr_fopen(CONFIG_FILE_NAME, "wb");
    fwrite(cfg, sizeof(config_t), 1, fd);
    fclose(fd);

    dRMemory_free((__POINTER__)cfg);
}

static void CONFIG_DEFAULT(void){

    __DWORD__       eax, ebx, ecx, edx, esi, edi, ebp;

    ___24cc54h_sfx_volume = 0xc000;
    ___24cc58h_msx_volume = 0x8000;
    ___1a1e50h_com_port_standard = STANDARD_COM1;
    ___199fa4h_com_port_addr = 0x3f8;
    ___199fa8h_com_port_irq = 4;
    ___196a94h_difficulty = I_LIVE_TO_RIDE;
    ___19bd58h_gamepad = 0;
    ___196a90h_modem_dialing = 1;
    ___1a1f3ch_counter = 0;
    strcpy(___1a201ah_modem_dial_number, "");
    strcpy(___1a1ffch_modem_init_string, "atz");

    career_records_default();

    ___1a1110h_gp_steer_left = B(___199f3eh);
    ___1a1130h_gp_steer_right = B(___199f3fh);
    ___1a113ch_gp_brake = B(___199f41h);
    ___1a1164h_gp_accelerate = B(___199f42h);
    ___1a1120h_gp_turbo_boost = B(___199f43h);
    ___1a1118h_gp_machine_gun = B(___199f44h);
    ___1a111ch_gp_drop_mine = B(___199f45h);
    ___1a1140h.turbo_boost = DR_SCAN_LSHIFT;
    ___1a1140h.horn = DR_SCAN_SPACE;
    ___1a1140h.steer_left = DR_SCAN_LEFT;
    ___1a1140h.steer_right = DR_SCAN_RIGHT;
    ___1a1140h.brake = DR_SCAN_Z;
    ___1a1140h.drop_mine = DR_SCAN_LALT;
    ___1a1140h.accelerate = DR_SCAN_A;
    ___1a1140h.machine_gun = DR_SCAN_LCTRL;

    CONFIG_WRITE();
}
