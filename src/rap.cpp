#include <stdio.h>
#include <string.h>
#include <time.h>
#include "SDL.h"
#include "common.h"
#include "glbapi.h"
#include "i_video.h"
#include "gfxapi.h"
#include "vmemapi.h"
#include "intro.h"
#include "fx.h"
#include "windows.h"
#include "swdapi.h"
#include "ptrapi.h"
#include "imsapi.h"
#include "kbdapi.h"
#include "input.h"
#include "prefapi.h"
#include "loadsave.h"
#include "help.h"
#include "objects.h"
#include "demo.h"
#include "tile.h"
#include "anims.h"
#include "bonus.h"
#include "shots.h"
#include "eshot.h"
#include "enemy.h"
#include "shadow.h"
#include "flame.h"
#include "input.h"
#include "joyapi.h"
#include "i_lastscr.h"
#include "fileids.h"

#ifdef _WIN32
#include <io.h>
#endif // _WIN32
#ifdef __GNUC__
#include <unistd.h>
#endif // __GNUC__

#define wmemcpy(dst,src,size) memmove(dst,src,size)

struct bday_t {
    int month;
    int day;
    int year;
    const char *f_c;
};

int wRandSeed = 1;

int wrand(void)
{
    wRandSeed = ((long long)wRandSeed * 1103515245) + 12345;
    
    return (wRandSeed >> 16) & 0x7fff;
}

void wsrand(int seed)
{
    wRandSeed = seed;
}

#define MAX_BDAY 6

bday_t bday[MAX_BDAY];

#define MONTH     2
#define DAY       20
#define YEAR      1994
#define WLENGTH   12

int bday_flag = 0;
int bday_num = -1;
char *palette;
int godmode;
int cur_diff;
int reg_flag;
int demo_mode;
int demo_flag;
int ingameflag;
int cur_game;
int game_wave[4];

int gameflag[4];
int curship[16];
int lship[8];
int dship[8];
int fship[8];
texture_t *numbers[11];

char gdmodestr[] = "CASTLE";

player_t plr;

char* g_highmem;
char* LASTSCR;

texture_t *ptrtex;
int draw_player;
int fadeflag;
int end_fadeflag;
int debugflag;
int playerx = PLAYERINITX;
int playery = PLAYERINITY;
int player_cx = PLAYERINITX;
int player_cy = PLAYERINITY;
int playerbasepic = 3;
int startfadeflag;
int g_oldsuper = -1;
int g_oldshield = -1;
int startendwave = -1;
int gl_cnt;
int g_flash = 0;
char gpal[768];
int end_wave;
int playerpic = 4;
int g_mapleft;

int fadecnt;

#define ROTPAL_START 240 
#define FADE_FRAMES  20
#define MAX_GLOW 20

int shakes[FADE_FRAMES] = {
    -4, 4, -3, 3, -2, 2, -1, 1,
    -1, 1, -1, 1, -1, 1, -1, 1,
    -1, 1, 0, 0
};

int o_engine[8] = {
    0, 1, 2, 3, 2, 1, 0, 0
};

int o_gun1[8] = {
    1, 3, 5, 6, 5, 3, 1, 0
};

int o_gun2[8] = {
    1, 3, 6, 9, 6, 3, 2, 0
};

int o_gun3[8] = {
    2, 6, 8, 11, 8, 6, 2, 0
};

int glowtable[20] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
};

char flatnames[4][14] = {
    "FLATSG1_ITM",
    "FLATSG2_ITM",
    "FLATSG3_ITM",
    "FLATSG4_ITM"
};

flat_t *flatlib[4];

/***************************************************************************
RAP_Bday() - Get system date
 ***************************************************************************/
void 
RAP_Bday(
    void
)
{
    time_t t = time(NULL);
    struct tm *date = localtime(&t);
    int loop;
    
    bday_flag = 0;
    bday_num = -1;
    
    for (loop = 0; loop < MAX_BDAY; loop++)
    {
        if (bday[loop].day)
        {
            if (date->tm_mon + 1 == bday[loop].month && date->tm_mday == bday[loop].day && date->tm_year + 1900 >= bday[loop].year)
            {
                bday_num = loop;
                
                if (loop == 0)
                    bday_flag = 1;
            }
        }
    }
}

/***************************************************************************
InitScreen() - Prints the Init Screen
 ***************************************************************************/
void 
InitScreen(
    void
)
{
    printf(" RAPTOR: Call Of The Shadows V1.2                        (c)1994 Cygnus Studios\n");
}

/*==========================================================================
   ShutDown () Shut Down function called by EXIT_xxx functions
 ==========================================================================*/
void 
ShutDown(
    int errcode
)
{
    if (!errcode && !godmode)
        WIN_Order();

    //IPT_DeInit();
    //DMX_DeInit();
    //GFX_EndSystem();
    //PTR_End();
    //KBD_End();
    
    if (reg_flag)
        LASTSCR = GLB_GetItem(FILE002_LASTSCR2_TXT); //Get ANSI Screen Fullversion from GLB to char*
    else
        LASTSCR = GLB_GetItem(FILE001_LASTSCR1_TXT); //Get ANSI Screen Shareware from GLB to char*

    closewindow();                                   //Close Main Window
    I_LASTSCR();                                     //Call to display ANSI Screen 
    GLB_FreeAll();
    IPT_CloJoy();                                    //Close Joystick
    SWD_End();
    SDL_Quit();
    
    free(g_highmem);
}

/***************************************************************************
RAP_ClearSides () - Clears the Sides
 ***************************************************************************/
void 
RAP_ClearSides(
    void
)
{
    GFX_ColorBox(0, 0, MAP_LEFT, 200, 0);
    GFX_ColorBox(320 - MAP_LEFT, 0, MAP_LEFT, 200, 0);
}

/***************************************************************************
RAP_GetShipPic () - Loads Correct Ship Pics for Light/Dark Waves
 ***************************************************************************/
void 
RAP_GetShipPic(
    void
)
{
    int lightflag, loop;
    lightflag = 1;
    
    // GAME 2 wave 8
    if (cur_game == 1 && game_wave[cur_game] == 7)
        lightflag = 0;
    
    // GAME 3 wave 3
    if (cur_game == 2 && game_wave[cur_game] == 2)
        lightflag = 0;

    for (loop = 0; loop < 7; loop++)
    {
        if (lightflag)
        {
            curship[loop] = lship[loop];
            curship[loop + 7] = lship[loop];
        }
        else
        {
            curship[loop] = dship[loop];
            curship[loop + 7] = fship[loop];
        }
    }
    
    for (loop = 0; loop < 14; loop++)
    {
        GLB_CacheItem(curship[loop]);
    }
}

/***************************************************************************
   Rot_Color () - Rotates color in palette
 ***************************************************************************/
void 
Rot_Color(
    char *gpal, 
    int snum, 
    int len)
{
    short pos;
    short maxloop;
    char h1[3];
    
    pos = snum * 3;
    maxloop = len * 3;

    wmemcpy(h1, &gpal[pos], 3);
    wmemcpy(&gpal[pos], &gpal[pos+3], maxloop);
    wmemcpy(&gpal[pos+maxloop-3], h1, 3);
}

/***************************************************************************
   InitMobj() - Inits an object to be moved
 ***************************************************************************/
void 
InitMobj(
    mobj_t *cur            // INPUT : pointer to MOVEOBJ
)
{
    cur->done = 0;
    cur->addx = 1;
    cur->addy = 1;
    
    cur->delx = cur->x2 - cur->x;
    cur->dely = cur->y2 - cur->y;
    
    if (cur->delx < 0)
    {
        cur->delx = -cur->delx;
        cur->addx = -cur->addx;
    }
    if (cur->dely < 0)
    {
        cur->dely = -cur->dely;
        cur->addy = -cur->addy;
    }
    
    if (cur->delx >= cur->dely)
    {
        cur->err = -(cur->dely >> 1);
        cur->maxloop = cur->delx + 1;
    }
    else
    {
        cur->err = cur->delx >> 1;
        cur->maxloop = cur->dely + 1;
    }
}

void MoveMobj(mobj_t *m)
{
    if (m->maxloop == 0)
    {
        m->done = 1;
        return;
    }
    if (m->delx >= m->dely)
    {
        m->x += m->addx;
        m->err += m->dely;
        if (m->err > 0)
        {
            m->y += m->addy;
            m->err -= m->delx;
        }
    }
    else
    {
        m->y += m->addy;
        m->err += m->delx;
        if (m->err > 0)
        {
            m->x += m->addx;
            m->err -= m->dely;
        }
    }
    m->maxloop--;
}

int MoveSobj(mobj_t *m, int a2)
{
    if (a2 == 0)
        return 0;
    if (m->delx >= m->dely)
    {
        while (a2)
        {
            a2--;
            m->maxloop--;
            m->x += m->addx;
            m->err += m->dely;
            if (m->err > 0)
            {
                m->y += m->addy;
                m->err -= m->delx;
            }
        }
    }
    else
    {
        while (a2)
        {
            a2--;
            m->maxloop--;
            m->y += m->addy;
            m->err += m->delx;
            if (m->err > 0)
            {
                m->x += m->addx;
                m->err -= m->dely;
            }
        }
    }
    if (m->maxloop < 1)
        m->done = 1;
    return a2;
}

void RAP_PrintNum(int a1, int a2, char *a3)
{
    int l, v18;

    l = strlen(a3);
    GFX_PutSprite(numbers[10], a1, a2);
    a1 += 9;
    while (--l != -1)
    {
        v18 = *a3 - '0';
        if (v18 < 11 && v18 >= 0)
            GFX_PutSprite(numbers[v18], a1, a2);
        a1 += 8;
        a3++;
    }
}

void RAP_DisplayShieldLevel(int a1, int a2)
{
    char *v1c;
    unsigned int v24, v20;
    int i;

    v1c = &displayscreen[ylookup[199] + a1];

    v20 = 5898;
    v24 = 0;
    for (i = 0; i < 100; i++)
    {
        if (i < a2)
            memset(v1c, 74 - (v24 >> 16), 4);
        else
            memset(v1c, 0, 4);
        v24 += v20;
        v1c -= 320 * 2;
    }
}

void RAP_DisplayStats(void)
{
    char v4c[24];
    int v24, v20, i, v1c, v30;
    texture_t *v34, *v2c;
    static int damage = -1;
    static int blinkflag = 1;

    v24 = OBJS_GetAmt(15);
    if (g_oldsuper != v24)
    {
        RAP_DisplayShieldLevel(8, v24);
        g_oldsuper = v24;
    }
    v20 = OBJS_GetAmt(16);
    if (g_oldshield != v20)
        RAP_DisplayShieldLevel(308, v20);

    if (v20 <= 0 && !godmode)
    {
        ANIMS_StartAnim(5, playerx + (wrand()%32), playery + (wrand()%32));
        ANIMS_StartAnim(6, playerx + (wrand()%32), playery + (wrand()%32));
        if (startendwave > 24)
        {
            if ((wrand()%2) == 0)
                SND_Patch(8, 0x1e);
            else
                SND_Patch(8, 0xe1);
        }
        if (startendwave == -1)
            startendwave = 60;
        if (startendwave == 24)
        {
            draw_player = 0;
            SND_Patch(8, 127);
            SND_Patch(9, 127);
            ANIMS_StartAnim(4, player_cx, player_cy);
            for (i = 0; i < 512; i++)
            {
                v1c = playerx - 16 + (wrand() % 32) * 2;
                v30 = playery - 16 + (wrand() % 32) * 2;
                if (i&1)
                    ANIMS_StartAnim(4, v1c, v30);
                else
                    ANIMS_StartAAnim(7, v1c, v30);
            }
            SND_Patch(9, 127);
        }
    }

    if (startendwave != -1 && v20 > 0)
    {
        if (startendwave == 40)
        {
            IPT_PauseControl(1);
            SND_Patch(13, 127);
        }
        if (startendwave < 40)
        {
            v1c = 0;
            if (playerx < 0x98)
                v1c = 8;
            else if (playerx > 0xa8)
                v1c = -8;
            IPT_FMovePlayer(v1c, -4);
        }
    }
    if (v20 < 11 && !godmode)
    {
        if (!(gl_cnt % 8))
        {
            blinkflag ^= 1;
            if (blinkflag)
            {
                if (damage)
                {
                    damage--;
                    if ((haptic) && (control == 2))
                    {
                        IPT_CalJoyRumbleHigh();                                                                   //Rumble when Shield is low
                    }
                }
            }
        }
        if (v20 < g_oldshield && v24 < 1)
        {
            if (OBJS_LoseObj())
            {
                SND_Patch(11, 127);
                damage = 2;
            }
        }
        if (blinkflag)
        {
            if (damage)
            {
                v2c = v34 = (texture_t*)GLB_GetItem(FILE111_WEPDEST_PIC);
                GFX_PutSprite(v34, (320 - v2c->width) >> 1, 0xad);
            }
            if (startendwave == -1)
                SND_Patch(22, 127);
            v2c = v34 = (texture_t*)GLB_GetItem(FILE110_SHLDLOW_PIC);
            GFX_PutSprite(v34, (320 - v2c->width) >> 1, 0xb6);
        }
    }
    g_oldshield = v20;
    OBJS_DisplayStats();
    sprintf(v4c, "%08u", plr.score);
    RAP_PrintNum(0x77, 2, v4c);
    if (demo_mode == 1)
        DEMO_DisplayStats();
    if (debugflag)
    {
        sprintf(v4c, "%02u", plr.diff[cur_game]);
        RAP_PrintNum(0x12, 2, v4c);
        v1c = 32;
        for (i = 0; i < 16; i++)
        {
            GFX_ColorBox(v1c, 0, 8, 8, 240 + i);
            v1c += 8;
        }
    }
}

void RAP_PaletteStuff(void)
{
    static int wblink = 0;
    static int glow1 = 0;
    static int glow2 = 8;
    static int cnt = 0;
    static int palcnt = 0;
    static int blink = 0;
    int v1c;
    char *v24, *v28;
    int v20;
    if (cnt & 1)
    {
        if (palcnt % 4 != 0)
            Rot_Color(gpal, 240, 5);
        if (palcnt % 2 != 0)
            Rot_Color(gpal, 245, 5);
        v1c = 250 * 3;
        v24 = &gpal[v1c];
        v28 = &palette[v1c];
        v20 = glowtable[glow1];
        glow1++;
        glow1 %= 20;
        *v24 = *v28 - v20;
        if ((uint8_t)*v28 < v20)
            *v24 = 0;
        v24++;
        v28++;
        *v24 = *v28 - v20;
        if ((uint8_t)*v28 < v20)
            *v24 = 0;
        v24++;
        v28++;
        *v24 = *v28 - v20;
        if ((uint8_t)*v28 < v20)
            *v24 = 0;

        v1c = 251 * 3;
        v24 = &gpal[v1c];
        v28 = &palette[v1c];
        v20 = glowtable[glow2];
        glow2++;
        glow2 %= 20;
        *v24 = *v28 - v20;
        if ((uint8_t)*v28 < v20)
            *v24 = 0;
        v24++;
        v28++;
        *v24 = *v28 - v20;
        if ((uint8_t)*v28 < v20)
            *v24 = 0;
        v24++;
        v28++;
        *v24 = *v28 - v20;
        if ((uint8_t)*v28 < v20)
            *v24 = 0;
        if (palcnt % 2)
        {
            v1c = 252 * 3;
            v24 = &gpal[v1c];
            v28 = &palette[v1c];
            if (blink & 1)
            {
                memcpy(v24, v28, 3);
                v24 += 3;
                v28 += 3;
                memset(v24, 0, 3);
            }
            else
            {
                memset(v24, 0, 3);
                v24 += 3;
                v28 += 3;
                memcpy(v24, v28, 3);
            }
            blink++;
        }
        v1c = 254 * 3;
        if ((wrand() % 3) == 0)
        {
            v24 = &gpal[v1c];
            v28 = &palette[v1c];
            memcpy(v24, v28, 3);
        }
        else
        {
            v24 = &gpal[v1c];
            memset(v24, 0, 3);
        }
        v1c = 255 * 3;
        if (wblink < 3)
        {
            v24 = &gpal[v1c];
            v28 = &palette[v1c];
            memcpy(v24, v28, 3);
        }
        else
        {
            v24 = &gpal[v1c];
            memset(v24, 0, 3);
        }
        wblink++;
        wblink %= 6;
        GFX_SetPalette(gpal, 240);
        palcnt++;
    }
    cnt++;
}

int Do_Game(void)
{
    int v20, v24, v28, v2c, v30, v1c;
    v20 = 0;
    v24 = 0;
    v28 = 1;
    v2c = 0;
    v30 = 0;
    draw_player = 1;

    wsrand(game_wave[cur_game] << 10);
    fadeflag = 0;
    end_fadeflag = 0;
    KBD_Clear();
    IMS_StartAck();
    buttons[0] = 0;
    buttons[1] = 0;
    buttons[2] = 0;
    buttons[3] = 0;
    playerx = 0x90;
    playery = 0xa0;
    if (!demo_mode) {
        PTR_SetPos(0x90, 0xa0);
        IPT_Start();
    }
    RAP_GetShipPic();
    BONUS_Clear();
    ANIMS_Clear();
    SHOTS_Clear();
    ESHOT_Clear();
    memcpy(gpal, palette, 768);
    GFX_FadeOut(0, 0, 0, 2);
    memset(displaybuffer, 0, 0xfa00);
    memset(displayscreen, 0, 0xfa00);
    v1c = GFX_GetFrameCount();
    g_flash = 0;
    g_oldsuper = -1;
    g_oldshield = -1;
    startendwave = -1;
    if (demo_flag == 1)
        DEMO_StartRec();

    v30 = plr.score;
    IMS_StartAck();
    memset(buttons, 0, sizeof(buttons));
    do
    {
        num_shadows = num_gshadows = 0;
        IPT_MovePlayer();
        if (KBD_IsKey(0x3b))                                                                   //Input Help Screen GamePad not implemented
        {
            SWD_SetClearFlag(0);
            IPT_End();
            HELP_Win("OVERVW06_TXT");
            memset(displayscreen, 0, 0xfa00);
            IPT_Start();
            g_oldsuper = -1;
            g_oldshield = -1;
            RAP_ClearSides();
            RAP_DisplayStats();
            SWD_SetClearFlag(1);
            IMS_StartAck();
            buttons[0] = 0;
            buttons[1] = 0;
            buttons[2] = 0;
            buttons[3] = 0;
            v20 = 0;
            v24 = 0;
        }
        if (KBD_IsKey(0x19) || JOY_IsKeyInGameStart(Start))                                                                  //Input Pause Screen
        {
            while (IMS_IsAck())
            {
            }
            SWD_SetClearFlag(0);
            RAP_ClearSides();
            WIN_Pause();
            g_oldsuper = -1;
            g_oldshield = -1;
            RAP_ClearSides();
            RAP_DisplayStats();
            SWD_SetClearFlag(1);
            IMS_StartAck();
            buttons[0] = 0;
            buttons[1] = 0;
            buttons[2] = 0;
            buttons[3] = 0;
            v20 = 0;
            v24 = 0;
        }
        if (KBD_IsKey(0x42) && godmode)
        {
            while (IMS_IsAck())
            {
            }
            debugflag ^= 1;
        }
        if (DEMO_Think())
            break;
        if (demo_mode == 2)
        {
            if (IMS_IsAck() && !buttons[1] && !buttons[2] && !buttons[3])
            {
                v2c = 1;
                break;
            }
        }
       
        switch (lastscan)
        {
        case 0:
            break;
        case 1:
            break;
        case 2:
            OBJS_MakeSpecial(3);
            break;
        case 3:
            OBJS_MakeSpecial(4);
            break;
        case 4:
            OBJS_MakeSpecial(5);
            break;
        case 5:
            OBJS_MakeSpecial(6);
            break;
        case 6:
            OBJS_MakeSpecial(7);
            break;
        case 7:
            OBJS_MakeSpecial(8);
            break;
        case 8:
            OBJS_MakeSpecial(9);
            break;
        case 9:
            OBJS_MakeSpecial(10);
            break;
        case 10:
            OBJS_MakeSpecial(12);
            break;
        case 11:
            OBJS_MakeSpecial(14);
            break;
        case 12:
            OBJS_MakeSpecial(13);
            break;
        }
        if (buttons[0])
        {
            OBJS_Use(0);
            OBJS_Use(1);
            OBJS_Use(2);
            buttons[0] = 0;
            if (plr.sweapon != -1)
                OBJS_Use(plr.sweapon);
        }
        if (buttons[1])
        {
            buttons[1] = 0;
            if (!v20)
            {
                SND_Patch(20, 127);
                v20 = 1;
                OBJS_GetNext();
            }
        }
        else
        {
            if (v20 == 1)
                v20 = 0;
        }
        if (buttons[2])
        {
            buttons[2] = 0;
            if (!v24)
            {
                v24 = 1;
                OBJS_Use(11);
            }
        }
        else
        {
            if (v24 == 1)
                v24 = 0;
        }
        if (startendwave != -1)
        {
            if (startendwave == 0)
                end_wave = 1;
            startendwave--;
        }
        gl_cnt++;
        TILE_Think();
        ENEMY_Think();
        ESHOT_Think();
        BONUS_Think();
        SHOTS_Think();
        ANIMS_Think();
        OBJS_Think();
        if (draw_player)
            SHADOW_Add(curship[playerpic + g_flash], playerx, playery);
        TILE_Display();
        SHADOW_DisplayGround();
        ENEMY_DisplayGround();
        SHADOW_DisplaySky();
        ANIMS_DisplayGround();
        ENEMY_DisplaySky();
        SHOTS_Display();
        BONUS_Display();
        ANIMS_DisplaySky();
        if (draw_player)
        {
            FLAME_Down(player_cx - o_engine[playerpic] - 3, player_cy + 15, 4, gl_cnt % 2);
            FLAME_Down(player_cx + o_engine[playerpic] - 2, player_cy + 15, 4, gl_cnt % 2);
            GFX_PutSprite((texture_t*)GLB_GetItem(curship[playerpic + g_flash]), playerx, playery);
            g_flash = 0;
        }
        ANIMS_DisplayHigh();
        ESHOT_Display();
        if (fadeflag)
        {
            if (fadecnt >= 19)
            {
                RAP_ClearSides();
                g_mapleft = 16;
                GFX_SetPalette(gpal, 0);
                fadeflag = 0;
                g_oldsuper = -1;
                g_oldshield = -1;
            }
            else
            {
                RAP_ClearSides();
                retraceflag = 0;
                GFX_FadeFrame(gpal, fadecnt, 20);
                retraceflag = 0;
                g_mapleft = shakes[fadecnt] + 16;
                fadecnt++;
            }
        }
        else
        {
            if (!v28)
                RAP_PaletteStuff();
        }
        RAP_DisplayStats();
        while (GFX_GetFrameCount() - v1c < 3)
        {
        }
        v1c = GFX_GetFrameCount();
        if (fadeflag)
            TILE_ShakeScreen();
        else
            TILE_DisplayScreen();
        if (startfadeflag)
        {
            SND_Patch(15, 127);
            SND_Patch(8, 127);
            retraceflag = 1;
            GFX_FadeOut(63, 60, 60, 1);
            GFX_FadeStart();
            startfadeflag = 0;
            fadeflag = 1;
            fadecnt = 0;
            retraceflag = 0;
        }
        if (reg_flag && keyboard[14])
        {
            OBJS_Add(14);
            OBJS_Add(16);
            OBJS_Add(16);
            OBJS_Add(16);
            plr.score = 0;
        }
        if (v28)
        {
            v28 = 0;
            GFX_FadeIn(gpal, 64);
        }
        if (keyboard[45] && keyboard[56])
        {
            RAP_ClearSides();
            SWD_SetClearFlag(0);
            IPT_End();
            WIN_AskExit();
            IPT_Start();
            g_oldsuper = -1;
            g_oldshield = -1;
            RAP_ClearSides();
            RAP_DisplayStats();
            SWD_SetClearFlag(1);
            IMS_StartAck();
            buttons[0] = 0;
            buttons[1] = 0;
            buttons[2] = 0;
            buttons[3] = 0;
            v20 = 0;
            v24 = 0;
        }
        if (KBD_IsKey(1) || JOY_IsKeyInGameBack(Back))                                                                      //Fixed Line GamePad Abort Mission Screen 
        {
            if (godmode)
                end_wave = 1;
            if (!demo_mode)
            {
                SWD_SetClearFlag(0);
                IPT_End();
                RAP_ClearSides();
                if (WIN_AskBool("Abort Mission ?"))
                {
                    plr.score = v30;
                    v2c = 1;
                    break;
                }
                IPT_Start();
                g_oldsuper = -1;
                g_oldshield = -1;
                RAP_ClearSides();
                RAP_DisplayStats();
                SWD_SetClearFlag(1);
                IMS_StartAck();
                buttons[0] = 0;
                buttons[1] = 0;
                buttons[2] = 0;
                buttons[3] = 0;
                v20 = 0;
                v24 = 0;
            }
            else if (demo_mode == 2)
            {
                v2c = 1;
                break;
            }
            else if (demo_mode == 1)
                break;
        }
    } while (!end_wave);
    GFX_FadeOut(0, 0, 0, 32);
    RAP_FreeMap();
    end_wave = 0;
    memset(displaybuffer, 0, 0xfa00);
    GFX_MarkUpdate(0, 0, 320, 200);
    IPT_PauseControl(0);
    GFX_DisplayUpdate();
    GFX_SetPalette(palette, 0);
    IPT_End();
    if (demo_flag == 1)
        DEMO_SaveFile();
    return v2c;
}

void RAP_InitMem(void)
{
    unsigned int heapsize = 0x495FF0;                       
    g_highmem = (char*)calloc(heapsize, 1);
    VM_InitMemory(g_highmem, heapsize);
    GLB_UseVM();
}

int main(int argc, char *argv[])
{
    char *shost, *reg_text, *pal;
    int i, eps, v28, v20;

    shost = getenv("S_HOST");

    InitScreen();

    RAP_InitLoadSave();
    if (access(RAP_SetupFilename(), 0))
    {
        printf("\n\n** You must run SETUP first! **\n");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Raptor", "** You must run SETUP first! **", NULL);
        exit(0);
    }

    godmode = 0;

    if (shost != NULL && !strcmp(shost, gdmodestr))
        godmode = 1;
    else
        godmode = 0;

    if (argv[1])
    {
        if (!strcmp(argv[1], "REC"))
        {
            DEMO_SetFileName(argv[2]);
            demo_flag = 1;
            printf("DEMO RECORD enabled\n");
        }
        else if (!strcmp(argv[1], "PLAY"))
        {
            if (!access(argv[2], 0))
            {
                DEMO_SetFileName(argv[2]);
                demo_flag = 2;
                printf("DEMO PLAYBACK enabled\n");
            }
        }
    }

    if (godmode)
        printf("GOD mode enabled\n");
    cur_diff = 0;

    if (!access("FILE0001.GLB", 0))
        gameflag[0] = 1;
    if (!access("FILE0002.GLB", 0))
        gameflag[1] = 1;
    if (!access("FILE0003.GLB", 0) && !access("FILE0004.GLB", 0))
    {
        gameflag[2] = 1;
        gameflag[3] = 1;
    }

    if (gameflag[1] + gameflag[2])
        reg_flag = 1;

    eps = 0;
    for (i = 0; i < 4; i++)
        if (gameflag[i])
            eps++;

    if (access("FILE0000.GLB", 0) || !eps)
    {
        printf("All game data files NOT FOUND cannot proceed !!\n");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Raptor", "All game data files NOT FOUND cannot proceed !!", NULL);
        exit(0);
    }
    printf("Init -\n");
    EXIT_Install(ShutDown);
    memset(bday, 0, sizeof(bday));
    bday[0].month = 5;
    bday[0].day = 16;
    bday[0].year = 1994;
    bday[0].f_c = "Scott Host";

    bday[1].month = 2;
    bday[1].day = 27;
    bday[1].year = 1996;
    bday[1].f_c = "Dave T.";

    bday[2].month = 10;
    bday[2].day = 2;
    bday[2].year = 1995;
    bday[2].f_c = "Jim M.";

    bday[3].month = 3;
    bday[3].day = 12;
    bday[3].year = 1995;
    bday[3].f_c = "Bobby P.";

    bday[4].month = 8;
    bday[4].day = 28;
    bday[4].year = 1995;
    bday[4].f_c = "Rich F.";

    bday[5].month = 4;
    bday[5].day = 24;
    bday[5].year = 1996;
    bday[5].f_c = "Paul R.";

    RAP_Bday();

    if (bday_num != -1)
        printf("Birthday() = %s\n", bday[bday_num].f_c);

    if (access(RAP_SetupFilename(), 0))
        EXIT_Error("You Must run SETUP.EXE First !!");

    if (!INI_InitPreference(RAP_SetupFilename()))
        EXIT_Error("SETUP Error");

    fflush(stdout);

    KBD_Install();
    GFX_InitSystem();
    SWD_Install(0);
    VIDEO_LoadPrefs();
    IPT_LoadPrefs();
    switch (control)
    {
    default:
        printf("PTR_Init()-Auto\n");
        fflush(stdout);
        v28 = PTR_Init(1);
        usekb_flag = 1;
        break;
    case 2:
        printf("PTR_Init()-Joystick\n");
        fflush(stdout);
        v28 = PTR_Init(2);
        usekb_flag = 0;
        break;
    case 1:
        printf("PTR_Init()-Mouse\n");
        fflush(stdout);
        v28 = PTR_Init(1);
        usekb_flag = 0;
        break;
    }
    
    if (reg_flag)
    {
        printf("Registered EXE!\n");
        fflush(stdout);
    }
    GLB_InitSystem(argv[0], 6, 0);
    if (reg_flag)
    {
        reg_text = GLB_GetItem(FILE000_ATENTION_TXT);
        printf("%s\n", reg_text);
        GLB_FreeItem(0);
    }
    SND_InitSound();
    IPT_Init();
    GLB_FreeAll();
    RAP_InitMem();
    printf("Loading Graphics\n");
    pal = GLB_LockItem(FILE100_PALETTE_DAT);
    memset(pal, 0, 3);
    palette = pal;
    SHADOW_Init();
    FLAME_Init();
    fflush(stdout);
    if (v28)
    {
        ptrtex = (texture_t*)GLB_LockItem(FILE112_CURSOR_PIC);
        PTR_SetPic(ptrtex);
        PTR_SetPos(160, 100);
        PTR_DrawCursor(0);
    }
    for (i = 0; i < 7; i++)
    {
        lship[i] = FILE11a_LPLAYER_PIC + i;
        if (gameflag[1])
        {
            dship[i] = FILE245_DPLAYER_PIC + i;
            fship[i] = FILE24c_FPLAYER_PIC + i;
        }
    }
    for (i = 0; i < 4; i++)
    {
        if (gameflag[i])
        {
            v20 = GLB_GetItemID(flatnames[i]);
            flatlib[i] = (flat_t*)GLB_LockItem(v20);
        }
        else
            flatlib[i] = NULL;
    }
    for (i = 0; i < 11; i++)
    {
        v20 = FILE105_N0_PIC + i;
        numbers[i] = (texture_t*)GLB_LockItem(v20);
    }

    FLAME_InitShades();
    HELP_Init();
    OBJS_Init();
    TILE_Init();
    SHOTS_Init();
    ESHOT_Init();
    BONUS_Init();
    ANIMS_Init();
    SND_Setup();
    GFX_SetPalRange(0, 239);
    GFX_InitVideo(palette);
    SHADOW_MakeShades();
    RAP_ClearPlayer();
    if (!godmode)
        INTRO_Credits();
    if (demo_flag != 2)
    {
        SND_PlaySong(86, 1, 1);
        INTRO_PlayMain();
        SND_PlaySong(87, 1, 1);
    }
    else if (demo_flag == 2)
    {
        DEMO_LoadFile();
        DEMO_Play();
    }

    cur_game = 0;
    game_wave[0] = 0;
    game_wave[1] = 0;
    game_wave[2] = 0;
    game_wave[3] = 0;
    do
    {
       WIN_MainMenu();
       WIN_MainLoop();
    } while (1);

    return 0;
}
