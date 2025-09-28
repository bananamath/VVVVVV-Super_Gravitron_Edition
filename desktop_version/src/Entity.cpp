#define OBJ_DEFINITION
#include "Entity.h"

#include <SDL.h>
#include <stdarg.h>

#include "CustomLevels.h"
#include "Font.h"
#include "Game.h"
#include "GlitchrunnerMode.h"
#include "Graphics.h"
#include "Localization.h"
#include "Map.h"
#include "Maths.h"
#include "Music.h"
#include "Script.h"
#include "UtilityClass.h"
#include "Vlogging.h"
#include "Xoshiro.h"

static int getgridpoint( int t )
{
    return t / 8;
}

bool entityclass::checktowerspikes(int t)
{
    if (map.invincibility)
    {
        return false;
    }

    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("checktowerspikes() out-of-bounds!");
        return false;
    }

    SDL_Rect temprect;
    temprect.x = entities[t].xp + entities[t].cx;
    temprect.y = entities[t].yp + entities[t].cy;
    temprect.w = entities[t].w;
    temprect.h = entities[t].h;

    int tempx = getgridpoint(temprect.x);
    int tempy = getgridpoint(temprect.y);
    int tempw = getgridpoint(temprect.x + temprect.w - 1);
    int temph = getgridpoint(temprect.y + temprect.h - 1);
    if (map.towerspikecollide(tempx, tempy)) return true;
    if (map.towerspikecollide(tempw, tempy)) return true;
    if (map.towerspikecollide(tempx, temph)) return true;
    if (map.towerspikecollide(tempw, temph)) return true;
    if (temprect.h >= 12)
    {
        int tpy1 = getgridpoint(temprect.y + 6);
        if (map.towerspikecollide(tempx, tpy1)) return true;
        if (map.towerspikecollide(tempw, tpy1)) return true;
        if (temprect.h >= 18)
        {
            tpy1 = getgridpoint(temprect.y + 12);
            if (map.towerspikecollide(tempx, tpy1)) return true;
            if (map.towerspikecollide(tempw, tpy1)) return true;
            if (temprect.h >= 24)
            {
                tpy1 = getgridpoint(temprect.y + 18);
                if (map.towerspikecollide(tempx, tpy1)) return true;
                if (map.towerspikecollide(tempw, tpy1)) return true;
            }
        }
    }
    return false;
}

void entityclass::init(void)
{
    platformtile = 0;
    customplatformtile=0;
    vertplatforms = false;
    horplatforms = false;

    nearelephant = false;
    upsetmode = false;
    upset = 0;

    customenemy = 0;
    customwarpmode = false; customwarpmodevon = false; customwarpmodehon = false;
    customactivitycolour = "";
    customactivitypositiony = -1;
    customactivitytext = "";
    trophytext = 0;
    oldtrophytext = 0;
    trophytype = 0;
    altstates = 0;


    SDL_memset(customcrewmoods, true, sizeof(customcrewmoods));

    resetallflags();
    SDL_memset(collect, false, sizeof(collect));
    SDL_memset(customcollect, false, sizeof(customcollect));

    k = 0;
}

void entityclass::resetallflags(void)
{
    SDL_memset(flags, false, sizeof(flags));
}

int entityclass::swncolour( int t )
{
    //given colour t, return colour in setcol
    if (t == 0) return EntityColour_ENEMY_CYAN;
    if (t == 1) return EntityColour_ENEMY_RED;
    if (t == 2) return EntityColour_ENEMY_PINK;
    if (t == 3) return EntityColour_ENEMY_BLUE;
    if (t == 4) return EntityColour_ENEMY_YELLOW;
    if (t == 5) return EntityColour_ENEMY_GREEN;
    return EntityColour_CREW_CYAN; // Fallback to color 0
}

void entityclass::swnenemiescol( int t )
{
    //change the colour of all SWN enemies to the current one
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == EntityType_GRAVITRON_ENEMY)
        {
            entities[i].colour = swncolour(t);
        }
    }
}

void entityclass::gravcreate(int ypos, int dir, int xoff /*= 0*/, int yoff /*= 0*/, int speed /*= 7*/, int id /*= 0*/)
{
    if (dir == 0) // Right
    {
        createentity(-150 - xoff, 58 + (ypos * 20) + yoff, 23, 0, speed, id);
    }
    else if (dir == 1) // Left
    {
        createentity(320 + 150 + xoff, 58 + (ypos * 20) + yoff, 23, 1, speed, id);
    }
    else if (dir == 2) // Wall
    {
        createentity(0 + xoff, 58 + (ypos * 20) + yoff, 23, 2, speed, id);
    }
    else if (dir == 3) //Homing
    {
        createentity(0 + xoff, 58 + (ypos * 20) + yoff, 23, 3, speed, id);
    }
}

void entityclass::swnfreeze()
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23)
        {
            entities[i].freeze = true;
        }
    }
}
template <typename... Rest>
void entityclass::swnfreeze(int id, Rest... rest)
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23 && entities[i].id == id)
        {
            entities[i].freeze = true;
        }
    }

    if (sizeof...(rest) != 0)
    {
        swnfreeze(rest...);
    }
}

void entityclass::swnunfreeze()
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23)
        {
            entities[i].freeze = false;
        }
    }
}
template <typename... Rest>
void entityclass::swnunfreeze(int id, Rest... rest)
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23 && entities[i].id == id)
        {
            entities[i].freeze = false;
        }
    }

    if (sizeof...(rest) != 0)
    {
        swnunfreeze(rest...);
    }
}

void entityclass::swnreverse()
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23)
        {
            entities[i].reverse = true;
        }
    }
}
template <typename... Rest>
void entityclass::swnreverse(int id, Rest... rest)
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23 && entities[i].id == id)
        {
            entities[i].reverse = true;
        }
    }

    if (sizeof...(rest) != 0)
    {
        swnreverse(rest...);
    }
}

void entityclass::swnunreverse()
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23)
        {
            entities[i].reverse = false;
        }
    }
}
template <typename... Rest>
void entityclass::swnunreverse(int id, Rest... rest)
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23 && entities[i].id == id)
        {
            entities[i].reverse = false;
        }
    }

    if (sizeof...(rest) != 0)
    {
        swnunreverse(rest...);
    }
}

void entityclass::swndelete()
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23)
        {
            disableentity(i);
        }
    }
}
template <typename... Rest>
void entityclass::swndelete(int id, Rest... rest)
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23 && entities[i].id == id)
        {
            disableentity(i);
        }
    }

    if (sizeof...(rest) != 0)
    {
        swndelete(rest...);
    }
}

void entityclass::swnspeedchange(int speed)
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23)
        {
            entities[i].para = speed;
        }
    }
}
template <typename... Rest>
void entityclass::swnspeedchange(int speed, int id, Rest... rest)
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23 && entities[i].id == id)
        {
            entities[i].para = speed;
        }
    }

    if (sizeof...(rest) != 0)
    {
        swnspeedchange(speed, rest...);
    }
}

// -20 moves up one row, 20 moves down one row
void entityclass::swnmove(int amount)
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23)
        {
            entities[i].yp = std::max(48, std::min(entities[i].yp + amount, 168));
        }
    }
}
// -20 moves up one row, 20 moves down one row
template <typename... Rest>
void entityclass::swnmove(int amount, int id, Rest... rest)
{
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == 23 && entities[i].id == id)
        {
            entities[i].yp = std::max(48, std::min(entities[i].yp + amount, 168));
        }
    }

    if (sizeof...(rest) != 0)
    {
        swnmove(amount, rest...);
    }
}

void entityclass::generateswnwave( int t )
{
    //generate a wave for the SWN game
    if (game.swndelay <= 0)
    {
        if (t == 0)   //game 0, survive for 30 seconds
        {
            switch(game.swnstate)
            {
            case 0:
                //Decide on a wave here
                //default case
                game.swnstate = 1;
                game.swndelay = 5;

                if (game.swntimer <= 150)   //less than 5 seconds
                {
                    game.swnstate = 9;
                    game.swndelay = 8;
                }
                else    if (game.swntimer <= 300)    //less than 10 seconds
                {
                    game.swnstate = 6;
                    game.swndelay = 12;
                }
                else    if (game.swntimer <= 360)    //less than 12 seconds
                {
                    game.swnstate = 5+game.swnstate2;
                    game.swndelay = 15;
                }
                else    if (game.swntimer <= 420)    //less than 14 seconds
                {
                    game.swnstate = 7+game.swnstate2;
                    game.swndelay = 15;
                }
                else    if (game.swntimer <= 480)    //less than 16 seconds
                {
                    game.swnstate = 5+game.swnstate2;
                    game.swndelay = 15;
                }
                else    if (game.swntimer <= 540)    //less than 18 seconds
                {
                    game.swnstate = 7+game.swnstate2;
                    game.swndelay = 15;
                }
                else    if (game.swntimer <= 600)    //less than 20 seconds
                {
                    game.swnstate = 5+game.swnstate2;
                    game.swndelay = 15;
                }
                else    if (game.swntimer <= 900)    //less than 30 seconds
                {
                    game.swnstate = 4;
                    game.swndelay = 20;
                }
                else    if (game.swntimer <= 1050)    //less than 35 seconds
                {
                    game.swnstate = 3;
                    game.swndelay = 10;
                }
                else    if (game.swntimer <= 1200)    //less than 40 seconds
                {
                    game.swnstate = 3;
                    game.swndelay = 20;
                }
                else    if (game.swntimer <= 1500)    //less than 50 seconds
                {
                    game.swnstate = 2;
                    game.swndelay = 10;
                }
                else    if (game.swntimer <= 1650)    //less than 55 seconds
                {
                    game.swnstate = 1;
                    game.swndelay = 15;
                }
                else    if (game.swntimer <= 1800)    //less than 60 seconds
                {
                    game.swnstate = 1;
                    game.swndelay = 25;
                }

                if (game.deathcounts - game.swndeaths > 7) game.swndelay += 2;
                if (game.deathcounts - game.swndeaths > 15) game.swndelay += 2;
                if (game.deathcounts - game.swndeaths > 25) game.swndelay += 4;
                break;
            case 1:
                createentity(-150, 58 + (int(xoshiro_rand() * 6) * 20), 23, 0, 0);
                game.swnstate = 0;
                game.swndelay = 0; //return to decision state
                break;
            case 2:
                if(game.swnstate3==0)
                {
                    game.swnstate2++;
                    if (game.swnstate2 >= 6)
                    {
                        game.swnstate3 = 1;
                        game.swnstate2--;
                    }
                }
                else
                {
                    game.swnstate2--;
                    if (game.swnstate2 < 0)
                    {
                        game.swnstate3 = 0;
                        game.swnstate2++;
                    }
                }
                createentity(-150, 58 + (int(game.swnstate2) * 20), 23, 0, 0);
                game.swnstate = 0;
                game.swndelay = 0; //return to decision state
                break;
            case 3:
                createentity(320+150, 58 + (int(xoshiro_rand() * 6) * 20), 23, 1, 0);
                game.swnstate = 0;
                game.swndelay = 0; //return to decision state
                break;
            case 4:
                //left and right compliments
                game.swnstate2 = int(xoshiro_rand() * 6);
                createentity(-150, 58 + (game.swnstate2  * 20), 23, 0, 0);
                createentity(320+150, 58 + ((5-game.swnstate2) * 20), 23, 1, 0);
                game.swnstate = 0;
                game.swndelay = 0; //return to decision state
                game.swnstate2 = 0;
                break;
            case 5:
                //Top and bottom
                createentity(-150, 58, 23, 0, 0);
                createentity(-150, 58 + (5 * 20), 23, 0, 0);
                game.swnstate = 0;
                game.swndelay = 0; //return to decision state
                game.swnstate2 = 1;
                break;
            case 6:
                //Middle
                createentity(-150, 58 + (2 * 20), 23, 0, 0);
                createentity(-150, 58 + (3 * 20), 23, 0, 0);
                game.swnstate = 0;
                game.swndelay = 0; //return to decision state
                game.swnstate2 = 0;
                break;
            case 7:
                //Top and bottom
                createentity(320+150, 58, 23, 1, 0);
                createentity(320+150, 58 + (5 * 20), 23, 1, 0);
                game.swnstate = 0;
                game.swndelay = 0; //return to decision state
                game.swnstate2 = 1;
                break;
            case 8:
                //Middle
                createentity(320+150, 58 + (2 * 20), 23, 1, 0);
                createentity(320+150, 58 + (3 * 20), 23, 1, 0);
                game.swnstate = 0;
                game.swndelay = 0; //return to decision state
                game.swnstate2 = 0;
                break;
            case 9:
                if(game.swnstate3==0)
                {
                    game.swnstate2++;
                    if (game.swnstate2 >= 6)
                    {
                        game.swnstate3 = 1;
                        game.swnstate2--;
                    }
                }
                else
                {
                    game.swnstate2--;
                    if (game.swnstate2 < 0)
                    {
                        game.swnstate3 = 0;
                        game.swnstate2++;
                    }
                }
                createentity(320 + 150, 58 + (int(game.swnstate2) * 20), 23, 1, 0);
                game.swnstate = 0;
                game.swndelay = 0; //return to decision state
                break;
            }
        }
        else if (t == 1)
        {
            //Game 2, super gravitron

            for (int i = 0; i < game.numpatterns; i++)
            {
                if (game.swnpatterns[i].swncase == game.swnstate)
                {
                    game.swnpatternname = game.swnpatterns[i].name;
                    game.swnpatternunlock[i] = 1;
                    break;
                }
            }

            switch (game.swnstate)
            {
                case 0: { 
                    game.swnstate2 = 0;
                    game.swnstate3 = 0;
                    game.swnstate4 = 0;
                    game.swnstate5 = 0;
                    game.swnstate6 = 0;
                    game.swnstate7 = 0;
                    game.swnstate8 = 0;
                    game.swnstate9 = 0;
                    game.swnstate10 = 0;

                    game.swnrand = xoshiro_rand() * (game.common + game.standard + game.unusual + game.rare + game.exotic);

                    if (game.swnrand < game.common)
                    {
                        game.swnstate = game.swncommonpatterns[int(xoshiro_rand() * game.swncommonpatterns[0]) + 1];
                    }
                    else if (game.swnrand < game.common + game.standard)
                    {
                        game.swnstate = game.swnstandardpatterns[int(xoshiro_rand() * game.swnstandardpatterns[0]) + 1];
                    }
                    else if (game.swnrand < game.common + game.standard + game.unusual)
                    {
                        game.swnstate = game.swnunusualpatterns[int(xoshiro_rand() * game.swnunusualpatterns[0]) + 1];
                    }
                    else if (game.swnrand < game.common + game.standard + game.unusual + game.rare)
                    {
                        game.swnstate = game.swnrarepatterns[int(xoshiro_rand() * game.swnrarepatterns[0]) + 1];
                    }
                    else if (game.swnrand < game.common + game.standard + game.unusual + game.rare + game.exotic)
                    {
                        game.swnstate = game.swnexoticpatterns[int(xoshiro_rand() * game.swnexoticpatterns[0]) + 1];
                    }

                    if (game.swnpractice != 0)
                    {
                        game.swnstate = game.swnpractice;

                        if (game.swnranddelay)
                        {
                            game.swndelay = int(xoshiro_rand() * 31);
                            game.swnranddelay = false;
                        }
                    }

                    game.swnbidirectional = int(xoshiro_rand() * 2);
                } break;

                // ------------------------ PASTE PATTERNS HERE ------------------------ //

                case 100: {     
                    // ^v^ //
                    // standard //

                    game.swnbidirectional ? gravcreate(5, 0, 15, 0, 7, 1) : gravcreate(5, 1, 0, 0, 7, 1);
                    game.swnbidirectional ? gravcreate(4, 0, 35, 0, 7, 2) : gravcreate(4, 1, 20, 0, 7, 2);
                    game.swnbidirectional ? gravcreate(3, 0, 55, 0, 7, 3) : gravcreate(3, 1, 40, 0, 7, 3);
                    game.swnbidirectional ? gravcreate(3, 0, 95, 0, 7, 4) : gravcreate(3, 1, 80, 0, 7, 4);
                    game.swnbidirectional ? gravcreate(2, 0, 75, 0, 7, 5) : gravcreate(2, 1, 60, 0, 7, 5);
                    game.swnbidirectional ? gravcreate(4, 0, 115, 0, 7, 6) : gravcreate(4, 1, 100, 0, 7, 6);
                    game.swnbidirectional ? gravcreate(5, 0, 135, 0, 7, 7) : gravcreate(5, 1, 120, 0, 7, 7);
                    game.swnbidirectional ? gravcreate(0, 0, 115, 0, 7, 8) : gravcreate(0, 1, 100, 0, 7, 8);
                    game.swnbidirectional ? gravcreate(1, 0, 135, 0, 7, 9) : gravcreate(1, 1, 120, 0, 7, 9);
                    game.swnbidirectional ? gravcreate(2, 0, 155, 0, 7, 10) : gravcreate(2, 1, 140, 0, 7, 10);
                    game.swnbidirectional ? gravcreate(3, 0, 175, 0, 7, 11) : gravcreate(3, 1, 160, 0, 7, 11);
                    game.swnbidirectional ? gravcreate(2, 0, 195, 0, 7, 12) : gravcreate(2, 1, 180, 0, 7, 12);
                    game.swnbidirectional ? gravcreate(1, 0, 215, 0, 7, 13) : gravcreate(1, 1, 200, 0, 7, 13);
                    game.swnbidirectional ? gravcreate(0, 0, 235, 0, 7, 14) : gravcreate(0, 1, 220, 0, 7, 14);
                    game.swnbidirectional ? gravcreate(5, 0, 215, 0, 7, 15) : gravcreate(5, 1, 200, 0, 7, 15);
                    game.swnbidirectional ? gravcreate(4, 0, 235, 0, 7, 16) : gravcreate(4, 1, 220, 0, 7, 16);
                    game.swnbidirectional ? gravcreate(3, 0, 255, 0, 7, 17) : gravcreate(3, 1, 240, 0, 7, 17);
                    game.swnbidirectional ? gravcreate(2, 0, 275, 0, 7, 18) : gravcreate(2, 1, 260, 0, 7, 18);
                    game.swnbidirectional ? gravcreate(3, 0, 295, 0, 7, 19) : gravcreate(3, 1, 280, 0, 7, 19);
                    game.swnbidirectional ? gravcreate(4, 0, 315, 0, 7, 20) : gravcreate(4, 1, 300, 0, 7, 20);
                    game.swnbidirectional ? gravcreate(5, 0, 335, 0, 7, 21) : gravcreate(5, 1, 320, 0, 7, 21);

                    game.swnstate = 0;
                    game.swndelay = 90;
                } break;

                case 101: {
                    // Cavern //
                    // rare //
                
                    if (game.swnstate2 == 90)
                    {
                        gravcreate(5, 2, 302, 0, 0, 12);
                        gravcreate(4, 2, 302, 0, 0, 11);
                        gravcreate(2, 2, 302, 0, 0, 10);
                        gravcreate(3, 2, 302, 0, 0, 9);
                        gravcreate(1, 2, 302, 0, 0, 8);
                        gravcreate(0, 2, 302, 0, 0, 7);
                        gravcreate(0, 2, 2, 0, 0, 6);
                        gravcreate(1, 2, 2, 0, 0, 5);
                        gravcreate(2, 2, 2, 0, 0, 4);
                        gravcreate(3, 2, 2, 0, 0, 3);
                        gravcreate(4, 2, 2, 0, 0, 2);
                        gravcreate(5, 2, 2, 0, 0, 1);

                        game.swnstate3 = int(xoshiro_rand() * 2); // 0 or 1, right or left

                        for (int i = 0; i < 10; i++)
                        {
                            game.swnstate5 = int(xoshiro_rand() * 3) + 2; // 2, 3, or 4

                            if (int(xoshiro_rand() * 2) == 0) // stalagmite or stalactite
                            {
                                for (int j = 0; j <= game.swnstate5; j++)
                                {
                                    gravcreate(j, game.swnstate3, game.swnstate4, 0, 3);
                                }
                            }
                            else
                            {
                                for (int j = 0; j <= game.swnstate5; j++)
                                {
                                    gravcreate(5 - j, game.swnstate3, game.swnstate4, 0, 3);
                                }
                            }

                            game.swnstate4 += 80;
                        }

                        game.swnstate = 0;
                        game.swndelay = 360;
                    }
                    else
                    {
                        if (game.swnstate2 % 15 == 0)
                        {
                            if (game.swnstate2 % 2 == 0)
                            {
                                game.swnwallwarnings = "302,158,302,138,302,98,302,118,302,78,302,58,2,58,2,78,2,98,2,118,2,138,2,158,";
                            }
                            else
                            {
                                game.swnwallwarnings = "";
                            }
                        }

                        game.swnstate2++;

                        game.swnstate = 101;
                        game.swndelay = 0;
                    }
                } break;

                case 102: {     
                    // Timing //
                    // standard //

                    game.swnbidirectional ? gravcreate(0, 1, 60, 0, 6, 1) : gravcreate(0, 0, 75, 0, 6, 1);
                    game.swnbidirectional ? gravcreate(1, 1, 60, 0, 6, 2) : gravcreate(1, 0, 75, 0, 6, 2);
                    game.swnbidirectional ? gravcreate(2, 1, 60, 0, 6, 3) : gravcreate(2, 0, 75, 0, 6, 3);
                    game.swnbidirectional ? gravcreate(3, 1, 60, 0, 6, 4) : gravcreate(3, 0, 75, 0, 6, 4);
                    game.swnbidirectional ? gravcreate(4, 1, 60, 0, 6, 5) : gravcreate(4, 0, 75, 0, 6, 5);
                    game.swnbidirectional ? gravcreate(5, 1, 60, 0, 6, 6) : gravcreate(5, 0, 75, 0, 6, 6);
                    game.swnbidirectional ? gravcreate(0, 1, 0, 0, 6, 7) : gravcreate(0, 0, 15, 0, 6, 7);
                    game.swnbidirectional ? gravcreate(1, 1, 0, 0, 6, 8) : gravcreate(1, 0, 15, 0, 6, 8);
                    game.swnbidirectional ? gravcreate(2, 1, 0, 0, 6, 9) : gravcreate(2, 0, 15, 0, 6, 9);
                    game.swnbidirectional ? gravcreate(3, 1, 0, 0, 6, 10) : gravcreate(3, 0, 15, 0, 6, 10);
                    game.swnbidirectional ? gravcreate(4, 1, 0, 0, 6, 11) : gravcreate(4, 0, 15, 0, 6, 11);
                    game.swnbidirectional ? gravcreate(5, 1, 0, 0, 6, 12) : gravcreate(5, 0, 15, 0, 6, 12);
                    game.swnbidirectional ? gravcreate(5, 1, 120, 0, 6, 13) : gravcreate(5, 0, 135, 0, 6, 13);
                    game.swnbidirectional ? gravcreate(4, 1, 120, 0, 6, 14) : gravcreate(4, 0, 135, 0, 6, 14);
                    game.swnbidirectional ? gravcreate(2, 1, 120, 0, 6, 15) : gravcreate(2, 0, 135, 0, 6, 15);
                    game.swnbidirectional ? gravcreate(3, 1, 120, 0, 6, 16) : gravcreate(3, 0, 135, 0, 6, 16);
                    game.swnbidirectional ? gravcreate(1, 1, 120, 0, 6, 17) : gravcreate(1, 0, 135, 0, 6, 17);
                    game.swnbidirectional ? gravcreate(0, 1, 120, 0, 6, 18) : gravcreate(0, 0, 135, 0, 6, 18);
                    game.swnbidirectional ? gravcreate(5, 1, 240, 0, 6, 19) : gravcreate(5, 0, 255, 0, 6, 19);
                    game.swnbidirectional ? gravcreate(4, 1, 240, 0, 6, 20) : gravcreate(4, 0, 255, 0, 6, 20);
                    game.swnbidirectional ? gravcreate(3, 1, 240, 0, 6, 21) : gravcreate(3, 0, 255, 0, 6, 21);
                    game.swnbidirectional ? gravcreate(2, 1, 240, 0, 6, 22) : gravcreate(2, 0, 255, 0, 6, 22);
                    game.swnbidirectional ? gravcreate(0, 1, 240, 0, 6, 23) : gravcreate(0, 0, 255, 0, 6, 23);
                    game.swnbidirectional ? gravcreate(1, 1, 240, 0, 6, 24) : gravcreate(1, 0, 255, 0, 6, 24);
                    game.swnbidirectional ? gravcreate(0, 1, 300, 0, 6, 25) : gravcreate(0, 0, 315, 0, 6, 25);
                    game.swnbidirectional ? gravcreate(1, 1, 300, 0, 6, 26) : gravcreate(1, 0, 315, 0, 6, 26);
                    game.swnbidirectional ? gravcreate(3, 1, 300, 0, 6, 27) : gravcreate(3, 0, 315, 0, 6, 27);
                    game.swnbidirectional ? gravcreate(2, 1, 300, 0, 6, 28) : gravcreate(2, 0, 315, 0, 6, 28);
                    game.swnbidirectional ? gravcreate(4, 1, 300, 0, 6, 29) : gravcreate(4, 0, 315, 0, 6, 29);
                    game.swnbidirectional ? gravcreate(5, 1, 300, 0, 6, 30) : gravcreate(5, 0, 315, 0, 6, 30);
                    game.swnbidirectional ? gravcreate(0, 1, 180, 0, 6, 31) : gravcreate(0, 0, 195, 0, 6, 31);
                    game.swnbidirectional ? gravcreate(1, 1, 180, 0, 6, 32) : gravcreate(1, 0, 195, 0, 6, 32);
                    game.swnbidirectional ? gravcreate(2, 1, 180, 0, 6, 33) : gravcreate(2, 0, 195, 0, 6, 33);
                    game.swnbidirectional ? gravcreate(3, 1, 180, 0, 6, 34) : gravcreate(3, 0, 195, 0, 6, 34);
                    game.swnbidirectional ? gravcreate(4, 1, 180, 0, 6, 35) : gravcreate(4, 0, 195, 0, 6, 35);
                    game.swnbidirectional ? gravcreate(5, 1, 180, 0, 6, 36) : gravcreate(5, 0, 195, 0, 6, 36);

                    game.swnstate = 0;
                    game.swndelay = 120;
                } break;

                case 103: {     
                    // Sandwich Wrap //
                    // exotic //

                    if (game.swnstate2 == 90)
                    {
                        game.swnbidirectional ? gravcreate(1, 1, 480, 10, 7, 1) : gravcreate(1, 0, 495, 10, 7, 1);
                        game.swnbidirectional ? gravcreate(3, 1, 480, 10, 7, 2) : gravcreate(3, 0, 495, 10, 7, 2);
                        game.swnbidirectional ? gravcreate(2, 1, 480, 10, 7, 3) : gravcreate(2, 0, 495, 10, 7, 3);
                        game.swnbidirectional ? gravcreate(1, 1, 160, 10, 7, 4) : gravcreate(1, 0, 175, 10, 7, 4);
                        game.swnbidirectional ? gravcreate(3, 1, 160, 10, 7, 5) : gravcreate(3, 0, 175, 10, 7, 5);
                        game.swnbidirectional ? gravcreate(2, 1, 160, 10, 7, 6) : gravcreate(2, 0, 175, 10, 7, 6);
                        game.swnbidirectional ? gravcreate(3, 0, 335, 10, 7, 7) : gravcreate(3, 1, 320, 10, 7, 7);
                        game.swnbidirectional ? gravcreate(1, 0, 335, 10, 7, 8) : gravcreate(1, 1, 320, 10, 7, 8);
                        game.swnbidirectional ? gravcreate(2, 0, 335, 10, 7, 9) : gravcreate(2, 1, 320, 10, 7, 9);
                        game.swnbidirectional ? gravcreate(1, 0, 15, 10, 7, 10) : gravcreate(1, 1, 0, 10, 7, 10);
                        game.swnbidirectional ? gravcreate(3, 0, 15, 10, 7, 11) : gravcreate(3, 1, 0, 10, 7, 11);
                        game.swnbidirectional ? gravcreate(2, 0, 15, 10, 7, 12) : gravcreate(2, 1, 0, 10, 7, 12);
                        game.swnbidirectional ? gravcreate(3, 1, 0, 0, 0, 13) : gravcreate(3, 0, 15, 0, 0, 13);
                        game.swnbidirectional ? gravcreate(5, 2, 152, 0, 0, 14) : gravcreate(5, 2, 152, 0, 0, 14);
                        game.swnbidirectional ? gravcreate(4, 2, 152, 0, 0, 15) : gravcreate(4, 2, 152, 0, 0, 15);
                        game.swnbidirectional ? gravcreate(3, 2, 152, 0, 0, 16) : gravcreate(3, 2, 152, 0, 0, 16);
                        game.swnbidirectional ? gravcreate(1, 2, 152, 0, 0, 17) : gravcreate(1, 2, 152, 0, 0, 17);
                        game.swnbidirectional ? gravcreate(2, 2, 152, 0, 0, 18) : gravcreate(2, 2, 152, 0, 0, 18);
                        game.swnbidirectional ? gravcreate(0, 2, 152, 0, 0, 19) : gravcreate(0, 2, 152, 0, 0, 19);

                        game.swnstate = 0;
                        game.swndelay = 120;
                    }
                    else
                    {
                        if (game.swnstate2 % 15 == 0)
                        {
                            if (game.swnstate2 % 2 == 0)
                            {
                                game.swnwallwarnings = game.swnbidirectional ? "152,158,152,138,152,118,152,78,152,98,152,58," : "152,158,152,138,152,118,152,78,152,98,152,58,";
                            }
                            else
                            {
                                game.swnwallwarnings = "";
                            }
                        }

                        game.swnstate2++;

                        game.swnstate = 103;
                        game.swndelay = 0;
                    }
                } break;

                case 104: {     
                    // Conveyor //
                    // rare //

                    if (game.swnstate2 == 90)
                    {
                        gravcreate(5, 2, 302, 0, 0, 1);
                        gravcreate(5, 2, 182, 0, 0, 2);
                        gravcreate(5, 2, 62, 0, 0, 3);
                        gravcreate(5, 2, 242, 0, 0, 4);
                        gravcreate(5, 2, 122, 0, 0, 5);
                        gravcreate(5, 2, 2, 0, 0, 6);
                        gravcreate(4, 0, 755, 0, 3, 7);
                        gravcreate(3, 0, 755, 0, 3, 8);
                        gravcreate(2, 0, 755, 0, 3, 9);
                        gravcreate(2, 0, 735, 0, 3, 10);
                        gravcreate(3, 0, 735, 0, 3, 11);
                        gravcreate(4, 0, 735, 0, 3, 12);
                        gravcreate(4, 0, 715, 0, 3, 13);
                        gravcreate(4, 0, 615, 0, 3, 14);
                        gravcreate(3, 0, 615, 0, 3, 15);
                        gravcreate(3, 0, 575, 0, 3, 16);
                        gravcreate(3, 0, 595, 0, 3, 17);
                        gravcreate(4, 0, 595, 0, 3, 18);
                        gravcreate(4, 0, 575, 0, 3, 19);
                        gravcreate(2, 0, 575, 0, 3, 20);
                        gravcreate(4, 0, 475, 0, 3, 21);
                        gravcreate(3, 0, 475, 0, 3, 22);
                        gravcreate(3, 0, 455, 0, 3, 23);
                        gravcreate(4, 0, 455, 0, 3, 24);
                        gravcreate(4, 0, 435, 0, 3, 25);
                        gravcreate(2, 0, 455, 0, 3, 26);
                        gravcreate(4, 0, 335, 0, 3, 27);
                        gravcreate(3, 0, 335, 0, 3, 28);
                        gravcreate(3, 0, 315, 0, 3, 29);
                        gravcreate(3, 0, 295, 0, 3, 30);
                        gravcreate(4, 0, 295, 0, 3, 31);
                        gravcreate(4, 0, 315, 0, 3, 32);
                        gravcreate(2, 0, 315, 0, 3, 33);
                        gravcreate(4, 0, 195, 0, 3, 34);
                        gravcreate(4, 0, 175, 0, 3, 35);
                        gravcreate(3, 0, 175, 0, 3, 36);
                        gravcreate(2, 0, 175, 0, 3, 37);
                        gravcreate(3, 0, 155, 0, 3, 38);
                        gravcreate(4, 0, 155, 0, 3, 39);
                        gravcreate(4, 0, 55, 0, 3, 40);
                        gravcreate(4, 0, 35, 0, 3, 41);
                        gravcreate(4, 0, 15, 0, 3, 42);
                        gravcreate(3, 0, 35, 0, 3, 43);
                        gravcreate(3, 0, 15, 0, 3, 44);
                        gravcreate(3, 0, 55, 0, 3, 45);
                        gravcreate(2, 0, 35, 0, 3, 46);
                        gravcreate(2, 0, 15, 0, 3, 47);
                        gravcreate(4, 0, 855, 0, 3, 48);
                        gravcreate(3, 0, 855, 0, 3, 49);
                        gravcreate(2, 0, 855, 0, 3, 50);
                        gravcreate(3, 0, 875, 0, 3, 51);
                        gravcreate(4, 0, 875, 0, 3, 52);
                        gravcreate(4, 0, 895, 0, 3, 53);
                        gravcreate(3, 0, 895, 0, 3, 54);
                        gravcreate(2, 0, 895, 0, 3, 55);

                        game.swnstate = 0;
                        game.swndelay = 450;
                    }
                    else
                    {
                        if (game.swnstate2 % 15 == 0)
                        {
                            if (game.swnstate2 % 2 == 0)
                            {
                                game.swnwallwarnings = "302,158,182,158,62,158,242,158,122,158,2,158,";
                            }
                            else
                            {
                                game.swnwallwarnings = "";
                            }
                        }

                        game.swnstate2++;

                        game.swnstate = 104;
                        game.swndelay = 0;
                    }
                } break;

                case 105: {     
                    // YaY //
                    // unusual //

                    if (game.swnstate2 == 90)
                    {
                        gravcreate(5, 2, 292, 0, 0, 1);
                        gravcreate(4, 2, 292, 0, 0, 2);
                        gravcreate(3, 2, 292, 0, 0, 3);
                        gravcreate(2, 2, 312, 0, 0, 4);
                        gravcreate(3, 2, 12, 0, 0, 5);
                        gravcreate(4, 2, 12, 0, 0, 6);
                        gravcreate(5, 2, 12, 0, 0, 7);
                        gravcreate(2, 2, 152, 0, 0, 8);
                        gravcreate(5, 2, 172, 0, 0, 9);
                        gravcreate(4, 2, 172, 0, 0, 10);
                        gravcreate(5, 2, 132, 0, 0, 11);
                        gravcreate(4, 2, 132, 0, 0, 12);
                        gravcreate(3, 2, 172, 0, 0, 13);
                        gravcreate(3, 2, 132, 0, 0, 14);
                        gravcreate(0, 2, 32, 0, 0, 15);
                        gravcreate(1, 2, 52, 0, 0, 16);
                        gravcreate(0, 2, 272, 0, 0, 17);
                        gravcreate(1, 2, 252, 0, 0, 18);
                        gravcreate(5, 2, 232, 0, 0, 19);
                        gravcreate(3, 2, 232, 0, 0, 20);
                        gravcreate(4, 2, 232, 0, 0, 21);
                        gravcreate(2, 2, 232, 0, 0, 22);
                        gravcreate(1, 2, 212, 0, 0, 23);
                        gravcreate(0, 2, 192, 0, 0, 24);
                        gravcreate(0, 2, 112, 0, 0, 25);
                        gravcreate(1, 2, 92, 0, 0, 26);
                        gravcreate(2, 2, 72, 0, 0, 27);
                        gravcreate(3, 2, 72, 0, 0, 28);
                        gravcreate(4, 2, 72, 0, 0, 29);
                        gravcreate(5, 2, 72, 0, 0, 30);
                        gravcreate(4, 2, 152, 0, 0, 31);
                        gravcreate(4, 2, 312, 0, 0, 32);
                        gravcreate(4, 2, -8, 0, 0, 33);
                        gravcreate(2, 2, -8, 0, 0, 34);

                        game.swnstate = 0;
                        game.swndelay = 150;
                    }
                    else
                    {
                        if (game.swnstate2 % 15 == 0)
                        {
                            if (game.swnstate2 % 2 == 0)
                            {
                                game.swnwallwarnings = "292,158,292,138,292,118,312,98,12,118,12,138,12,158,152,98,172,158,172,138,132,158,132,138,172,118,132,118,32,58,52,78,272,58,252,78,232,158,232,118,232,138,232,98,212,78,192,58,112,58,92,78,72,98,72,118,72,138,72,158,152,138,312,138,-8,138,-8,98,";
                            }
                            else
                            {
                                game.swnwallwarnings = "";
                            }
                        }

                        game.swnstate2++;

                        game.swnstate = 105;
                        game.swndelay = 0;
                    }
                } break;

                case 106: {     
                    // Red Light Green Light //
                    // exotic //

                    if (game.swnstate2 == 0)
                    {
                        game.swnbidirectional ? gravcreate(4, 0, 355, 0, 6, 1) : gravcreate(4, 1, 340, 0, 6, 1);
                        game.swnbidirectional ? gravcreate(3, 0, 355, 0, 6, 2) : gravcreate(3, 1, 340, 0, 6, 2);
                        game.swnbidirectional ? gravcreate(2, 0, 355, 0, 6, 3) : gravcreate(2, 1, 340, 0, 6, 3);
                        game.swnbidirectional ? gravcreate(1, 0, 355, 0, 6, 4) : gravcreate(1, 1, 340, 0, 6, 4);
                        game.swnbidirectional ? gravcreate(0, 0, 355, 0, 6, 5) : gravcreate(0, 1, 340, 0, 6, 5);
                        game.swnbidirectional ? gravcreate(5, 0, 355, 0, 6, 6) : gravcreate(5, 1, 340, 0, 6, 6);
                        game.swnbidirectional ? gravcreate(0, 0, 55, 0, 6, 7) : gravcreate(0, 1, 40, 0, 6, 7);
                        game.swnbidirectional ? gravcreate(1, 0, 55, 0, 6, 8) : gravcreate(1, 1, 40, 0, 6, 8);
                        game.swnbidirectional ? gravcreate(2, 0, 55, 0, 6, 9) : gravcreate(2, 1, 40, 0, 6, 9);
                        game.swnbidirectional ? gravcreate(3, 0, 55, 0, 6, 10) : gravcreate(3, 1, 40, 0, 6, 10);
                        game.swnbidirectional ? gravcreate(5, 0, 55, 0, 6, 11) : gravcreate(5, 1, 40, 0, 6, 11);
                        game.swnbidirectional ? gravcreate(4, 0, 55, 0, 6, 12) : gravcreate(4, 1, 40, 0, 6, 12);
                        game.swnbidirectional ? gravcreate(5, 0, 75, 0, 6, 13) : gravcreate(5, 1, 60, 0, 6, 13);
                        game.swnbidirectional ? gravcreate(5, 0, 95, 0, 6, 14) : gravcreate(5, 1, 80, 0, 6, 14);
                        game.swnbidirectional ? gravcreate(4, 0, 95, 0, 6, 15) : gravcreate(4, 1, 80, 0, 6, 15);
                        game.swnbidirectional ? gravcreate(4, 0, 75, 0, 6, 16) : gravcreate(4, 1, 60, 0, 6, 16);
                        game.swnbidirectional ? gravcreate(2, 0, 95, 0, 6, 17) : gravcreate(2, 1, 80, 0, 6, 17);
                        game.swnbidirectional ? gravcreate(3, 0, 95, 0, 6, 18) : gravcreate(3, 1, 80, 0, 6, 18);
                        game.swnbidirectional ? gravcreate(3, 0, 75, 0, 6, 19) : gravcreate(3, 1, 60, 0, 6, 19);
                        game.swnbidirectional ? gravcreate(2, 0, 75, 0, 6, 20) : gravcreate(2, 1, 60, 0, 6, 20);
                        game.swnbidirectional ? gravcreate(1, 0, 75, 0, 6, 21) : gravcreate(1, 1, 60, 0, 6, 21);
                        game.swnbidirectional ? gravcreate(1, 0, 95, 0, 6, 22) : gravcreate(1, 1, 80, 0, 6, 22);
                        game.swnbidirectional ? gravcreate(0, 0, 95, 0, 6, 23) : gravcreate(0, 1, 80, 0, 6, 23);
                        game.swnbidirectional ? gravcreate(0, 0, 75, 0, 6, 24) : gravcreate(0, 1, 60, 0, 6, 24);
                        game.swnbidirectional ? gravcreate(0, 0, 115, 0, 6, 25) : gravcreate(0, 1, 100, 0, 6, 25);
                        game.swnbidirectional ? gravcreate(0, 0, 155, 0, 6, 26) : gravcreate(0, 1, 140, 0, 6, 26);
                        game.swnbidirectional ? gravcreate(1, 0, 135, 0, 6, 27) : gravcreate(1, 1, 120, 0, 6, 27);
                        game.swnbidirectional ? gravcreate(0, 0, 135, 0, 6, 28) : gravcreate(0, 1, 120, 0, 6, 28);
                        game.swnbidirectional ? gravcreate(1, 0, 155, 0, 6, 29) : gravcreate(1, 1, 140, 0, 6, 29);
                        game.swnbidirectional ? gravcreate(1, 0, 115, 0, 6, 30) : gravcreate(1, 1, 100, 0, 6, 30);
                        game.swnbidirectional ? gravcreate(2, 0, 115, 0, 6, 31) : gravcreate(2, 1, 100, 0, 6, 31);
                        game.swnbidirectional ? gravcreate(2, 0, 135, 0, 6, 32) : gravcreate(2, 1, 120, 0, 6, 32);
                        game.swnbidirectional ? gravcreate(2, 0, 155, 0, 6, 33) : gravcreate(2, 1, 140, 0, 6, 33);
                        game.swnbidirectional ? gravcreate(3, 0, 155, 0, 6, 34) : gravcreate(3, 1, 140, 0, 6, 34);
                        game.swnbidirectional ? gravcreate(3, 0, 115, 0, 6, 35) : gravcreate(3, 1, 100, 0, 6, 35);
                        game.swnbidirectional ? gravcreate(3, 0, 135, 0, 6, 36) : gravcreate(3, 1, 120, 0, 6, 36);
                        game.swnbidirectional ? gravcreate(4, 0, 135, 0, 6, 37) : gravcreate(4, 1, 120, 0, 6, 37);
                        game.swnbidirectional ? gravcreate(4, 0, 155, 0, 6, 38) : gravcreate(4, 1, 140, 0, 6, 38);
                        game.swnbidirectional ? gravcreate(5, 0, 155, 0, 6, 39) : gravcreate(5, 1, 140, 0, 6, 39);
                        game.swnbidirectional ? gravcreate(5, 0, 135, 0, 6, 40) : gravcreate(5, 1, 120, 0, 6, 40);
                        game.swnbidirectional ? gravcreate(5, 0, 115, 0, 6, 41) : gravcreate(5, 1, 100, 0, 6, 41);
                        game.swnbidirectional ? gravcreate(4, 0, 115, 0, 6, 42) : gravcreate(4, 1, 100, 0, 6, 42);
                        game.swnbidirectional ? gravcreate(5, 0, 195, 0, 6, 43) : gravcreate(5, 1, 180, 0, 6, 43);
                        game.swnbidirectional ? gravcreate(5, 0, 175, 0, 6, 44) : gravcreate(5, 1, 160, 0, 6, 44);
                        game.swnbidirectional ? gravcreate(4, 0, 175, 0, 6, 45) : gravcreate(4, 1, 160, 0, 6, 45);
                        game.swnbidirectional ? gravcreate(4, 0, 195, 0, 6, 46) : gravcreate(4, 1, 180, 0, 6, 46);
                        game.swnbidirectional ? gravcreate(4, 0, 215, 0, 6, 47) : gravcreate(4, 1, 200, 0, 6, 47);
                        game.swnbidirectional ? gravcreate(5, 0, 215, 0, 6, 48) : gravcreate(5, 1, 200, 0, 6, 48);
                        game.swnbidirectional ? gravcreate(5, 0, 235, 0, 6, 49) : gravcreate(5, 1, 220, 0, 6, 49);
                        game.swnbidirectional ? gravcreate(4, 0, 235, 0, 6, 50) : gravcreate(4, 1, 220, 0, 6, 50);
                        game.swnbidirectional ? gravcreate(4, 0, 255, 0, 6, 51) : gravcreate(4, 1, 240, 0, 6, 51);
                        game.swnbidirectional ? gravcreate(5, 0, 255, 0, 6, 52) : gravcreate(5, 1, 240, 0, 6, 52);
                        game.swnbidirectional ? gravcreate(5, 0, 275, 0, 6, 53) : gravcreate(5, 1, 260, 0, 6, 53);
                        game.swnbidirectional ? gravcreate(4, 0, 275, 0, 6, 54) : gravcreate(4, 1, 260, 0, 6, 54);
                        game.swnbidirectional ? gravcreate(3, 0, 275, 0, 6, 55) : gravcreate(3, 1, 260, 0, 6, 55);
                        game.swnbidirectional ? gravcreate(2, 0, 275, 0, 6, 56) : gravcreate(2, 1, 260, 0, 6, 56);
                        game.swnbidirectional ? gravcreate(1, 0, 275, 0, 6, 57) : gravcreate(1, 1, 260, 0, 6, 57);
                        game.swnbidirectional ? gravcreate(0, 0, 275, 0, 6, 58) : gravcreate(0, 1, 260, 0, 6, 58);
                        game.swnbidirectional ? gravcreate(3, 0, 255, 0, 6, 59) : gravcreate(3, 1, 240, 0, 6, 59);
                        game.swnbidirectional ? gravcreate(3, 0, 235, 0, 6, 60) : gravcreate(3, 1, 220, 0, 6, 60);
                        game.swnbidirectional ? gravcreate(3, 0, 215, 0, 6, 61) : gravcreate(3, 1, 200, 0, 6, 61);
                        game.swnbidirectional ? gravcreate(3, 0, 195, 0, 6, 62) : gravcreate(3, 1, 180, 0, 6, 62);
                        game.swnbidirectional ? gravcreate(3, 0, 175, 0, 6, 63) : gravcreate(3, 1, 160, 0, 6, 63);
                        game.swnbidirectional ? gravcreate(2, 0, 175, 0, 6, 64) : gravcreate(2, 1, 160, 0, 6, 64);
                        game.swnbidirectional ? gravcreate(2, 0, 195, 0, 6, 65) : gravcreate(2, 1, 180, 0, 6, 65);
                        game.swnbidirectional ? gravcreate(2, 0, 215, 0, 6, 66) : gravcreate(2, 1, 200, 0, 6, 66);
                        game.swnbidirectional ? gravcreate(2, 0, 235, 0, 6, 67) : gravcreate(2, 1, 220, 0, 6, 67);
                        game.swnbidirectional ? gravcreate(2, 0, 255, 0, 6, 68) : gravcreate(2, 1, 240, 0, 6, 68);
                        game.swnbidirectional ? gravcreate(1, 0, 255, 0, 6, 69) : gravcreate(1, 1, 240, 0, 6, 69);
                        game.swnbidirectional ? gravcreate(1, 0, 235, 0, 6, 70) : gravcreate(1, 1, 220, 0, 6, 70);
                        game.swnbidirectional ? gravcreate(0, 0, 235, 0, 6, 71) : gravcreate(0, 1, 220, 0, 6, 71);
                        game.swnbidirectional ? gravcreate(0, 0, 255, 0, 6, 72) : gravcreate(0, 1, 240, 0, 6, 72);
                        game.swnbidirectional ? gravcreate(0, 0, 215, 0, 6, 73) : gravcreate(0, 1, 200, 0, 6, 73);
                        game.swnbidirectional ? gravcreate(1, 0, 215, 0, 6, 74) : gravcreate(1, 1, 200, 0, 6, 74);
                        game.swnbidirectional ? gravcreate(1, 0, 195, 0, 6, 75) : gravcreate(1, 1, 180, 0, 6, 75);
                        game.swnbidirectional ? gravcreate(0, 0, 195, 0, 6, 76) : gravcreate(0, 1, 180, 0, 6, 76);
                        game.swnbidirectional ? gravcreate(1, 0, 175, 0, 6, 77) : gravcreate(1, 1, 160, 0, 6, 77);
                        game.swnbidirectional ? gravcreate(0, 0, 175, 0, 6, 78) : gravcreate(0, 1, 160, 0, 6, 78);
                        game.swnbidirectional ? gravcreate(5, 0, 35, 0, 6, 79) : gravcreate(5, 1, 20, 0, 6, 79);
                        game.swnbidirectional ? gravcreate(5, 0, 15, 0, 6, 80) : gravcreate(5, 1, 0, 0, 6, 80);
                        game.swnbidirectional ? gravcreate(4, 0, 15, 0, 6, 81) : gravcreate(4, 1, 0, 0, 6, 81);
                        game.swnbidirectional ? gravcreate(4, 0, 35, 0, 6, 82) : gravcreate(4, 1, 20, 0, 6, 82);
                        game.swnbidirectional ? gravcreate(3, 0, 35, 0, 6, 83) : gravcreate(3, 1, 20, 0, 6, 83);
                        game.swnbidirectional ? gravcreate(3, 0, 15, 0, 6, 84) : gravcreate(3, 1, 0, 0, 6, 84);
                        game.swnbidirectional ? gravcreate(2, 0, 15, 0, 6, 85) : gravcreate(2, 1, 0, 0, 6, 85);
                        game.swnbidirectional ? gravcreate(2, 0, 35, 0, 6, 86) : gravcreate(2, 1, 20, 0, 6, 86);
                        game.swnbidirectional ? gravcreate(1, 0, 35, 0, 6, 87) : gravcreate(1, 1, 20, 0, 6, 87);
                        game.swnbidirectional ? gravcreate(1, 0, 15, 0, 6, 88) : gravcreate(1, 1, 0, 0, 6, 88);
                        game.swnbidirectional ? gravcreate(0, 0, 15, 0, 6, 89) : gravcreate(0, 1, 0, 0, 6, 89);
                        game.swnbidirectional ? gravcreate(0, 0, 35, 0, 6, 90) : gravcreate(0, 1, 20, 0, 6, 90);
                    }

                    game.swnstate2++;

                    if (game.swnstate2 < 92)
                    {
                        if (game.swnstate2 >= 86)
                        {
                            if (game.swnstate3 == 0)
                            {
                                game.swnstate3 = 1;

                                swnfreeze();

                                game.swndelay = 30;
                            }
                            else
                            {
                                game.swnstate3 = 0;

                                swnunfreeze();

                                game.swndelay = 15;
                            }
                        }
                        else
                        {
                            game.swndelay = 0;
                        }

                        game.swnstate = 106;
                    }
                    else
                    {
                        game.swnstate = 0;
                        game.swndelay = 0;
                    }
                } break;
                        
                case 107: {     
                    // Bug March //
                    // exotic //

                    if (game.swnstate2 == 0)
                    {
                        game.swnbidirectional ? gravcreate(0, 0, 415, 0, 7, 1) : gravcreate(0, 1, 400, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(5, 0, 315, 0, 7, 3) : gravcreate(5, 1, 300, 0, 7, 3);
                        game.swnbidirectional ? gravcreate(0, 0, 215, 0, 7, 3) : gravcreate(0, 1, 200, 0, 7, 3);
                        game.swnbidirectional ? gravcreate(5, 0, 115, 0, 7, 3) : gravcreate(5, 1, 100, 0, 7, 3);
                        game.swnbidirectional ? gravcreate(0, 0, 15, 0, 7, 3) : gravcreate(0, 1, 0, 0, 7, 3);
                        game.swnbidirectional ? gravcreate(5, 0, 115, 0, 7, 1) : gravcreate(5, 1, 100, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(0, 0, 215, 0, 7, 1) : gravcreate(0, 1, 200, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(5, 0, 315, 0, 7, 1) : gravcreate(5, 1, 300, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(3, 0, 65, -10, 7, 2) : gravcreate(3, 1, 50, -10, 7, 2);
                        game.swnbidirectional ? gravcreate(3, 0, 165, -10, 7, 2) : gravcreate(3, 1, 150, -10, 7, 2);
                        game.swnbidirectional ? gravcreate(3, 0, 265, -10, 7, 2) : gravcreate(3, 1, 250, -10, 7, 2);
                        game.swnbidirectional ? gravcreate(3, 0, 365, -10, 7, 2) : gravcreate(3, 1, 350, -10, 7, 2);
                        game.swnbidirectional ? gravcreate(0, 0, 615, 0, 7, 1) : gravcreate(0, 1, 600, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(5, 0, 515, 0, 7, 1) : gravcreate(5, 1, 500, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(0, 0, 415, 0, 7, 3) : gravcreate(0, 1, 400, 0, 7, 3);
                        game.swnbidirectional ? gravcreate(5, 0, 515, 0, 7, 3) : gravcreate(5, 1, 500, 0, 7, 3);
                        game.swnbidirectional ? gravcreate(3, 0, 465, -10, 7, 2) : gravcreate(3, 1, 450, -10, 7, 2);
                        game.swnbidirectional ? gravcreate(3, 0, 565, -10, 7, 2) : gravcreate(3, 1, 550, -10, 7, 2);
                        game.swnbidirectional ? gravcreate(0, 0, 615, 0, 7, 3) : gravcreate(0, 1, 600, 0, 7, 3);
                        game.swnbidirectional ? gravcreate(5, 0, 715, 0, 7, 1) : gravcreate(5, 1, 700, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(3, 0, 665, -10, 7, 2) : gravcreate(3, 1, 650, -10, 7, 2);
                        
                        swnspeedchange(4, 1, 3);
                        swnspeedchange(2, 2);
                    }

                    if (game.swnstate2 < 570)
                    {
                        if (game.swnstate2 % 50 == 0)
                        {
                            if (game.swnstate3 == 0)
                            {
                                game.swnstate3 = 1;

                                swnfreeze(3);
                                swnunfreeze(1);
                            }
                            else
                            {
                                game.swnstate3 = 0;

                                swnfreeze(1);
                                swnunfreeze(3);
                            }
                        }

                        game.swnstate = 107;
                    }
                    else
                    {
                        game.swnstate = 0;
                    }

                    game.swnstate2++;
                    game.swndelay = 0;
                } break;

                case 108: {     
                    // Hunted //
                    // common //

                    if (game.swnstate2 == 90)
                    {
                        game.swnhomingtimer = 105;
                        game.swnbidirectional ? gravcreate(2, 3, -98, 10, 7, 1) : gravcreate(2, 3, 402, 10, 7, 1);
                        game.swnbidirectional ? gravcreate(2, 2, 102, 0, 0, 2) : gravcreate(2, 2, 202, 0, 0, 2);
                        game.swnbidirectional ? gravcreate(2, 2, 202, 0, 0, 3) : gravcreate(2, 2, 102, 0, 0, 3);
                        game.swnbidirectional ? gravcreate(2, 2, 2, 0, 0, 4) : gravcreate(2, 2, 302, 0, 0, 4);
                        game.swnbidirectional ? gravcreate(2, 2, 302, 0, 0, 5) : gravcreate(2, 2, 2, 0, 0, 5);
                        game.swnbidirectional ? gravcreate(0, 2, 302, 0, 0, 6) : gravcreate(0, 2, 2, 0, 0, 6);
                        game.swnbidirectional ? gravcreate(1, 2, 302, 0, 0, 7) : gravcreate(1, 2, 2, 0, 0, 7);
                        game.swnbidirectional ? gravcreate(4, 2, 302, 0, 0, 8) : gravcreate(4, 2, 2, 0, 0, 8);
                        game.swnbidirectional ? gravcreate(3, 2, 302, 0, 0, 9) : gravcreate(3, 2, 2, 0, 0, 9);
                        game.swnbidirectional ? gravcreate(5, 2, 302, 0, 0, 10) : gravcreate(5, 2, 2, 0, 0, 10);
                        game.swnbidirectional ? gravcreate(5, 2, 202, 0, 0, 11) : gravcreate(5, 2, 102, 0, 0, 11);
                        game.swnbidirectional ? gravcreate(4, 2, 202, 0, 0, 12) : gravcreate(4, 2, 102, 0, 0, 12);
                        game.swnbidirectional ? gravcreate(3, 2, 202, 0, 0, 13) : gravcreate(3, 2, 102, 0, 0, 13);
                        game.swnbidirectional ? gravcreate(1, 2, 202, 0, 0, 14) : gravcreate(1, 2, 102, 0, 0, 14);
                        game.swnbidirectional ? gravcreate(0, 2, 202, 0, 0, 15) : gravcreate(0, 2, 102, 0, 0, 15);
                        game.swnbidirectional ? gravcreate(0, 2, 102, 0, 0, 16) : gravcreate(0, 2, 202, 0, 0, 16);
                        game.swnbidirectional ? gravcreate(1, 2, 102, 0, 0, 17) : gravcreate(1, 2, 202, 0, 0, 17);
                        game.swnbidirectional ? gravcreate(3, 2, 102, 0, 0, 18) : gravcreate(3, 2, 202, 0, 0, 18);
                        game.swnbidirectional ? gravcreate(4, 2, 102, 0, 0, 19) : gravcreate(4, 2, 202, 0, 0, 19);
                        game.swnbidirectional ? gravcreate(5, 2, 102, 0, 0, 20) : gravcreate(5, 2, 202, 0, 0, 20);
                        game.swnbidirectional ? gravcreate(5, 2, 2, 0, 0, 21) : gravcreate(5, 2, 302, 0, 0, 21);
                        game.swnbidirectional ? gravcreate(4, 2, 2, 0, 0, 22) : gravcreate(4, 2, 302, 0, 0, 22);
                        game.swnbidirectional ? gravcreate(3, 2, 2, 0, 0, 23) : gravcreate(3, 2, 302, 0, 0, 23);
                        game.swnbidirectional ? gravcreate(0, 2, 2, 0, 0, 24) : gravcreate(0, 2, 302, 0, 0, 24);
                        game.swnbidirectional ? gravcreate(1, 2, 2, 0, 0, 25) : gravcreate(1, 2, 302, 0, 0, 25);

                        game.swnstate = 0;
                        game.swndelay = 105;
                    }
                    else
                    {
                        if (game.swnstate2 % 15 == 0)
                        {
                            if (game.swnstate2 % 2 == 0)
                            {
                                game.swnwallwarnings = game.swnbidirectional ? "102,98,202,98,2,98,302,98,302,58,302,78,302,138,302,118,302,158,202,158,202,138,202,118,202,78,202,58,102,58,102,78,102,118,102,138,102,158,2,158,2,138,2,118,2,58,2,78," : "202,98,102,98,302,98,2,98,2,58,2,78,2,138,2,118,2,158,102,158,102,138,102,118,102,78,102,58,202,58,202,78,202,118,202,138,202,158,302,158,302,138,302,118,302,58,302,78,";
                            }
                            else
                            {
                                game.swnwallwarnings = "";
                            }
                        }

                        game.swnstate2++;

                        game.swnstate = 108;
                        game.swndelay = 0;
                    }
                } break;

                case 109: {     
                    // Stars //
                    // common //

                    game.swnbidirectional ? gravcreate(3, 0, 35, 10, 7, 1) : gravcreate(3, 1, 20, 10, 7, 1);
                    game.swnbidirectional ? gravcreate(2, 0, 15, 10, 7, 2) : gravcreate(2, 1, 0, 10, 7, 2);
                    game.swnbidirectional ? gravcreate(1, 0, 35, 10, 7, 3) : gravcreate(1, 1, 20, 10, 7, 3);
                    game.swnbidirectional ? gravcreate(2, 0, 55, 10, 7, 4) : gravcreate(2, 1, 40, 10, 7, 4);
                    game.swnbidirectional ? gravcreate(1, 0, 95, -10, 7, 5) : gravcreate(1, 1, 80, -10, 7, 5);
                    game.swnbidirectional ? gravcreate(0, 0, 115, -10, 7, 6) : gravcreate(0, 1, 100, -10, 7, 6);
                    game.swnbidirectional ? gravcreate(2, 0, 115, -10, 7, 7) : gravcreate(2, 1, 100, -10, 7, 7);
                    game.swnbidirectional ? gravcreate(1, 0, 135, -10, 7, 8) : gravcreate(1, 1, 120, -10, 7, 8);
                    game.swnbidirectional ? gravcreate(5, 0, 155, -10, 7, 9) : gravcreate(5, 1, 140, -10, 7, 9);
                    game.swnbidirectional ? gravcreate(4, 0, 175, -10, 7, 10) : gravcreate(4, 1, 160, -10, 7, 10);
                    game.swnbidirectional ? gravcreate(5, 0, 195, -10, 7, 11) : gravcreate(5, 1, 180, -10, 7, 11);
                    game.swnbidirectional ? gravcreate(5, 0, 175, 10, 7, 12) : gravcreate(5, 1, 160, 10, 7, 12);
                    game.swnbidirectional ? gravcreate(2, 0, 235, 0, 7, 13) : gravcreate(2, 1, 220, 0, 7, 13);
                    game.swnbidirectional ? gravcreate(1, 0, 255, 0, 7, 14) : gravcreate(1, 1, 240, 0, 7, 14);
                    game.swnbidirectional ? gravcreate(2, 0, 275, 0, 7, 15) : gravcreate(2, 1, 260, 0, 7, 15);
                    game.swnbidirectional ? gravcreate(3, 0, 255, 0, 7, 16) : gravcreate(3, 1, 240, 0, 7, 16);
                    game.swnbidirectional ? gravcreate(0, 0, 355, -10, 7, 17) : gravcreate(0, 1, 340, -10, 7, 17);
                    game.swnbidirectional ? gravcreate(1, 0, 335, -10, 7, 18) : gravcreate(1, 1, 320, -10, 7, 18);
                    game.swnbidirectional ? gravcreate(2, 0, 355, -10, 7, 19) : gravcreate(2, 1, 340, -10, 7, 19);
                    game.swnbidirectional ? gravcreate(1, 0, 375, -10, 7, 20) : gravcreate(1, 1, 360, -10, 7, 20);
                    game.swnbidirectional ? gravcreate(5, 0, 435, 10, 7, 21) : gravcreate(5, 1, 420, 10, 7, 21);
                    game.swnbidirectional ? gravcreate(4, 0, 415, 10, 7, 22) : gravcreate(4, 1, 400, 10, 7, 22);
                    game.swnbidirectional ? gravcreate(3, 0, 435, 10, 7, 23) : gravcreate(3, 1, 420, 10, 7, 23);
                    game.swnbidirectional ? gravcreate(4, 0, 455, 10, 7, 24) : gravcreate(4, 1, 440, 10, 7, 24);

                    game.swnstate = 0;
                    game.swndelay = 105;
                } break;

                case 110: {     
                    // Doing Things The Side Way //
                    // exotic //

                    if (game.swnstate2 == 0)
                    {
                        game.swnbidirectional ? gravcreate(0, 0, 1795, 0, 7, 324) : gravcreate(0, 1, 1780, 0, 7, 324);
                        game.swnbidirectional ? gravcreate(1, 0, 1795, 0, 7, 323) : gravcreate(1, 1, 1780, 0, 7, 323);
                        game.swnbidirectional ? gravcreate(2, 0, 1795, 0, 7, 322) : gravcreate(2, 1, 1780, 0, 7, 322);
                        game.swnbidirectional ? gravcreate(3, 0, 1795, 0, 7, 321) : gravcreate(3, 1, 1780, 0, 7, 321);
                        game.swnbidirectional ? gravcreate(4, 0, 1795, 0, 7, 320) : gravcreate(4, 1, 1780, 0, 7, 320);
                        game.swnbidirectional ? gravcreate(5, 0, 1795, 0, 7, 319) : gravcreate(5, 1, 1780, 0, 7, 319);
                        game.swnbidirectional ? gravcreate(4, 0, 1775, 0, 7, 318) : gravcreate(4, 1, 1760, 0, 7, 318);
                        game.swnbidirectional ? gravcreate(2, 0, 1775, 0, 7, 317) : gravcreate(2, 1, 1760, 0, 7, 317);
                        game.swnbidirectional ? gravcreate(0, 0, 1775, 0, 7, 316) : gravcreate(0, 1, 1760, 0, 7, 316);
                        game.swnbidirectional ? gravcreate(1, 0, 1755, 0, 7, 315) : gravcreate(1, 1, 1740, 0, 7, 315);
                        game.swnbidirectional ? gravcreate(3, 0, 1755, 0, 7, 314) : gravcreate(3, 1, 1740, 0, 7, 314);
                        game.swnbidirectional ? gravcreate(5, 0, 1755, 0, 7, 313) : gravcreate(5, 1, 1740, 0, 7, 313);
                        game.swnbidirectional ? gravcreate(4, 0, 1735, 0, 7, 312) : gravcreate(4, 1, 1720, 0, 7, 312);
                        game.swnbidirectional ? gravcreate(2, 0, 1735, 0, 7, 311) : gravcreate(2, 1, 1720, 0, 7, 311);
                        game.swnbidirectional ? gravcreate(0, 0, 1735, 0, 7, 310) : gravcreate(0, 1, 1720, 0, 7, 310);
                        game.swnbidirectional ? gravcreate(1, 0, 1715, 0, 7, 309) : gravcreate(1, 1, 1700, 0, 7, 309);
                        game.swnbidirectional ? gravcreate(3, 0, 1715, 0, 7, 308) : gravcreate(3, 1, 1700, 0, 7, 308);
                        game.swnbidirectional ? gravcreate(5, 0, 1715, 0, 7, 307) : gravcreate(5, 1, 1700, 0, 7, 307);
                        game.swnbidirectional ? gravcreate(4, 0, 1695, 0, 7, 306) : gravcreate(4, 1, 1680, 0, 7, 306);
                        game.swnbidirectional ? gravcreate(2, 0, 1695, 0, 7, 305) : gravcreate(2, 1, 1680, 0, 7, 305);
                        game.swnbidirectional ? gravcreate(0, 0, 1695, 0, 7, 304) : gravcreate(0, 1, 1680, 0, 7, 304);
                        game.swnbidirectional ? gravcreate(1, 0, 1675, 0, 7, 303) : gravcreate(1, 1, 1660, 0, 7, 303);
                        game.swnbidirectional ? gravcreate(3, 0, 1675, 0, 7, 301) : gravcreate(3, 1, 1660, 0, 7, 301);
                        game.swnbidirectional ? gravcreate(5, 0, 1675, 0, 7, 300) : gravcreate(5, 1, 1660, 0, 7, 300);
                        game.swnbidirectional ? gravcreate(4, 0, 1655, 0, 7, 299) : gravcreate(4, 1, 1640, 0, 7, 299);
                        game.swnbidirectional ? gravcreate(2, 0, 1655, 0, 7, 298) : gravcreate(2, 1, 1640, 0, 7, 298);
                        game.swnbidirectional ? gravcreate(0, 0, 1655, 0, 7, 297) : gravcreate(0, 1, 1640, 0, 7, 297);
                        game.swnbidirectional ? gravcreate(1, 0, 1635, 0, 7, 296) : gravcreate(1, 1, 1620, 0, 7, 296);
                        game.swnbidirectional ? gravcreate(3, 0, 1635, 0, 7, 295) : gravcreate(3, 1, 1620, 0, 7, 295);
                        game.swnbidirectional ? gravcreate(5, 0, 1635, 0, 7, 293) : gravcreate(5, 1, 1620, 0, 7, 293);
                        game.swnbidirectional ? gravcreate(4, 0, 1615, 0, 7, 292) : gravcreate(4, 1, 1600, 0, 7, 292);
                        game.swnbidirectional ? gravcreate(2, 0, 1615, 0, 7, 291) : gravcreate(2, 1, 1600, 0, 7, 291);
                        game.swnbidirectional ? gravcreate(0, 0, 1615, 0, 7, 290) : gravcreate(0, 1, 1600, 0, 7, 290);
                        game.swnbidirectional ? gravcreate(1, 0, 1595, 0, 7, 289) : gravcreate(1, 1, 1580, 0, 7, 289);
                        game.swnbidirectional ? gravcreate(3, 0, 1595, 0, 7, 288) : gravcreate(3, 1, 1580, 0, 7, 288);
                        game.swnbidirectional ? gravcreate(5, 0, 1595, 0, 7, 287) : gravcreate(5, 1, 1580, 0, 7, 287);
                        game.swnbidirectional ? gravcreate(4, 0, 1575, 0, 7, 286) : gravcreate(4, 1, 1560, 0, 7, 286);
                        game.swnbidirectional ? gravcreate(2, 0, 1575, 0, 7, 285) : gravcreate(2, 1, 1560, 0, 7, 285);
                        game.swnbidirectional ? gravcreate(0, 0, 1575, 0, 7, 284) : gravcreate(0, 1, 1560, 0, 7, 284);
                        game.swnbidirectional ? gravcreate(1, 0, 1555, 0, 7, 283) : gravcreate(1, 1, 1540, 0, 7, 283);
                        game.swnbidirectional ? gravcreate(3, 0, 1555, 0, 7, 282) : gravcreate(3, 1, 1540, 0, 7, 282);
                        game.swnbidirectional ? gravcreate(5, 0, 1555, 0, 7, 281) : gravcreate(5, 1, 1540, 0, 7, 281);
                        game.swnbidirectional ? gravcreate(4, 0, 1535, 0, 7, 280) : gravcreate(4, 1, 1520, 0, 7, 280);
                        game.swnbidirectional ? gravcreate(2, 0, 1535, 0, 7, 279) : gravcreate(2, 1, 1520, 0, 7, 279);
                        game.swnbidirectional ? gravcreate(0, 0, 1535, 0, 7, 278) : gravcreate(0, 1, 1520, 0, 7, 278);
                        game.swnbidirectional ? gravcreate(5, 0, 1515, 0, 7, 277) : gravcreate(5, 1, 1500, 0, 7, 277);
                        game.swnbidirectional ? gravcreate(3, 0, 1515, 0, 7, 276) : gravcreate(3, 1, 1500, 0, 7, 276);
                        game.swnbidirectional ? gravcreate(1, 0, 1515, 0, 7, 275) : gravcreate(1, 1, 1500, 0, 7, 275);
                        game.swnbidirectional ? gravcreate(5, 0, 1495, 0, 7, 257) : gravcreate(5, 1, 1480, 0, 7, 257);
                        game.swnbidirectional ? gravcreate(4, 0, 1495, 0, 7, 256) : gravcreate(4, 1, 1480, 0, 7, 256);
                        game.swnbidirectional ? gravcreate(0, 0, 1495, 0, 7, 255) : gravcreate(0, 1, 1480, 0, 7, 255);
                        game.swnbidirectional ? gravcreate(1, 0, 1495, 0, 7, 254) : gravcreate(1, 1, 1480, 0, 7, 254);
                        game.swnbidirectional ? gravcreate(2, 0, 1495, 0, 7, 253) : gravcreate(2, 1, 1480, 0, 7, 253);
                        game.swnbidirectional ? gravcreate(3, 0, 1495, 0, 7, 252) : gravcreate(3, 1, 1480, 0, 7, 252);
                        game.swnbidirectional ? gravcreate(2, 0, 865, 10, 7, 239) : gravcreate(2, 1, 850, 10, 7, 239);
                        game.swnbidirectional ? gravcreate(2, 0, 845, 10, 7, 238) : gravcreate(2, 1, 830, 10, 7, 238);
                        game.swnbidirectional ? gravcreate(2, 0, 825, 10, 7, 237) : gravcreate(2, 1, 810, 10, 7, 237);
                        game.swnbidirectional ? gravcreate(2, 0, 805, 10, 7, 236) : gravcreate(2, 1, 790, 10, 7, 236);
                        game.swnbidirectional ? gravcreate(0, 0, 1265, 0, 7, 229) : gravcreate(0, 1, 1250, 0, 7, 229);
                        game.swnbidirectional ? gravcreate(5, 0, 1265, 0, 7, 228) : gravcreate(5, 1, 1250, 0, 7, 228);
                        game.swnbidirectional ? gravcreate(2, 0, 1295, 0, 7, 227) : gravcreate(2, 1, 1280, 0, 7, 227);
                        game.swnbidirectional ? gravcreate(3, 0, 1295, 0, 7, 226) : gravcreate(3, 1, 1280, 0, 7, 226);
                        game.swnbidirectional ? gravcreate(2, 0, 1235, 0, 7, 225) : gravcreate(2, 1, 1220, 0, 7, 225);
                        game.swnbidirectional ? gravcreate(3, 0, 1235, 0, 7, 224) : gravcreate(3, 1, 1220, 0, 7, 224);
                        game.swnbidirectional ? gravcreate(5, 0, 1155, 0, 7, 223) : gravcreate(5, 1, 1140, 0, 7, 223);
                        game.swnbidirectional ? gravcreate(5, 0, 1135, 0, 7, 221) : gravcreate(5, 1, 1120, 0, 7, 221);
                        game.swnbidirectional ? gravcreate(5, 0, 1115, 0, 7, 220) : gravcreate(5, 1, 1100, 0, 7, 220);
                        game.swnbidirectional ? gravcreate(0, 0, 1095, 0, 7, 219) : gravcreate(0, 1, 1080, 0, 7, 219);
                        game.swnbidirectional ? gravcreate(0, 0, 1075, 0, 7, 218) : gravcreate(0, 1, 1060, 0, 7, 218);
                        game.swnbidirectional ? gravcreate(0, 0, 1055, 0, 7, 217) : gravcreate(0, 1, 1040, 0, 7, 217);
                        game.swnbidirectional ? gravcreate(5, 0, 1035, 0, 7, 216) : gravcreate(5, 1, 1020, 0, 7, 216);
                        game.swnbidirectional ? gravcreate(5, 0, 1015, 0, 7, 215) : gravcreate(5, 1, 1000, 0, 7, 215);
                        game.swnbidirectional ? gravcreate(5, 0, 995, 0, 7, 214) : gravcreate(5, 1, 980, 0, 7, 214);
                        game.swnbidirectional ? gravcreate(0, 0, 975, 0, 7, 213) : gravcreate(0, 1, 960, 0, 7, 213);
                        game.swnbidirectional ? gravcreate(0, 0, 935, 0, 7, 167) : gravcreate(0, 1, 920, 0, 7, 167);
                        game.swnbidirectional ? gravcreate(0, 0, 955, 0, 7, 166) : gravcreate(0, 1, 940, 0, 7, 166);
                        game.swnbidirectional ? gravcreate(0, 0, 15, 0, 7, 131) : gravcreate(0, 1, 0, 0, 7, 131);
                        game.swnbidirectional ? gravcreate(1, 0, 15, 0, 7, 130) : gravcreate(1, 1, 0, 0, 7, 130);
                        game.swnbidirectional ? gravcreate(3, 0, 15, 0, 7, 129) : gravcreate(3, 1, 0, 0, 7, 129);
                        game.swnbidirectional ? gravcreate(2, 0, 35, 0, 7, 128) : gravcreate(2, 1, 20, 0, 7, 128);
                        game.swnbidirectional ? gravcreate(1, 0, 55, 0, 7, 127) : gravcreate(1, 1, 40, 0, 7, 127);
                        game.swnbidirectional ? gravcreate(0, 0, 75, 0, 7, 126) : gravcreate(0, 1, 60, 0, 7, 126);
                        game.swnbidirectional ? gravcreate(5, 0, 235, 0, 7, 125) : gravcreate(5, 1, 220, 0, 7, 125);
                        game.swnbidirectional ? gravcreate(4, 0, 235, 0, 7, 124) : gravcreate(4, 1, 220, 0, 7, 124);
                        game.swnbidirectional ? gravcreate(3, 0, 235, 0, 7, 123) : gravcreate(3, 1, 220, 0, 7, 123);
                        game.swnbidirectional ? gravcreate(2, 0, 515, 0, 7, 122) : gravcreate(2, 1, 500, 0, 7, 122);
                        game.swnbidirectional ? gravcreate(3, 0, 515, 0, 7, 121) : gravcreate(3, 1, 500, 0, 7, 121);
                        game.swnbidirectional ? gravcreate(4, 0, 715, 0, 7, 104) : gravcreate(4, 1, 700, 0, 7, 104);
                        game.swnbidirectional ? gravcreate(3, 0, 735, 0, 7, 103) : gravcreate(3, 1, 720, 0, 7, 103);
                        game.swnbidirectional ? gravcreate(2, 0, 735, 0, 7, 102) : gravcreate(2, 1, 720, 0, 7, 102);
                        game.swnbidirectional ? gravcreate(1, 0, 715, 0, 7, 101) : gravcreate(1, 1, 700, 0, 7, 101);
                        game.swnbidirectional ? gravcreate(0, 0, 595, 0, 7, 100) : gravcreate(0, 1, 580, 0, 7, 100);
                        game.swnbidirectional ? gravcreate(5, 0, 595, 0, 7, 99) : gravcreate(5, 1, 580, 0, 7, 99);
                        game.swnbidirectional ? gravcreate(1, 0, 595, 0, 7, 97) : gravcreate(1, 1, 580, 0, 7, 97);
                        game.swnbidirectional ? gravcreate(4, 0, 595, 0, 7, 96) : gravcreate(4, 1, 580, 0, 7, 96);
                        game.swnbidirectional ? gravcreate(2, 0, 495, 0, 7, 89) : gravcreate(2, 1, 480, 0, 7, 89);
                        game.swnbidirectional ? gravcreate(3, 0, 495, 0, 7, 88) : gravcreate(3, 1, 480, 0, 7, 88);
                        game.swnbidirectional ? gravcreate(0, 0, 415, 0, 7, 87) : gravcreate(0, 1, 400, 0, 7, 87);
                        game.swnbidirectional ? gravcreate(1, 0, 415, 0, 7, 86) : gravcreate(1, 1, 400, 0, 7, 86);
                        game.swnbidirectional ? gravcreate(4, 0, 415, 0, 7, 85) : gravcreate(4, 1, 400, 0, 7, 85);
                        game.swnbidirectional ? gravcreate(5, 0, 415, 0, 7, 84) : gravcreate(5, 1, 400, 0, 7, 84);
                        game.swnbidirectional ? gravcreate(2, 0, 695, 0, 7, 83) : gravcreate(2, 1, 680, 0, 7, 83);
                        game.swnbidirectional ? gravcreate(3, 0, 695, 0, 7, 82) : gravcreate(3, 1, 680, 0, 7, 82);
                        game.swnbidirectional ? gravcreate(3, 0, 335, 0, 7, 53) : gravcreate(3, 1, 320, 0, 7, 53);
                        game.swnbidirectional ? gravcreate(5, 0, 55, 0, 7, 52) : gravcreate(5, 1, 40, 0, 7, 52);
                        game.swnbidirectional ? gravcreate(4, 0, 75, 0, 7, 51) : gravcreate(4, 1, 60, 0, 7, 51);
                        game.swnbidirectional ? gravcreate(3, 0, 95, 0, 7, 50) : gravcreate(3, 1, 80, 0, 7, 50);
                        game.swnbidirectional ? gravcreate(2, 0, 315, 0, 7, 45) : gravcreate(2, 1, 300, 0, 7, 45);
                        game.swnbidirectional ? gravcreate(1, 0, 295, 0, 7, 44) : gravcreate(1, 1, 280, 0, 7, 44);
                        game.swnbidirectional ? gravcreate(0, 0, 275, 0, 7, 43) : gravcreate(0, 1, 260, 0, 7, 43);
                        game.swnbidirectional ? gravcreate(5, 0, 295, 0, 7, 37) : gravcreate(5, 1, 280, 0, 7, 37);
                        game.swnbidirectional ? gravcreate(4, 0, 275, 0, 7, 34) : gravcreate(4, 1, 260, 0, 7, 34);
                        game.swnbidirectional ? gravcreate(3, 0, 255, 0, 7, 33) : gravcreate(3, 1, 240, 0, 7, 33);
                        game.swnbidirectional ? gravcreate(2, 0, 235, 0, 7, 27) : gravcreate(2, 1, 220, 0, 7, 27);
                        game.swnbidirectional ? gravcreate(3, 0, 175, 0, 7, 22) : gravcreate(3, 1, 160, 0, 7, 22);
                        game.swnbidirectional ? gravcreate(2, 0, 175, 0, 7, 21) : gravcreate(2, 1, 160, 0, 7, 21);
                        game.swnbidirectional ? gravcreate(1, 0, 175, 0, 7, 20) : gravcreate(1, 1, 160, 0, 7, 20);
                        game.swnbidirectional ? gravcreate(0, 0, 175, 0, 7, 19) : gravcreate(0, 1, 160, 0, 7, 19);
                        game.swnbidirectional ? gravcreate(5, 0, 115, 0, 7, 18) : gravcreate(5, 1, 100, 0, 7, 18);
                        game.swnbidirectional ? gravcreate(4, 0, 115, 0, 7, 17) : gravcreate(4, 1, 100, 0, 7, 17);
                        game.swnbidirectional ? gravcreate(3, 0, 115, 0, 7, 16) : gravcreate(3, 1, 100, 0, 7, 16);
                        game.swnbidirectional ? gravcreate(2, 0, 115, 0, 7, 15) : gravcreate(2, 1, 100, 0, 7, 15);
                        game.swnbidirectional ? gravcreate(2, 0, 15, 0, 7, 9) : gravcreate(2, 1, 0, 0, 7, 9);
                        
                        swnspeedchange(5);
                    }

                    if (game.swnstate2 < 375)
                    {
                        game.swnstate = 110;
                        game.swndelay = 0;
                    }
                    else
                    {
                        swnreverse();

                        game.swnstate = 0;
                        game.swndelay = 360;
                    }

                    game.swnstate2++;
                } break;
                       
                case 111: {     
                    // Worms //
                    // common //

                    if (game.swnstate2 == 0)
                    {
                        game.swnbidirectional ? gravcreate(2, 0, 35, 0, 7, 2) : gravcreate(2, 1, 20, 0, 7, 2);
                        game.swnbidirectional ? gravcreate(5, 0, 135, 0, 7, 2) : gravcreate(5, 1, 120, 0, 7, 2);
                        game.swnbidirectional ? gravcreate(1, 0, 215, 0, 7, 2) : gravcreate(1, 1, 200, 0, 7, 2);
                        game.swnbidirectional ? gravcreate(3, 0, 315, 0, 7, 2) : gravcreate(3, 1, 300, 0, 7, 2);
                        game.swnbidirectional ? gravcreate(3, 0, 335, 0, 7, 1) : gravcreate(3, 1, 320, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(3, 0, 295, 0, 7, 1) : gravcreate(3, 1, 280, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(1, 0, 235, 0, 7, 1) : gravcreate(1, 1, 220, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(1, 0, 195, 0, 7, 1) : gravcreate(1, 1, 180, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(5, 0, 155, 0, 7, 1) : gravcreate(5, 1, 140, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(5, 0, 115, 0, 7, 1) : gravcreate(5, 1, 100, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(2, 0, 15, 0, 7, 1) : gravcreate(2, 1, 0, 0, 7, 1);
                        game.swnbidirectional ? gravcreate(2, 0, 55, 0, 7, 1) : gravcreate(2, 1, 40, 0, 7, 1);
                    }

                    if (game.swnstate2 < 120)
                    {
                        if (game.swnstate2 % 30 == 0)
                        {
                            swnmove(-20, 2);
                        }
                        else if (game.swnstate2 % 15 == 0)
                        {
                            swnmove(20, 2);
                        }

                        game.swnstate = 111;
                    }
                    else
                    {
                        game.swnstate = 0;
                    }

                    game.swnstate2++;
                    game.swndelay = 0;
                } break;

            }
        }
    }
    else
    {
        game.swndelay--;
    }
}

void entityclass::createblock( int t, int xp, int yp, int w, int h, int trig /*= 0*/, const std::string& script /*= ""*/, bool custom /*= false*/)
{
    k = blocks.size();

    blockclass newblock;
    blockclass* blockptr;

    /* Can we reuse the slot of a disabled block? */
    bool reuse = false;
    for (size_t i = 0; i < blocks.size(); ++i)
    {
        if (blocks[i].wp == 0
        && blocks[i].hp == 0
        && blocks[i].rect.w == 0
        && blocks[i].rect.h == 0)
        {
            reuse = true;
            blockptr = &blocks[i];
            break;
        }
    }

    if (!reuse)
    {
        blockptr = &newblock;
    }
    else
    {
        blockptr->clear();
    }

    blockclass& block = *blockptr;
    switch(t)
    {
    case BLOCK: //Block
        block.type = BLOCK;
        block.xp = xp;
        block.yp = yp;
        block.wp = w;
        block.hp = h;
        block.rectset(xp, yp, w, h);
        break;
    case TRIGGER: //Trigger
        block.type = TRIGGER;
        block.wp = w;
        block.hp = h;
        block.rectset(xp, yp, w, h);
        block.trigger = trig;
        block.script = script;
        break;
    case DAMAGE: //Damage
        block.type = DAMAGE;
        block.wp = w;
        block.hp = h;
        block.rectset(xp, yp, w, h);
        break;
    case DIRECTIONAL: //Directional
        block.type = DIRECTIONAL;
        block.wp = w;
        block.hp = h;
        block.rectset(xp, yp, w, h);
        block.trigger = trig;
        break;
    case SAFE: //Safe block
        block.type = SAFE;
        block.xp = xp;
        block.yp = yp;
        block.wp = w;
        block.hp = h;
        block.rectset(xp, yp, w, h);
        break;
    case ACTIVITY: //Activity Zone
        block.type = ACTIVITY;
        block.wp = w;
        block.hp = h;
        block.rectset(xp, yp, w, h);

        //Ok, each and every activity zone in the game is initilised here. "Trig" in this case is a variable that
        //assigns all the details.
        switch(trig)
        {
        case 0: //testing zone
            block.prompt = "Press {button} to explode";
            block.script = "intro";
            block.setblockcolour("orange");
            trig=1;
            break;
        case 1:
            block.prompt = "Press {button} to talk to Violet";
            block.script = "talkpurple";
            block.setblockcolour("purple");
            trig=0;
            break;
        case 2:
            block.prompt = "Press {button} to talk to Vitellary";
            block.script = "talkyellow";
            block.setblockcolour("yellow");
            trig=0;
            break;
        case 3:
            block.prompt = "Press {button} to talk to Vermilion";
            block.script = "talkred";
            block.setblockcolour("red");
            trig=0;
            break;
        case 4:
            block.prompt = "Press {button} to talk to Verdigris";
            block.script = "talkgreen";
            block.setblockcolour("green");
            trig=0;
            break;
        case 5:
            block.prompt = "Press {button} to talk to Victoria";
            block.script = "talkblue";
            block.setblockcolour("blue");
            trig=0;
            break;
        case 6:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_station_1";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 7:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_outside_1";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 8:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_outside_2";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 9:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_outside_3";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 10:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_outside_4";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 11:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_outside_5";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 12:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_outside_6";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 13:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_finallevel";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 14:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_station_2";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 15:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_station_3";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 16:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_station_4";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 17:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_warp_1";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 18:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_warp_2";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 19:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_lab_1";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 20:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_lab_2";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 21:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_secretlab";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 22:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_shipcomputer";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 23:
            block.prompt = "Press {button} to activate terminals";
            block.script = "terminal_radio";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 24:
            block.prompt = "Press {button} to activate terminal";
            block.script = "terminal_jukebox";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 25:
            block.prompt = "Passion for Exploring";
            block.script = "terminal_juke1";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 26:
            block.prompt = "Pushing Onwards";
            block.script = "terminal_juke2";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 27:
            block.prompt = "Positive Force";
            block.script = "terminal_juke3";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 28:
            block.prompt = "Presenting VVVVVV";
            block.script = "terminal_juke4";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 29:
            block.prompt = "Potential for Anything";
            block.script = "terminal_juke5";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 30:
            block.prompt = "Predestined Fate";
            block.script = "terminal_juke6";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 31:
            block.prompt = "Pipe Dream";
            block.script = "terminal_juke7";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 32:
            block.prompt = "Popular Potpourri";
            block.script = "terminal_juke8";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 33:
            block.prompt = "Pressure Cooker";
            block.script = "terminal_juke9";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 34:
            block.prompt = "ecroF evitisoP";
            block.script = "terminal_juke10";
            block.setblockcolour("orange");
            trig=0;
            break;
        case 35:
            if (custom)
            {
                block.prompt = "Press {button} to interact";
            }
            else
            {
                block.prompt = "Press {button} to activate terminal";
            }
            block.script = "custom_"+customscript;
            block.setblockcolour("orange");
            trig=0;
            break;
        }
        break;
    }

    if (customactivitytext != "")
    {
        block.prompt = customactivitytext;
        block.gettext = false;
        customactivitytext = "";
    }
    else
    {
        block.gettext = true;
    }

    if (customactivitycolour != "")
    {
        block.setblockcolour(customactivitycolour.c_str());
        customactivitycolour = "";
    }

    if (customactivitypositiony != -1)
    {
        block.activity_y = customactivitypositiony;
        customactivitypositiony = -1;
    }
    else
    {
        block.activity_y = 0;
    }

    if (!reuse)
    {
        blocks.push_back(block);
    }
}

/* Disable entity, and return true if entity was successfully disabled */
bool entityclass::disableentity(int t)
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("disableentity() out-of-bounds!");
        return true;
    }
    if (entities[t].rule == 0 && t == getplayer())
    {
        /* Don't disable the player entity! */
        return false;
    }

    entities[t].invis = true;
    entities[t].size = -1;
    entities[t].type = EntityType_INVALID;
    entities[t].rule = -1;
    entities[t].isplatform = false;

    return true;
}

void entityclass::removeallblocks(void)
{
    blocks.clear();
}

void entityclass::disableblock( int t )
{
    if (!INBOUNDS_VEC(t, blocks))
    {
        vlog_error("disableblock() out-of-bounds!");
        return;
    }

    blocks[t].wp = 0;
    blocks[t].hp = 0;

    blocks[t].rect.w = blocks[t].wp;
    blocks[t].rect.h = blocks[t].hp;
}

void entityclass::moveblockto(int x1, int y1, int x2, int y2, int w, int h)
{
    for (size_t i = 0; i < blocks.size(); i++)
    {
        if (blocks[i].xp == x1 && blocks[i].yp == y1)
        {
            blocks[i].xp = x2;
            blocks[i].yp = y2;

            blocks[i].wp = w;
            blocks[i].hp = h;

            blocks[i].rectset(blocks[i].xp, blocks[i].yp, blocks[i].wp, blocks[i].hp);
            break;
        }
    }
}

void entityclass::disableblockat(int x, int y)
{
    for (size_t i = 0; i < blocks.size(); i++)
    {
        if (blocks[i].xp == x && blocks[i].yp == y)
        {
            disableblock(i);
        }
    }
}

void entityclass::removetrigger( int t )
{
    for(size_t i=0; i<blocks.size(); i++)
    {
        if(blocks[i].type == TRIGGER && blocks[i].trigger == t)
        {
            disableblock(i);
        }
    }
}

void entityclass::copylinecross(std::vector<entclass>& linecrosskludge, int t)
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("copylinecross() out-of-bounds!");
        return;
    }
    //Copy entity t into the first free linecrosskludge entity
    linecrosskludge.push_back(entities[t]);
}

void entityclass::revertlinecross(std::vector<entclass>& linecrosskludge, int t, int s)
{
    if (!INBOUNDS_VEC(t, entities) || !INBOUNDS_VEC(s, linecrosskludge))
    {
        vlog_error("revertlinecross() out-of-bounds!");
        return;
    }
    //Restore entity t info from linecrossing s
    entities[t].onentity = linecrosskludge[s].onentity;
    entities[t].state = linecrosskludge[s].state;
    entities[t].life = linecrosskludge[s].life;
}

static bool gridmatch( int p1, int p2, int p3, int p4, int p11, int p21, int p31, int p41 )
{
    if (p1 == p11 && p2 == p21 && p3 == p31 && p4 == p41) return true;
    return false;
}

static void entityclonefix(entclass* entity)
{
    const bool is_lies_emitter = entity->behave == 10;
    const bool is_factory_emitter = entity->behave == 12;

    const bool is_emitter = is_lies_emitter || is_factory_emitter;
    if (!is_emitter)
    {
        return;
    }

    const bool in_lies_emitter_room =
        game.roomx >= 113 && game.roomx <= 117 && game.roomy == 111;
    const bool in_factory_emitter_room =
        game.roomx == 113 && game.roomy >= 108 && game.roomy <= 110;

    const bool valid = (is_lies_emitter && in_lies_emitter_room)
        || (is_factory_emitter && in_factory_emitter_room);

    if (!valid)
    {
        /* Fix memory leak */
        entity->behave = -1;
    }
}

void entityclass::createentity(int xp, int yp, int t, int meta1, int meta2, int p1, int p2, int p3, int p4)
{
    k = entities.size();

    entclass newent;
    entclass* entptr;

    /* Can we reuse the slot of a disabled entity? */
    bool reuse = false;
    for (size_t i = 0; i < entities.size(); ++i)
    {
        if (entities[i].invis
        && entities[i].size == -1
        && entities[i].type == EntityType_INVALID
        && entities[i].rule == -1
        && !entities[i].isplatform)
        {
            reuse = true;
            entptr = &entities[i];
            break;
        }
    }

    if (!reuse)
    {
        entptr = &newent;
    }
    else
    {
        entptr->clear();
    }

    //Size 0 is a sprite
    //Size 1 is a tile
    //Beyond that are special cases (to do)
    //Size 2 is a moving platform of width 4 (32)
    //Size 3 is apparently a "big chunky pixel"
    //Size 4 is a coin/small pickup
    //Size 5 is a horizontal line, 6 is vertical

    //Rule 0 is the playable character
    //Rule 1 is anything harmful
    //Rule 2 is anything decorative (no collisions)
    //Rule 3 is anything that results in an entity to entity collision and state change
    //Rule 4 is a horizontal line, 5 is vertical
    //Rule 6 is a crew member

    bool custom_gray;
    // Special case for gray Warp Zone tileset!
    if (map.custommode)
    {
        const RoomProperty* const room = cl.getroomprop(game.roomx - 100, game.roomy - 100);
        custom_gray = room->tileset == 3 && room->tilecol == 6;
    }
    else
    {
        custom_gray = false;
    }

    entclass& entity = *entptr;
    entity.xp = xp;
    entity.yp = yp;
    entity.type = EntityType_INVALID;
    switch(t)
    {
    case 0: //Player
        entity.rule = 0; //Playable character
        entity.tile = 0;
        entity.colour = EntityColour_CREW_CYAN;
        entity.cx = 6;
        entity.cy = 2;
        entity.w = 12;
        entity.h = 21;
        entity.dir = 1;
        entity.type = EntityType_PLAYER;

        /* Fix wrong y-position if spawning in on conveyor */
        entity.newxp = xp;
        entity.newyp = yp;

        if (meta1 == 1) entity.invis = true;

        entity.gravity = true;
        break;
    case 1: //Simple enemy, bouncing off the walls
        entity.rule = 1;
        entity.behave = meta1;
        entity.para = meta2;
        entity.w = 16;
        entity.h = 16;
        entity.cx = 0;
        entity.cy = 0;

        entity.x1 = p1;
        entity.y1 = p2;
        entity.x2 = p3;
        entity.y2 = p4;

        entity.harmful = true;
        entity.tile = 24;
        entity.animate = 0;
        entity.colour = EntityColour_ENEMY_PINK;

        entity.type = EntityType_MOVING;

        if  (game.roomy == 111 && (game.roomx >= 113 && game.roomx <= 117))
        {
            entity.setenemy(0);
            entity.setenemyroom(game.roomx, game.roomy); //For colour
        }
        else if  (game.roomx == 113 && (game.roomy <= 110 && game.roomy >= 108))
        {
            entity.setenemy(1);
            entity.setenemyroom(game.roomx, game.roomy); //For colour
        }
        else if (game.roomx == 113 && game.roomy == 107)
        {
            //MAVVERRRICK
            entity.tile = 96;
            entity.colour = EntityColour_ENEMY_RED;
            entity.size = 9;
            entity.w = 64;
            entity.h = 44;
            entity.animate = 4;
        }
        else
        {
            entity.setenemyroom(game.roomx, game.roomy);
            entityclonefix(&entity);
        }
        break;
    case 2: //A moving platform
        entity.rule = 2;
        entity.type = EntityType_MOVING;
        entity.size = 2;
        entity.tile = 1;

        if (customplatformtile > 0){
            entity.tile = customplatformtile;
        }else if (platformtile > 0) {
            entity.tile = platformtile;
        }else{
          //appearance again depends on location
          if (gridmatch(p1, p2, p3, p4, 100, 70, 320, 160)) entity.tile = 616;
          if (gridmatch(p1, p2, p3, p4, 72, 0, 248, 240)) entity.tile = 610;
          if (gridmatch(p1, p2, p3, p4, -20, 0, 320, 240)) entity.tile = 413;

          if (gridmatch(p1, p2, p3, p4, -96, -72, 400, 312)) entity.tile = 26;
          if (gridmatch(p1, p2, p3, p4, -32, -40, 352, 264)) entity.tile = 27;
        }

        entity.w = 32;
        entity.h = 8;

        if (meta1 <= 1) vertplatforms = true;
        if (meta1 >= 2  && meta1 <= 5) horplatforms = true;
        if (meta1 == 14 || meta1 == 15) horplatforms = true; //special case for last part of Space Station
        if (meta1 >= 6  && meta1 <= 7) vertplatforms = true;

        if (meta1 >= 10  && meta1 <= 11)
        {
            //Double sized threadmills
            entity.w = 64;
            entity.h = 8;
            meta1 -= 2;
            entity.size = 8;
        }

        entity.behave = meta1;
        entity.para = meta2;

        if (meta1 >= 8  && meta1 <= 9)
        {
            horplatforms = true; //threadmill!
            entity.animate = 10;
            if(customplatformtile>0){
              entity.tile = customplatformtile+4;
              if (meta1 == 8) entity.tile += 4;
              if (meta1 == 9) entity.animate = 11;
            }else{
              entity.settreadmillcolour(game.roomx, game.roomy);
              if (meta1 == 8) entity.tile += 40;
              if (meta1 == 9) entity.animate = 11;
            }
        }
        else
        {
            entity.animate = 100;
        }

        entity.x1 = p1;
        entity.y1 = p2;
        entity.x2 = p3;
        entity.y2 = p4;

        entity.isplatform = true;

        createblock(0, xp, yp, 32, 8);
        break;
    case 3: //Disappearing platforms
        entity.rule = 3;
        entity.type = EntityType_DISAPPEARING_PLATFORM;
        entity.size = 2;
        entity.tile = 2;
        //appearance again depends on location
        if(customplatformtile>0)
        {
          entity.tile=customplatformtile;
        }
        else if (meta1 > 0)
        {
            entity.tile = meta1;
        }
        else
        {
            if(game.roomx==49 && game.roomy==52) entity.tile = 18;
            if (game.roomx == 50 && game.roomy == 52) entity.tile = 22;
        }

        entity.cy = -1;
        entity.w = 32;
        entity.h = 10;
        entity.behave = meta1;
        entity.para = meta2;
        entity.onentity = 1;
        entity.animate = 100;

        createblock(0, xp, yp, 32, 8);
        break;
    case 4: //Breakable blocks
        entity.rule = 6;
        entity.type = EntityType_QUICKSAND;
        entity.size = 1;
        entity.tile = 10;
        entity.cy = -1;
        entity.w = 8;
        entity.h = 10;
        entity.behave = meta1;
        entity.para = meta2;
        entity.onentity = 1;
        entity.animate = 100;

        createblock(0, xp, yp, 8, 8);
        break;
    case 5: //Gravity Tokens
        entity.rule = 3;
        entity.type = EntityType_GRAVITY_TOKEN;
        entity.size = 0;
        entity.tile = 11;
        entity.w = 16;
        entity.h = 16;
        entity.behave = meta1;
        entity.para = meta2;
        entity.onentity = 1;
        entity.animate = 100;
        break;
    case 6: //Decorative particles
        entity.rule = 2;
        entity.type = EntityType_PARTICLE;  //Particles
        entity.colour = EntityColour_PARTICLE_RED;
        entity.size = 3;
        entity.vx = meta1;
        entity.vy = meta2;

        entity.life = 12;
        break;
    case 7: //Decorative particles
        entity.rule = 2;
        entity.type = EntityType_PARTICLE;  //Particles
        entity.colour = EntityColour_CREW_CYAN;
        entity.size = 3;
        entity.vx = meta1;
        entity.vy = meta2;

        entity.life = 12;
        break;
    case 8: //Small collectibles
        entity.rule = 3;
        entity.type = EntityType_COIN;
        entity.size = 4;
        entity.colour = EntityColour_COIN;
        entity.tile = 48;
        entity.w = 8;
        entity.h = 8;
        entity.onentity = 1;
        entity.animate = 100;

        //Check if it's already been collected
        entity.para = meta1;
        if (!INBOUNDS_ARR(meta1, collect) || collect[meta1]) return;
        break;
    case 9: //Something Shiny
        entity.rule = 3;
        entity.type = EntityType_TRINKET;
        entity.size = 0;
        entity.tile = 22;
        entity.w = 16;
        entity.h = 16;
        entity.colour = EntityColour_TRINKET;
        entity.onentity = 1;
        entity.animate = 100;

        //Check if it's already been collected
        entity.para = meta1;
        if (!INBOUNDS_ARR(meta1, collect) || collect[meta1]) return;
        break;
    case 10: //Savepoint
        entity.rule = 3;
        entity.type = EntityType_CHECKPOINT;
        entity.size = 0;
        entity.tile = 20 + meta1;
        entity.w = 16;
        entity.h = 16;
        entity.colour = EntityColour_INACTIVE_ENTITY;
        entity.onentity = 1;
        entity.animate = 100;
        entity.para = meta2;

        if (game.savepoint == meta2)
        {
            entity.colour = EntityColour_ACTIVE_ENTITY;
            entity.onentity = 0;
        }

        if (game.nodeathmode)
        {
            return;
        }
        break;
    case 11: //Horizontal Gravity Line
        entity.rule = 4;
        entity.type = EntityType_HORIZONTAL_GRAVITY_LINE;
        entity.size = 5;
        entity.life = 0;
        entity.colour = EntityColour_GRAVITY_LINE_ACTIVE;
        entity.w = meta1;
        entity.h = 1;
        entity.onentity = 1;
        break;
    case 12: //Vertical Gravity Line
        entity.rule = 5;
        entity.type = EntityType_VERTICAL_GRAVITY_LINE;
        entity.size = 6;
        entity.life = 0;
        entity.colour = EntityColour_GRAVITY_LINE_ACTIVE;
        entity.w = 1;
        entity.h = meta1;
        //entity.colour = EntityColour_CREW_CYAN;
        entity.onentity = 1;
        break;
    case 13: //Warp token
        entity.rule = 3;
        entity.type = EntityType_WARP_TOKEN;
        entity.size = 0;
        entity.tile = 18;
        entity.w = 16;
        entity.h = 16;
        entity.colour = EntityColour_WARP_TOKEN;
        entity.onentity = 1;
        entity.animate = 2;
        //Added in port, hope it doesn't break anything
        entity.behave = meta1;
        entity.para = meta2;
        break;
    case 14: // Teleporter
        entity.rule = 3;
        entity.type = EntityType_TELEPORTER;
        entity.size = 7;
        entity.tile = 1; //inactive
        entity.w = 96;
        entity.h = 96;
        entity.colour = EntityColour_TELEPORTER_INACTIVE;
        entity.onentity = 1;
        entity.animate = 100;
        entity.para = meta2;
        break;
    case 15: // Crew Member (warp zone)
        entity.rule = 6;
        entity.type = EntityType_CREWMATE; //A special case!
        entity.tile = 144;
        entity.colour = EntityColour_CREW_GREEN; //144 for sad :(
        entity.cx = 6;
        entity.cy = 2;
        entity.w = 12;
        entity.h = 21;
        entity.dir = 0;

        entity.state = meta1;

        entity.gravity = true;
        break;
    case 16: // Crew Member, upside down (space station)
        entity.rule = 7;
        entity.type = EntityType_CREWMATE; //A special case!
        entity.tile = 144+6;
        entity.colour = EntityColour_CREW_YELLOW; //144 for sad (upside down+12):(
        entity.cx = 6;
        entity.cy = 2;
        entity.w = 12;
        entity.h = 21;
        entity.dir = 1;

        entity.state = meta1;

        entity.gravity = true;
        break;
    case 17: // Crew Member (Lab)
        entity.rule = 6;
        entity.type = EntityType_CREWMATE; //A special case!
        entity.tile = 144;
        entity.colour = EntityColour_CREW_BLUE; //144 for sad :(
        entity.cx = 6;
        entity.cy = 2;
        entity.w = 12;
        entity.h = 21;
        entity.dir = 1;

        entity.state = meta1;

        entity.gravity = true;
        break;
    case 18: // Crew Member (Ship)
        //This is the scriping crewmember
        entity.rule = 6;
        entity.type = EntityType_CREWMATE; //A special case!
        entity.colour = meta1;
        if (meta2 == 0)
        {
            entity.tile = 0;
        }
        else
        {
            entity.tile = 144;
        }
        entity.cx = 6;
        entity.cy = 2;
        entity.w = 12;
        entity.h = 21;
        entity.dir = 0;

        entity.state = p1;
        entity.para = p2;

        if (p1 == 17)
        {
            entity.dir = p2;
        }

        entity.gravity = true;
        break;
    case 19: // Crew Member (Ship) More tests!
        entity.rule = 6;
        entity.type = EntityType_CREWMATE; //A special case!
        entity.tile = 0;
        entity.colour = EntityColour_ENEMY_RED; //54 for sad :(
        entity.cx = 6;
        entity.cy = 2;
        entity.w = 12;
        entity.h = 21;
        entity.dir = 1;

        entity.state = meta1;

        entity.gravity = true;
        break;
    case 20: //Terminal
        entity.rule = 3;
        entity.type = EntityType_TERMINAL;
        entity.size = 0;
        entity.tile = 16 + meta1;
        entity.w = 16;
        entity.h = 16;
        entity.colour = EntityColour_INACTIVE_ENTITY;
        entity.onentity = 1;
        entity.animate = 100;
        entity.para = meta2;
        break;
    case 21: //as above, except doesn't highlight
        entity.rule = 3;
        entity.type = EntityType_TERMINAL;
        entity.size = 0;
        entity.tile = 16 + meta1;
        entity.w = 16;
        entity.h = 16;
        entity.colour = EntityColour_INACTIVE_ENTITY;
        entity.onentity = 0;
        entity.animate = 100;
        entity.para = meta2;
        break;
    case 22: //Fake trinkets, only appear if you've collected them
        entity.rule = 3;
        entity.type = EntityType_TRINKET;
        entity.size = 0;
        entity.tile = 22;
        entity.w = 16;
        entity.h = 16;
        entity.colour = EntityColour_TRINKET;
        entity.onentity = 0;
        entity.animate = 100;

        //Check if it's already been collected
        entity.para = meta1;
        if (INBOUNDS_ARR(meta1, collect) && !collect[meta1]) return;
        break;
    case 23: //SWN Enemies
        //Given a different behavior, these enemies are especially for SWN mode and disappear outside the screen.
        entity.rule = 1;
        entity.type = EntityType_GRAVITRON_ENEMY;
        entity.behave = meta1;
        entity.para = meta2;
        entity.id = p1;
        entity.w = 16;
        entity.h = 16;
        entity.cx = 0;
        entity.cy = 0;

        if (entity.behave == 3) {
            entity.timer = game.swntimer + game.swnhomingtimer;
        }

        entity.x1 = -2000;
        entity.y1 = -100;
        entity.x2 = 5200;
        entity.y2 = 340;

        entity.harmful = true;

        //initilise tiles here based on behavior
        entity.size = 12; //don't wrap around
        entity.colour = EntityColour_ENEMY_GRAVITRON;
        entity.tile = 78; //default case
        entity.animate = 1;
        if (game.swngame == SWN_SUPERGRAVITRON)
        {
            //set colour based on current state
            entity.colour = swncolour(game.swncolstate);
        }
        break;
    case 24: // Super Crew Member
        //This special crewmember is way more advanced than the usual kind, and can interact with game objects
        entity.rule = 6;
        entity.type = EntityType_SUPERCREWMATE; //A special case!
        entity.colour = meta1;
        if (meta1 == 16)
        {
            //victoria is sad!
            if (meta2 == 2) meta2 = 1;
        }
        else
        {
            if (meta2 == 2) meta2 = 0;
        }
        if (meta2 == 0)
        {
            entity.tile = 0;
        }
        else
        {
            entity.tile = 144;
        }
        entity.cx = 6;
        entity.cy = 2;
        entity.w = 12;
        entity.h = 21;
        entity.dir = 1;

        entity.x1 = -2000;
        entity.y1 = -100;
        entity.x2 = 5200;
        entity.y2 = 340;

        entity.state = p1;
        entity.para = p2;

        if (p1 == 17)
        {
            entity.dir = p2;
        }

        entity.gravity = true;
        break;
    case 25: //Trophies
        entity.rule = 3;
        entity.type = EntityType_TROPHY;
        entity.size = 0;
        entity.w = 16;
        entity.h = 16;
        entity.colour = EntityColour_INACTIVE_ENTITY;
        entity.onentity = 1;
        entity.animate = 100;
        entity.para = meta2;

        //Decide tile here based on given achievement: both whether you have them and what they are
        //default is just a trophy base:
        entity.tile = 180 + meta1;
        switch (meta2)
        {
        case 1:
            if (game.bestrank[TimeTrial_SPACESTATION1] >= 3)
            {
                entity.tile = 184 + meta1;
                entity.colour = EntityColour_TROPHY_SPACE_STATION_1;
            }
            break;
        case 2:
            if (game.bestrank[TimeTrial_LABORATORY] >= 3)
            {
                entity.tile = 186 + meta1;
                entity.colour = EntityColour_TROPHY_LABORATORY;
            }
            break;
        case 3:
            if (game.bestrank[TimeTrial_TOWER] >= 3)
            {
                entity.tile = 184 + meta1;
                entity.colour = EntityColour_TROPHY_TOWER;
            }
            break;
        case 4:
            if (game.bestrank[TimeTrial_SPACESTATION2] >= 3)
            {
                entity.tile = 184 + meta1;
                entity.colour = EntityColour_TROPHY_SPACE_STATION_2;
            }
            break;
        case 5:
            if (game.bestrank[TimeTrial_WARPZONE] >= 3)
            {
                entity.tile = 184 + meta1;
                entity.colour = EntityColour_TROPHY_WARP_ZONE;
            }
            break;
        case 6:
            if (game.bestrank[TimeTrial_FINALLEVEL] >= 3)
            {
                entity.tile = 184 + meta1;
                entity.colour = EntityColour_TROPHY_FINAL_LEVEL;
            }
            break;

        case 7:
            if (game.unlock[UnlockTrophy_GAME_COMPLETE])
            {
                entity.tile = 188 + meta1;
                entity.colour = EntityColour_TROPHY_GAME_COMPLETE;
                entity.h += 3;
                entity.yp -= 3;
            }
            break;
        case 8:
            if (game.unlock[UnlockTrophy_FLIPMODE_COMPLETE])
            {
                entity.tile = 188 + meta1;
                entity.colour = EntityColour_TROPHY_GAME_COMPLETE;
                entity.h += 3;
            }
            break;

        case 9:
            if (game.bestgamedeaths > -1)
            {
                if (game.bestgamedeaths <= 50)
                {
                    entity.tile = 182 + meta1;
                    entity.colour = EntityColour_TROPHY_FLASHY;
                }
            }
            break;
        case 10:
            if (game.bestgamedeaths > -1)
            {
                if (game.bestgamedeaths <= 100)
                {
                    entity.tile = 182 + meta1;
                    entity.colour = EntityColour_TROPHY_GOLD;
                }
            }
            break;
        case 11:
            if (game.bestgamedeaths > -1)
            {
                if (game.bestgamedeaths <= 250)
                {
                    entity.tile = 182 + meta1;
                    entity.colour = EntityColour_TROPHY_SILVER;
                }
            }
            break;
        case 12:
            if (game.bestgamedeaths > -1)
            {
                if (game.bestgamedeaths <= 500)
                {
                    entity.tile = 182 + meta1;
                    entity.colour = EntityColour_TROPHY_BRONZE;
                }
            }
            break;

        case 13:
            if(game.swnbestrank>=1)
            {
                entity.tile = 182 + meta1;
                entity.colour = EntityColour_TROPHY_BRONZE;
            }
            break;
        case 14:
            if(game.swnbestrank>=2)
            {
                entity.tile = 182 + meta1;
                entity.colour = EntityColour_TROPHY_BRONZE;
            }
            break;
        case 15:
            if(game.swnbestrank>=3)
            {
                entity.tile = 182 + meta1;
                entity.colour = EntityColour_TROPHY_BRONZE;
            }
            break;
        case 16:
            if(game.swnbestrank>=4)
            {
                entity.tile = 182 + meta1;
                entity.colour = EntityColour_TROPHY_SILVER;
            }
            break;
        case 17:
            if(game.swnbestrank>=5)
            {
                entity.tile = 182 + meta1;
                entity.colour = EntityColour_TROPHY_GOLD;
            }
            break;
        case 18:
            if(game.swnbestrank>=6)
            {
                entity.tile = 182 + meta1;
                entity.colour = EntityColour_TROPHY_FLASHY;
            }
            break;

        case 19:
            if (game.unlock[UnlockTrophy_NODEATHMODE_COMPLETE])
            {
                entity.tile = 3;
                entity.colour = EntityColour_TELEPORTER_FLASHING;
                entity.size = 13;
                entity.xp -= 64;
                entity.yp -= 128;
            }
            break;

        }

        break;
    case 26: //Epilogue super warp token
        entity.rule = 3;
        entity.type = EntityType_WARP_TOKEN;
        entity.size = 0;
        entity.tile = 18;
        entity.w = 16;
        entity.h = 16;
        entity.colour = EntityColour_TRINKET;
        entity.onentity = 0;
        entity.animate = 100;
        entity.para = meta2;
        entity.size = 13;
        break;

    /* Warp lines */
    case 51: /* Vertical */
    case 52: /* Vertical */
    case 53: /* Horizontal */
    case 54: /* Horizontal */
        if (t == 51)
        {
            entity.type = EntityType_WARP_LINE_LEFT;
        }
        else if (t == 52)
        {
            entity.type = EntityType_WARP_LINE_RIGHT;
        }
        else if (t == 53)
        {
            entity.type = EntityType_WARP_LINE_TOP;
        }
        else
        {
            entity.type = EntityType_WARP_LINE_BOTTOM;
        }

        entity.onentity = 1;
        entity.invis = true;
        entity.life = 0;
        switch (t)
        {
        case 51:
        case 52:
            entity.rule = 5;
            entity.size = 6;
            entity.w = 1;
            entity.h = meta1;
            break;
        case 53:
        case 54:
            entity.rule = 7;
            entity.size = 5;
            entity.w = meta1;
            entity.h = 1;
            break;
        }
        if (map.custommode)
        {
            customwarpmode = true;
            map.warpx = false;
            map.warpy = false;
        }
        break;
      case 55: // Crew Member (custom, collectable)
        //1 - position in array
        //2 - colour
        entity.rule = 3;
        entity.type = EntityType_COLLECTABLE_CREWMATE;
        if(INBOUNDS_ARR(meta2, customcrewmoods)
        && customcrewmoods[meta2]==1){
          entity.tile = 144;
        }else{
          entity.tile = 0;
        }
        entity.colour = graphics.crewcolour(meta2);
        entity.cx = 6;
        entity.cy = 2;
        entity.w = 12;
        entity.h = 21;
        entity.dir = 0;

        entity.state = 0;
        entity.onentity = 1;
        //entity.state = meta1;

        entity.gravity = true;

        //Check if it's already been collected
        entity.para = meta1;
        if (!INBOUNDS_ARR(meta1, customcollect) || customcollect[meta1]) return;
        break;
      case 56: //Custom enemy
        entity.rule = 1;
        entity.type = EntityType_MOVING;
        entity.behave = meta1;
        entity.para = meta2;
        entity.w = 16;
        entity.h = 16;
        entity.cx = 0;
        entity.cy = 0;

        entity.x1 = p1;
        entity.y1 = p2;
        entity.x2 = p3;
        entity.y2 = p4;

        entity.harmful = true;

        switch(customenemy){
          case 0: entity.setenemyroom(4+100, 0+100); break;
          case 1: entity.setenemyroom(2+100, 0+100); break;
          case 2: entity.setenemyroom(12+100, 3+100); break;
          case 3: entity.setenemyroom(13+100, 12+100); break;
          case 4: entity.setenemyroom(16+100, 9+100); break;
          case 5: entity.setenemyroom(19+100, 1+100); break;
          case 6: entity.setenemyroom(19+100, 2+100); break;
          case 7: entity.setenemyroom(18+100, 3+100); break;
          case 8: entity.setenemyroom(16+100, 0+100); break;
          case 9: entity.setenemyroom(14+100, 2+100); break;
          default: entity.setenemyroom(4+100, 0+100); break;
        }

        //Set colour based on room tile
         //Set custom colours
        if(customplatformtile>0){
          int entcol=(customplatformtile/12);
          switch(entcol){
            //RED
            case 3: case 7: case 12: case 23: case 28:
            case 34: case 42: case 48: case 58:
              entity.colour = EntityColour_ENEMY_RED; break;
            //GREEN
            case 5: case 9: case 22: case 25: case 29:
            case 31: case 38: case 46: case 52: case 53:
              entity.colour = EntityColour_ENEMY_GREEN; break;
            //BLUE
            case 1: case 6: case 14: case 27: case 33:
            case 44: case 50: case 57:
              entity.colour = EntityColour_ENEMY_BLUE; break;
            //YELLOW
            case 4: case 17: case 24: case 30: case 37:
            case 45: case 51: case 55:
              entity.colour = EntityColour_ENEMY_YELLOW; break;
            //PURPLE
            case 2: case 11: case 15: case 19: case 32:
            case 36: case 49:
              entity.colour = EntityColour_CREW_PURPLE; break;
            //CYAN
            case 8: case 10: case 13: case 18: case 26:
            case 35: case 41: case 47: case 54:
              entity.colour = EntityColour_ENEMY_CYAN; break;
            //PINK
            case 16: case 20: case 39: case 43: case 56:
              entity.colour = EntityColour_ENEMY_PINK; break;
            //ORANGE
            case 21: case 40:
              entity.colour = EntityColour_ENEMY_ORANGE; break;
            default:
              entity.colour = EntityColour_ENEMY_RED;
            break;
          }
        }

        if(custom_gray){
          entity.colour = EntityColour_ENEMY_GRAY;
        }

        entityclonefix(&entity);
        break;
    case 100: // Invalid enemy, but gets treated as a teleporter
        entity.type = EntityType_TELEPORTER;
        break;
    }

    entity.lerpoldxp = entity.xp;
    entity.lerpoldyp = entity.yp;
    entity.drawframe = entity.tile;

    if (!reuse)
    {
        entities.push_back(entity);
    }

    /* Fix crewmate facing directions
     * This is a bit kludge-y but it's better than copy-pasting
     * and is okay to do because entity 12 does not change state on its own
     */
    if (entity.type == EntityType_CREWMATE)
    {
        size_t indice;
        if (reuse)
        {
            indice = entptr - entities.data();
        }
        else
        {
            indice = entities.size() - 1;
        }
        updateentities(indice);
    }
}

void entityclass::createentity(int xp, int yp, int t, int meta1, int meta2, int p1, int p2)
{
    createentity(xp, yp, t, meta1, meta2, p1, p2, 320, 240);
}

void entityclass::createentity(int xp, int yp, int t, int meta1, int meta2, int p1)
{
    createentity(xp, yp, t, meta1, meta2, p1, 0);
}

void entityclass::createentity(int xp, int yp, int t, int meta1, int meta2)
{
    createentity(xp, yp, t, meta1, meta2, 0);
}

void entityclass::createentity(int xp, int yp, int t, int meta1)
{
    createentity(xp, yp, t, meta1, 0);
}

void entityclass::createentity(int xp, int yp, int t)
{
    createentity(xp, yp, t, 0);
}

//Returns true if entity is removed
bool entityclass::updateentities( int i )
{
    if (!INBOUNDS_VEC(i, entities))
    {
        vlog_error("updateentities() out-of-bounds!");
        return true;
    }

    if(entities[i].statedelay<=0)
    {
        switch(entities[i].type)
        {
        case EntityType_PLAYER:  //Player
            break;
        case EntityType_MOVING:  //Movement behaviors
            //Enemies can have a number of different behaviors:
            switch(entities[i].behave)
            {
            case 0: //Bounce, Start moving down
                if (entities[i].state == 0)   //Init
                {
                    entities[i].state = 3;
                    bool entitygone = updateentities(i);
                    if (entitygone) return true;
                }
                else if (entities[i].state == 1)
                {
                    if (entities[i].outside()) entities[i].state = entities[i].onwall;
                }
                else if (entities[i].state == 2)
                {
                    entities[i].vy = -entities[i].para;
                    entities[i].onwall = 3;
                    entities[i].state = 1;
                }
                else if (entities[i].state == 3)
                {
                    entities[i].vy = entities[i].para;
                    entities[i].onwall = 2;
                    entities[i].state = 1;
                }
                break;
            case 1: //Bounce, Start moving up
                if (entities[i].state == 0)   //Init
                {
                    entities[i].state = 2;
                    bool entitygone = updateentities(i);
                    if (entitygone) return true;
                }
                else if (entities[i].state == 1)
                {
                    if (entities[i].outside()) entities[i].state = entities[i].onwall;
                }
                else if (entities[i].state == 2)
                {
                    entities[i].vy = -entities[i].para;
                    entities[i].onwall = 3;
                    entities[i].state = 1;
                }
                else if (entities[i].state == 3)
                {
                    entities[i].vy = entities[i].para;
                    entities[i].onwall = 2;
                    entities[i].state = 1;
                }
                break;
            case 2: //Bounce, Start moving left
                if (entities[i].state == 0)   //Init
                {
                    entities[i].state = 3;
                    bool entitygone = updateentities(i);
                    if (entitygone) return true;
                }
                else if (entities[i].state == 1)
                {
                    if (entities[i].outside()) entities[i].state = entities[i].onwall;
                }
                else if (entities[i].state == 2)
                {
                    entities[i].vx = entities[i].para;
                    entities[i].onwall = 3;
                    entities[i].state = 1;
                }
                else if (entities[i].state == 3)
                {
                    entities[i].vx = -entities[i].para;
                    entities[i].onwall = 2;
                    entities[i].state = 1;
                }
                break;
            case 3: //Bounce, Start moving right
                if (entities[i].state == 0)   //Init
                {
                    entities[i].state = 3;
                    bool entitygone = updateentities(i);
                    if (entitygone) return true;
                }
                else if (entities[i].state == 1)
                {
                    if (entities[i].outside()) entities[i].state = entities[i].onwall;
                }
                else if (entities[i].state == 2)
                {
                    entities[i].vx = -entities[i].para;
                    entities[i].onwall = 3;
                    entities[i].state = 1;
                }
                else if (entities[i].state == 3)
                {
                    entities[i].vx = entities[i].para;
                    entities[i].onwall = 2;
                    entities[i].state = 1;
                }
                break;
            case 4: //Always move left
                if (entities[i].state == 0)   //Init
                {
                    entities[i].vx = entities[i].para;
                }
                break;
            case 5: //Always move right
                if (entities[i].state == 0)
                {
                    //Init
                    entities[i].vx = static_cast<int>(entities[i].para);
                    entities[i].state = 1;
                    entities[i].onwall = 2;
                }
                else if (entities[i].state == 2)
                {
                    entities[i].vx = 0;
                    entities[i].onwall = 0;
                    entities[i].xp -=  static_cast<int>(entities[i].para);
                    entities[i].statedelay=8;
                    entities[i].state=0;
                }
                break;
            case 6: //Always move up
                if (entities[i].state == 0)   //Init
                {
                    entities[i].vy = static_cast<int>(entities[i].para);
                    entities[i].state = 1;
                    entities[i].onwall = 2;
                }
                else if (entities[i].state == 2)
                {
                    entities[i].vy = static_cast<int>(-entities[i].para);
                    entities[i].onwall = 0;
                    entities[i].yp -=  (entities[i].para);
                    entities[i].statedelay=8;
                    entities[i].state=0;
                }
                break;
            case 7: //Always move down
                if (entities[i].state == 0)   //Init
                {
                    entities[i].vx = static_cast<int>(entities[i].para);
                }
                break;
            case 8:
            case 9:
                //Threadmill: don't move, just impart velocity
                if (entities[i].state == 0)   //Init
                {
                    entities[i].vx = 0;
                    entities[i].state = 1;
                    entities[i].onwall = 0;
                }
                break;
            case 10:
                //Emitter: shoot an enemy every so often
                if (entities[i].state == 0)
                {
                    createentity(entities[i].xp+28, entities[i].yp, 1, 10, 1);
                    entities[i].state = 1;
                    entities[i].statedelay = 12;
                }
                else if (entities[i].state == 1)
                {
                    entities[i].state = 0;
                }
                break;
            case 11: //Always move right, destroy when outside screen
                if (entities[i].state == 0)   //Init
                {
                    entities[i].vx = entities[i].para;
                    entities[i].state = 1;
                }
                else if (entities[i].state == 1)
                {
                    if (entities[i].xp >= 335)
                    {
                        return disableentity(i);
                    }
                    if (game.roomx == 117)
                    {
                        if (entities[i].xp >= (33*8)-32)
                        {
                            return disableentity(i);
                        }
                        //collector for LIES
                    }
                }
                break;
            case 12:
                //Emitter: shoot an enemy every so often (up)
                if (entities[i].state == 0)
                {
                    createentity(entities[i].xp, entities[i].yp, 1, 12, 1);
                    entities[i].state = 1;
                    entities[i].statedelay = 16;
                }
                else if (entities[i].state == 1)
                {
                    entities[i].state = 0;
                }
                break;
            case 13: //Always move up, destroy when outside screen
                if (entities[i].state == 0)   //Init
                {
                    entities[i].vy = entities[i].para;
                    entities[i].state = 1;
                }
                else if (entities[i].state == 1)
                {
                    if (entities[i].yp <= -60)
                    {
                        return disableentity(i);
                    }
                    if (game.roomx == 113 && game.roomy == 108)
                    {
                        if (entities[i].yp <= 60)
                        {
                            return disableentity(i);
                        }
                        //collector for factory
                    }
                }
                break;
            case 14: //Very special hack: as two, but doesn't move in specific circumstances
                if (entities[i].state == 0)   //Init
                {
                    for (size_t j = 0; j < entities.size(); j++)
                    {
                        if (entities[j].type == EntityType_DISAPPEARING_PLATFORM && entities[j].state== 3 && entities[j].xp == (entities[i].xp-32) )
                        {
                            entities[i].state = 3;
                            bool entitygone = updateentities(i);
                            if (entitygone) return true;
                        }
                    }
                }
                else if (entities[i].state == 1)
                {
                    if (entities[i].outside()) entities[i].state = entities[i].onwall;
                }
                else if (entities[i].state == 2)
                {
                    entities[i].vx = entities[i].para;
                    entities[i].onwall = 3;
                    entities[i].state = 1;
                }
                else if (entities[i].state == 3)
                {
                    entities[i].vx = -entities[i].para;
                    entities[i].onwall = 2;
                    entities[i].state = 1;
                }
                break;
            case 15: //As above, but for 3!
                if (entities[i].state == 0)   //Init
                {
                    for (size_t j = 0; j < entities.size(); j++)
                    {
                        if (entities[j].type == EntityType_DISAPPEARING_PLATFORM && entities[j].state==3 && entities[j].xp==entities[i].xp+32)
                        {
                            entities[i].state = 3;
                            bool entitygone = updateentities(i);
                            if (entitygone) return true;
                        }
                    }
                }
                else if (entities[i].state == 1)
                {
                    if (entities[i].outside()) entities[i].state = entities[i].onwall;
                }
                else if (entities[i].state == 2)
                {
                    entities[i].vx = -entities[i].para;
                    entities[i].onwall = 3;
                    entities[i].state = 1;
                }
                else if (entities[i].state == 3)
                {
                    entities[i].vx = entities[i].para;
                    entities[i].onwall = 2;
                    entities[i].state = 1;
                }
                break;
            case 16: //MAVERICK BUS FOLLOWS HIS OWN RULES
                if (entities[i].state == 0)   //Init
                {
                    int player = getplayer();
                    //first, y position
                    if (INBOUNDS_VEC(player, entities) && entities[player].yp > 14 * 8)
                    {
                        entities[i].tile = 120;
                        entities[i].yp = (28*8)-62;
                        entities[i].lerpoldyp = (28*8)-62;
                    }
                    else
                    {
                        entities[i].tile = 96;
                        entities[i].yp = 24;
                        entities[i].lerpoldyp = 24;
                    }
                    //now, x position
                    if (INBOUNDS_VEC(player, entities) && entities[player].xp > 20 * 8)
                    {
                        //approach from the left
                        entities[i].xp = -64;
                        entities[i].lerpoldxp = -64;
                        entities[i].state = 2;
                        bool entitygone = updateentities(i); //right
                        if (entitygone) return true;
                    }
                    else
                    {
                        //approach from the left
                        entities[i].xp = 320;
                        entities[i].lerpoldxp = 320;
                        entities[i].state = 3;
                        bool entitygone = updateentities(i); //left
                        if (entitygone) return true;
                    }

                }
                else if (entities[i].state == 1)
                {
                    if (entities[i].outside()) entities[i].state = entities[i].onwall;
                }
                else if (entities[i].state == 2)
                {
                    entities[i].vx = int(entities[i].para);
                    entities[i].onwall = 3;
                    entities[i].state = 1;
                }
                else if (entities[i].state == 3)
                {
                    entities[i].vx = int(-entities[i].para);
                    entities[i].onwall = 2;
                    entities[i].state = 1;
                }
                break;
            case 17: //Special for ASCII Snake (left)
                if (entities[i].state == 0)   //Init
                {
                    entities[i].statedelay = 6;
                    entities[i].xp -=  int(entities[i].para);
                    entities[i].lerpoldxp -=  int(entities[i].para);
                }
                break;
            case 18: //Special for ASCII Snake (right)
                if (entities[i].state == 0)   //Init
                {
                    entities[i].statedelay = 6;
                    entities[i].xp += int(entities[i].para);
                    entities[i].lerpoldxp += int(entities[i].para);
                }
                break;
            }
            break;
        case EntityType_DISAPPEARING_PLATFORM: //Disappearing platforms
            //wait for collision
            if (entities[i].state == 1)
            {
                entities[i].life = 12;
                entities[i].state = 2;
                entities[i].onentity = 0;

                music.playef(Sound_DISAPPEAR);
            }
            else if (entities[i].state == 2)
            {
                entities[i].life--;
                if (entities[i].life % 3 == 0) entities[i].walkingframe++;
                if (entities[i].life <= 0)
                {
                    disableblockat(entities[i].xp, entities[i].yp);
                    entities[i].state = 3;// = false;
                    entities[i].invis = true;
                }
            }
            else if (entities[i].state == 3)
            {
                //wait until recharged!
            }
            else if (entities[i].state == 4)
            {
                //restart!
                createblock(0, entities[i].xp, entities[i].yp, 32, 8);
                entities[i].state = 4;
                entities[i].invis = false;
                entities[i].walkingframe--;
                entities[i].state++;
                entities[i].onentity = 1;
            }
            else if (entities[i].state == 5)
            {
                entities[i].life+=3;
                if (entities[i].life % 3 == 0) entities[i].walkingframe--;
                if (entities[i].life >= 12)
                {
                    entities[i].life = 12;
                    entities[i].state = 0;
                    entities[i].walkingframe++;
                }
            }
            break;
        case EntityType_QUICKSAND: //Breakable blocks
            //Only counts if vy of player entity is non zero
            if (entities[i].state == 1)
            {
                entities[i].life = 4;
                entities[i].state = 2;
                entities[i].onentity = 0;
                music.playef(Sound_CRUMBLE);
            }
            else if (entities[i].state == 2)
            {
                entities[i].life--;
                entities[i].tile++;
                if (entities[i].life <= 0)
                {
                    disableblockat(entities[i].xp, entities[i].yp);
                    return disableentity(i);
                }
            }
            break;
        case EntityType_GRAVITY_TOKEN: //Gravity token
            //wait for collision
            if (entities[i].state == 1)
            {
                game.gravitycontrol = (game.gravitycontrol + 1) % 2;
                ++game.totalflips;
                return disableentity(i);

            }
            break;
        case EntityType_PARTICLE:  //Particle sprays
            if (entities[i].state == 0)
            {
                entities[i].life--;
                if (entities[i].life < 0)
                {
                    return disableentity(i);
                }
            }
            break;
        case EntityType_COIN: //Small pickup
            //wait for collision
            if (entities[i].state == 1)
            {
                music.playef(Sound_COIN);
                if (INBOUNDS_ARR(entities[i].para, collect))
                {
                    collect[(int) entities[i].para] = true;
                }

                return disableentity(i);
            }
            break;
        case EntityType_TRINKET: //Found a trinket
            //wait for collision
            if (entities[i].state == 1)
            {
                if (INBOUNDS_ARR(entities[i].para, collect))
                {
                    collect[(int) entities[i].para] = true;
                }

                if (game.intimetrial)
                {
                    music.playef(Sound_NEWRECORD);
                }
                else
                {
                    game.setstate(1000);
                    if(music.currentsong!=-1) music.silencedasmusik();
                    music.playef(Sound_TRINKET);
                    if (game.trinkets() > game.stat_trinkets && !map.custommode)
                    {
                        game.stat_trinkets = game.trinkets();
                        game.savestatsandsettings();
                    }
                }

                return disableentity(i);
            }
            break;
        case EntityType_CHECKPOINT: //Savepoints
            //wait for collision
            if (entities[i].state == 1)
            {
                //First, deactivate all other savepoints
                for (size_t j = 0; j < entities.size(); j++)
                {
                    if (entities[j].type == EntityType_CHECKPOINT)
                    {
                        entities[j].colour = EntityColour_INACTIVE_ENTITY;
                        entities[j].onentity = 1;
                    }
                }
                entities[i].colour = EntityColour_ACTIVE_ENTITY;
                entities[i].onentity = 0;
                game.savepoint = entities[i].para;
                music.playef(Sound_CHECKPOINT);

                game.savex = entities[i].xp - 4;

                if (entities[i].tile == 20)
                {
                    game.savey = entities[i].yp - 2;
                    game.savegc = 1;
                }
                else if (entities[i].tile == 21)
                {
                    game.savey = entities[i].yp - 7;
                    game.savegc = 0;
                }

                game.saverx = game.roomx;
                game.savery = game.roomy;
                int player = getplayer();
                if (INBOUNDS_VEC(player, entities))
                {
                    game.savedir = entities[player].dir;
                }
                entities[i].state = 0;

                game.checkpoint_save();
            }
            break;
        case EntityType_HORIZONTAL_GRAVITY_LINE: //Gravity Lines
            if (entities[i].state == 1)
            {
                entities[i].life--;
                entities[i].onentity = 0;

                if (entities[i].life <= 0)
                {
                    entities[i].state = 0;
                    entities[i].onentity = 1;
                }
            }
            break;
        case EntityType_VERTICAL_GRAVITY_LINE: //Vertical gravity Lines
            if (entities[i].state == 1)
            {
                entities[i].onentity = 3;
                entities[i].state = 2;


                music.playef(Sound_GRAVITYLINE);
                game.gravitycontrol = (game.gravitycontrol + 1) % 2;
                game.totalflips++;
                int temp = getplayer();
                if (game.gravitycontrol == 0)
                {
                    if (INBOUNDS_VEC(temp, entities) && entities[temp].vy < 3) entities[temp].vy = 3;
                }
                else
                {
                    if (INBOUNDS_VEC(temp, entities) && entities[temp].vy > -3) entities[temp].vy = -3;
                }
            }
            else if (entities[i].state == 2)
            {
                entities[i].life--;
                if (entities[i].life <= 0)
                {
                    entities[i].state = 0;
                    entities[i].onentity = 1;
                }
            }
            else if (entities[i].state == 3)
            {
                entities[i].state = 2;
                entities[i].life = 4;
                entities[i].onentity = 3;
            }
            else if (entities[i].state == 4)
            {
                //Special case for room initilisations: As state one, except without the reversal
                entities[i].onentity = 3;
                entities[i].state = 2;
            }
            break;
        case EntityType_WARP_TOKEN: //Warp point
            //wait for collision
            if (entities[i].state == 1)
            {
                //Depending on the room the warp point is in, teleport to a new location!
                entities[i].onentity = 0;
                //play a sound or somefink
                music.playef(Sound_TELEPORT);
                game.teleport = true;

                game.edteleportent = i;
                //for the multiple room:
                if (int(entities[i].xp) == 12*8) game.teleportxpos = 1;
                if (int(entities[i].xp) == 5*8) game.teleportxpos = 2;
                if (int(entities[i].xp) == 28*8) game.teleportxpos = 3;
                if (int(entities[i].xp) == 21*8) game.teleportxpos = 4;
            }
            break;
        case EntityType_CREWMATE: //Crew member
            //Somewhat complex AI: exactly what they do depends on room, location, state etc
            //At state 0, do nothing at all.
            if (entities[i].state == 1)
            {
                //happy!
                if (INBOUNDS_VEC(k, entities) && entities[k].rule == 6)    entities[k].tile = 0;
                if (INBOUNDS_VEC(k, entities) && entities[k].rule == 7)    entities[k].tile = 6;
                //Stay close to the hero!
                int j = getplayer();
                if (INBOUNDS_VEC(j, entities) && entities[j].xp > entities[i].xp + 5)
                {
                    entities[i].dir = 1;
                }
                else if (INBOUNDS_VEC(j, entities) && entities[j].xp < entities[i].xp - 5)
                {
                    entities[i].dir = 0;
                }

                if (INBOUNDS_VEC(j, entities) && entities[j].xp > entities[i].xp + 45)
                {
                    entities[i].ax = 3;
                }
                else if (INBOUNDS_VEC(j, entities) && entities[j].xp < entities[i].xp - 45)
                {
                    entities[i].ax = -3;
                }

                //Special rules:
                if (game.roomx == 110 && game.roomy == 105 && !map.custommode)
                {
                    if (entities[i].xp < 155)
                    {
                        if (entities[i].ax < 0) entities[i].ax = 0;
                    }
                }
            }
            else if (entities[i].state == 2)
            {
                //Basic rules, don't change expression
                int j = getplayer();
                if (INBOUNDS_VEC(j, entities) && entities[j].xp > entities[i].xp + 5)
                {
                    entities[i].dir = 1;
                }
                else if (INBOUNDS_VEC(j, entities) && entities[j].xp < entities[i].xp - 5)
                {
                    entities[i].dir = 0;
                }

                if (INBOUNDS_VEC(j, entities) && entities[j].xp > entities[i].xp + 45)
                {
                    entities[i].ax = 3;
                }
                else if (INBOUNDS_VEC(j, entities) && entities[j].xp < entities[i].xp - 45)
                {
                    entities[i].ax = -3;
                }
            }
            else if (entities[i].state == 10)
            {
                //Everything from 10 on is for cutscenes
                //Basic rules, don't change expression
                int j = getplayer();
                if (INBOUNDS_VEC(j, entities) && entities[j].xp > entities[i].xp + 5)
                {
                    entities[i].dir = 1;
                }
                else if (INBOUNDS_VEC(j, entities) && entities[j].xp < entities[i].xp - 5)
                {
                    entities[i].dir = 0;
                }

                if (INBOUNDS_VEC(j, entities) && entities[j].xp > entities[i].xp + 45)
                {
                    entities[i].ax = 3;
                }
                else if (INBOUNDS_VEC(j, entities) && entities[j].xp < entities[i].xp - 45)
                {
                    entities[i].ax = -3;
                }
            }
            else if (entities[i].state == 11)
            {
                //11-15 means to follow a specific character, in crew order (cyan, purple, yellow, red, green, blue)
                int j=getcrewman(EntityColour_CREW_PURPLE);
                if (INBOUNDS_VEC(j, entities))
                {
                    if (entities[j].xp > entities[i].xp + 5)
                    {
                        entities[i].dir = 1;
                    }
                    else if (entities[j].xp < entities[i].xp - 5)
                    {
                        entities[i].dir = 0;
                    }

                    if (entities[j].xp > entities[i].xp + 45)
                    {
                        entities[i].ax = 3;
                    }
                    else if (entities[j].xp < entities[i].xp - 45)
                    {
                        entities[i].ax = -3;
                    }
                }
            }
            else if (entities[i].state == 12)
            {
                //11-15 means to follow a specific character, in crew order (cyan, purple, yellow, red, green, blue)
                int j=getcrewman(EntityColour_CREW_YELLOW);
                if (INBOUNDS_VEC(j, entities))
                {
                    if (entities[j].xp > entities[i].xp + 5)
                    {
                        entities[i].dir = 1;
                    }
                    else if (entities[j].xp < entities[i].xp - 5)
                    {
                        entities[i].dir = 0;
                    }

                    if (entities[j].xp > entities[i].xp + 45)
                    {
                        entities[i].ax = 3;
                    }
                    else if (entities[j].xp < entities[i].xp - 45)
                    {
                        entities[i].ax = -3;
                    }
                }
            }
            else if (entities[i].state == 13)
            {
                //11-15 means to follow a specific character, in crew order (cyan, purple, yellow, red, green, blue)
                int j=getcrewman(EntityColour_CREW_RED);
                if (INBOUNDS_VEC(j, entities))
                {
                    if (entities[j].xp > entities[i].xp + 5)
                    {
                        entities[i].dir = 1;
                    }
                    else if (entities[j].xp < entities[i].xp - 5)
                    {
                        entities[i].dir = 0;
                    }

                    if (entities[j].xp > entities[i].xp + 45)
                    {
                        entities[i].ax = 3;
                    }
                    else if (entities[j].xp < entities[i].xp - 45)
                    {
                        entities[i].ax = -3;
                    }
                }
            }
            else if (entities[i].state == 14)
            {
                //11-15 means to follow a specific character, in crew order (cyan, purple, yellow, red, green, blue)
                int j=getcrewman(EntityColour_CREW_GREEN);
                if (INBOUNDS_VEC(j, entities))
                {
                    if (entities[j].xp > entities[i].xp + 5)
                    {
                        entities[i].dir = 1;
                    }
                    else if (entities[j].xp < entities[i].xp - 5)
                    {
                        entities[i].dir = 0;
                    }

                    if (entities[j].xp > entities[i].xp + 45)
                    {
                        entities[i].ax = 3;
                    }
                    else if (entities[j].xp < entities[i].xp - 45)
                    {
                        entities[i].ax = -3;
                    }
                }
            }
            else if (entities[i].state == 15)
            {
                //11-15 means to follow a specific character, in crew order (cyan, purple, yellow, red, green, blue)
                int j=getcrewman(EntityColour_CREW_BLUE);
                if (INBOUNDS_VEC(j, entities))
                {
                    if (entities[j].xp > entities[i].xp + 5)
                    {
                        entities[i].dir = 1;
                    }
                    else if (entities[j].xp < entities[i].xp - 5)
                    {
                        entities[i].dir = 0;
                    }

                    if (entities[j].xp > entities[i].xp + 45)
                    {
                        entities[i].ax = 3;
                    }
                    else if (entities[j].xp < entities[i].xp - 45)
                    {
                        entities[i].ax = -3;
                    }
                }
            }
            else if (entities[i].state == 16)
            {
                //Follow a position: given an x coordinate, seek it out.
                if (entities[i].para > entities[i].xp + 5)
                {
                    entities[i].dir = 1;
                }
                else if (entities[i].para < entities[i].xp - 5)
                {
                    entities[i].dir = 0;
                }

                if (entities[i].para > entities[i].xp + 45)
                {
                    entities[i].ax = 3;
                }
                else if (entities[i].para < entities[i].xp - 45)
                {
                    entities[i].ax = -3;
                }
            }
            else if (entities[i].state == 17)
            {
                //stand still
            }
            else if (entities[i].state == 18)
            {
                //Stand still and face the player
                int j = getplayer();
                if (INBOUNDS_VEC(j, entities) && entities[j].xp > entities[i].xp + 5)
                {
                    entities[i].dir = 1;
                }
                else if (INBOUNDS_VEC(j, entities) && entities[j].xp < entities[i].xp - 5)
                {
                    entities[i].dir = 0;
                }
            }
            else if (entities[i].state == 19)
            {
                //Walk right off the screen after time t
                if (entities[i].para <= 0)
                {
                    entities[i].dir = 1;
                    entities[i].ax = 3;
                }
                else
                {
                    entities[i].para--;
                }
            }
            else if (entities[i].state == 20)
            {
                //Panic! For briefing script
                if (entities[i].life == 0)
                {
                    //walk left for a bit
                    entities[i].ax = 0;
                    if (40 > entities[i].xp + 5)
                    {
                        entities[i].dir = 1;
                    }
                    else if (40 < entities[i].xp - 5)
                    {
                        entities[i].dir = 0;
                    }

                    if (40 > entities[i].xp + 45)
                    {
                        entities[i].ax = 3;
                    }
                    else if (40 < entities[i].xp - 45)
                    {
                        entities[i].ax = -3;
                    }
                    if ( (entities[i].ax) == 0)
                    {
                        entities[i].life = 1;
                        entities[i].para = 30;
                    }
                }
                else    if (entities[i].life == 1)
                {
                    //Stand around for a bit
                    entities[i].para--;
                    if (entities[i].para <= 0)
                    {
                        entities[i].life++;
                    }
                }
                else if (entities[i].life == 2)
                {
                    //walk right for a bit
                    entities[i].ax = 0;
                    if (280 > entities[i].xp + 5)
                    {
                        entities[i].dir = 1;
                    }
                    else if (280 < entities[i].xp - 5)
                    {
                        entities[i].dir = 0;
                    }

                    if (280 > entities[i].xp + 45)
                    {
                        entities[i].ax = 3;
                    }
                    else if (280 < entities[i].xp - 45)
                    {
                        entities[i].ax = -3;
                    }
                    if ( (entities[i].ax) == 0)
                    {
                        entities[i].life = 3;
                        entities[i].para = 30;
                    }
                }
                else    if (entities[i].life == 3)
                {
                    //Stand around for a bit
                    entities[i].para--;
                    if (entities[i].para <= 0)
                    {
                        entities[i].life=0;
                    }
                }
            }
            break;
        case EntityType_TERMINAL: //Terminals (very similar to savepoints)
            //wait for collision
            if (entities[i].state == 1)
            {
                entities[i].colour = EntityColour_ACTIVE_ENTITY;
                entities[i].onentity = 0;
                music.playef(Sound_TERMINALTOUCH);

                entities[i].state = 0;
            }
            break;
        case EntityType_SUPERCREWMATE: //Super Crew member
            //Actually needs less complex AI than the scripting crewmember
            if (entities[i].state == 0)
            {
                //follow player, but only if he's on the floor!
                int j = getplayer();
                if(INBOUNDS_VEC(j, entities) && entities[j].onground>0)
                {
                    if (entities[j].xp > entities[i].xp + 5)
                    {
                        entities[i].dir = 1;
                    }
                    else if (entities[j].xp>15 && entities[j].xp < entities[i].xp - 5)
                    {
                        entities[i].dir = 0;
                    }

                    if (entities[j].xp > entities[i].xp + 45)
                    {
                        entities[i].ax = 3;
                    }
                    else if (entities[j].xp < entities[i].xp - 45)
                    {
                        entities[i].ax = -3;
                    }
                    if (entities[i].ax < 0 && entities[i].xp < 60)
                    {
                        entities[i].ax = 0;
                    }
                }
                else
                {
                    if (INBOUNDS_VEC(j, entities) && entities[j].xp > entities[i].xp + 5)
                    {
                        entities[i].dir = 1;
                    }
                    else if (INBOUNDS_VEC(j, entities) && entities[j].xp < entities[i].xp - 5)
                    {
                        entities[i].dir = 0;
                    }

                    entities[i].ax = 0;
                }

                if (entities[i].xp > 240)
                {
                    entities[i].ax = 3;
                    entities[i].dir = 1;
                }
                if (entities[i].xp >= 310)
                {
                    game.scmprogress++;
                    return disableentity(i);
                }
            }
            break;
        case EntityType_TROPHY: //Trophy
            //wait for collision
            if (entities[i].state == 1)
            {
                if (!script.running) trophytext+=2;
                if (trophytext > 30) trophytext = 30;
                trophytype = entities[i].para;

                entities[i].state = 0;
            }
            break;
        case EntityType_GRAVITRON_ENEMY:
            //swn game!

            if ((game.swnstate == 0) && game.swndelay == 0)
            {
                entities[i].despawn = true;
            }

            switch(entities[i].behave)
            {
            case 0:
                // Right
                if (entities[i].state == 0)   //Init
                {
                    entities[i].vx = entities[i].para * (entities[i].freeze ? 0 : 1) * (entities[i].reverse ? -1 : 1);
                    
                    if (entities[i].despawn && (entities[i].xp < -20 || entities[i].xp > 324))
                    {
                        return disableentity(i);
                    }
                }
                break;

            case 1:
                // Left
                if (entities[i].state == 0)   //Init
                {
                    entities[i].vx = -entities[i].para * (entities[i].freeze ? 0 : 1) * (entities[i].reverse ? -1 : 1);

                    if (entities[i].despawn && (entities[i].xp < -20 || entities[i].xp > 324))
                    {
                        return disableentity(i);
                    }
                }
                break;

            case 2:
                // Wall
                if (entities[i].state == 0)   //Init
                {
                    if (entities[i].despawn)
                    {
                        return disableentity(i);
                    }
                }
                break;

            case 3:
                // Homing
                if (entities[i].state == 0)   //Init
                {
                    if (entities[i].freeze)
                    {
                        entities[i].vx = 0;
                        entities[i].vy = 0;
                    }
                    else
                    {
                        if (entities[i].xp < entities[getplayer()].xp)
                        {
                            if (entities[i].vx != entities[i].para * (entities[i].reverse ? -1 : 1))
                            {
                                entities[i].vx += (entities[i].reverse ? -1 : 1);
                            }
                        }
                        else
                        {
                            if (entities[i].vx != -entities[i].para * (entities[i].reverse ? -1 : 1))
                            {
                                entities[i].vx -= (entities[i].reverse ? -1 : 1);
                            }
                        }

                        if (entities[i].yp < entities[getplayer()].yp)
                        {
                            if (entities[i].vy != entities[i].para * (entities[i].reverse ? -1 : 1))
                            {
                                entities[i].vy += (entities[i].reverse ? -1 : 1);
                            }
                        }
                        else
                        {
                            if (entities[i].vy != -entities[i].para * (entities[i].reverse ? -1 : 1))
                            {
                                entities[i].vy -= (entities[i].reverse ? -1 : 1);
                            }
                        }
                    }

                    if (entities[i].timer - game.swntimer == 0)
                    {
                        return disableentity(i);
                    }
                }
                break;
            }
            break;

        case EntityType_WARP_LINE_LEFT: //Vertical warp line
            if (entities[i].state == 2){
              int j=getplayer();
              if(INBOUNDS_VEC(j, entities) && entities[j].xp<=307){
                customwarpmodevon=false;
                entities[i].state = 0;
              }
            }else if (entities[i].state == 1)
            {
              entities[i].state = 2;
              entities[i].statedelay = 2;
              entities[i].onentity = 1;
              customwarpmodevon=true;
            }
            break;
        case EntityType_WARP_LINE_RIGHT: //Vertical warp line
            if (entities[i].state == 2){
              int j=getplayer();
              if(INBOUNDS_VEC(j, entities) && entities[j].xp<=307){
                customwarpmodevon=false;
                entities[i].state = 0;
              }
            }else if (entities[i].state == 1)
            {
              entities[i].state = 2;
              entities[i].statedelay = 2;
              entities[i].onentity = 1;
              customwarpmodevon=true;
            }
            break;
          case EntityType_WARP_LINE_TOP: //Warp lines Horizonal
            if (entities[i].state == 2){
              customwarpmodehon=false;
              entities[i].state = 0;
            }else if (entities[i].state == 1)
            {
              entities[i].state = 2;
              entities[i].statedelay = 2;
              entities[i].onentity = 1;
              customwarpmodehon=true;
            }
            break;
        case EntityType_WARP_LINE_BOTTOM: //Warp lines Horizonal
            if (entities[i].state == 2){
              customwarpmodehon=false;
              entities[i].state = 0;
            }else if (entities[i].state == 1)
            {
               entities[i].state = 2;
               entities[i].statedelay = 2;
               entities[i].onentity = 1;
               customwarpmodehon=true;
            }
            break;
        case EntityType_COLLECTABLE_CREWMATE: //Collectable crewmate
            //wait for collision
            if (entities[i].state == 0)
            {
                //Basic rules, don't change expression
                int j = getplayer();
                if (INBOUNDS_VEC(j, entities) && entities[j].xp > entities[i].xp + 5)
                {
                    entities[i].dir = 1;
                }
                else if (INBOUNDS_VEC(j, entities) && entities[j].xp < entities[i].xp - 5)
                {
                    entities[i].dir = 0;
                }
            }
            else if (entities[i].state == 1)
            {
                if (INBOUNDS_ARR(entities[i].para, customcollect))
                {
                    customcollect[(int) entities[i].para] = true;
                }

                if (game.intimetrial)
                {
                    music.playef(Sound_RESCUE);
                }
                else
                {
                    game.setstate(1010);
                    //music.haltdasmusik();
                    if(music.currentsong!=-1) music.silencedasmusik();
                    music.playef(Sound_RESCUE);
                }

                return disableentity(i);
            }
            break;
        case EntityType_TELEPORTER: //The teleporter
            if (entities[i].state == 1)
            {
                //if inactive, activate!
                if (entities[i].tile == 1)
                {
                    music.playef(Sound_GAMESAVED);
                    entities[i].tile = 2;
                    entities[i].colour = EntityColour_TELEPORTER_ACTIVE;
                    if(!game.intimetrial && !game.nodeathmode)
                    {
                        game.setstate(2000);
                        game.setstatedelay(0);
                    }

                    game.activetele = true;
                    game.teleblock.x = entities[i].xp - 32;
                    game.teleblock.y = entities[i].yp - 32;
                    game.teleblock.w = 160;
                    game.teleblock.h = 160;


                    //Alright, let's set this as our savepoint too
                    //First, deactivate all other savepoints
                    for (size_t j = 0; j < entities.size(); j++)
                    {
                        if (entities[j].type == EntityType_CHECKPOINT)
                        {
                            entities[j].colour = EntityColour_INACTIVE_ENTITY;
                            entities[j].onentity = 1;
                        }
                    }
                    game.savepoint = static_cast<int>(entities[i].para);
                    game.savex = entities[i].xp + 44;
                    game.savey = entities[i].yp + 44;
                    game.savegc = 0;

                    game.saverx = game.roomx;
                    game.savery = game.roomy;
                    int player = getplayer();
                    if (INBOUNDS_VEC(player, entities))
                    {
                        game.savedir = entities[player].dir;
                    }
                }

                entities[i].onentity = 0;
                entities[i].state = 0;
            }
            else if (entities[i].state == 2)
            {
                //Initilise the teleporter without changing the game state or playing sound
                entities[i].onentity = 0;
                entities[i].tile = 6;
                entities[i].colour = EntityColour_TELEPORTER_FLASHING;

                game.activetele = true;
                game.teleblock.x = entities[i].xp - 32;
                game.teleblock.y = entities[i].yp - 32;
                game.teleblock.w = 160;
                game.teleblock.h = 160;

                entities[i].state = 0;
            }
            break;
        case EntityType_INVALID: // Invalid entity, do nothing!
            break;
        }
    }
    else
    {
        entities[i].statedelay--;
        if (entities[i].statedelay < 0)
        {
            entities[i].statedelay = 0;
        }
    }

    return false;
}

void entityclass::animateentities( int _i )
{
    if (!INBOUNDS_VEC(_i, entities))
    {
        vlog_error("animateentities() out-of-bounds!");
        return;
    }

    if(entities[_i].statedelay < 1)
    {
        switch(entities[_i].type)
        {
        case EntityType_PLAYER:
            entities[_i].framedelay--;
            if(entities[_i].dir==1)
            {
                entities[_i].drawframe=entities[_i].tile;
            }
            else
            {
                entities[_i].drawframe=entities[_i].tile+3;
            }

            if(entities[_i].visualonground>0 || entities[_i].visualonroof>0)
            {
                if(entities[_i].vx > 0.00f || entities[_i].vx < -0.00f)
                {
                    //Walking
                    if(entities[_i].framedelay<=1)
                    {
                        entities[_i].framedelay=4;
                        entities[_i].walkingframe++;
                    }
                    if (entities[_i].walkingframe >=2) entities[_i].walkingframe=0;
                    entities[_i].drawframe += entities[_i].walkingframe + 1;
                }

                if (entities[_i].visualonroof > 0) entities[_i].drawframe += 6;
                // Stuck in a wall? Then default to gravitycontrol
                if (entities[_i].visualonground > 0 && entities[_i].visualonroof > 0
                && game.gravitycontrol == 0)
                {
                    entities[_i].drawframe -= 6;
                }
            }
            else
            {
                entities[_i].drawframe ++;
                if (game.gravitycontrol == 1)
                {
                    entities[_i].drawframe += 6;
                }
            }

            if (game.deathseq > -1)
            {
                entities[_i].drawframe=13;
                if (entities[_i].dir == 1) entities[_i].drawframe = 12;
                if (game.gravitycontrol == 1) entities[_i].drawframe += 2;
            }
            break;
        case EntityType_MOVING:
        case EntityType_GRAVITRON_ENEMY:
            //Variable animation
            switch(entities[_i].animate)
            {
            case 0:
                //Simple oscilation
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 8;
                    if(entities[_i].actionframe==0)
                    {
                        entities[_i].walkingframe++;
                        if (entities[_i].walkingframe == 4)
                        {
                            entities[_i].walkingframe = 2;
                            entities[_i].actionframe = 1;
                        }
                    }
                    else
                    {
                        entities[_i].walkingframe--;
                        if (entities[_i].walkingframe == -1)
                        {
                            entities[_i].walkingframe = 1;
                            entities[_i].actionframe = 0;
                        }
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += entities[_i].walkingframe;
                break;
            case 1:
                //Simple Loop
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 8;
                    entities[_i].walkingframe++;
                    if (entities[_i].walkingframe == 4)
                    {
                        entities[_i].walkingframe = 0;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += entities[_i].walkingframe;
                break;
            case 2:
                //Simpler Loop (just two frames)
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 2;
                    entities[_i].walkingframe++;
                    if (entities[_i].walkingframe == 2)
                    {
                        entities[_i].walkingframe = 0;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += entities[_i].walkingframe;
                break;
            case 3:
                //Simpler Loop (just two frames, but double sized)
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 2;
                    entities[_i].walkingframe++;
                    if (entities[_i].walkingframe == 2)
                    {
                        entities[_i].walkingframe = 0;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += (entities[_i].walkingframe*2);
                break;
            case 4:
                //Simpler Loop (just two frames, but double sized) (as above but slower)
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 6;
                    entities[_i].walkingframe++;
                    if (entities[_i].walkingframe == 2)
                    {
                        entities[_i].walkingframe = 0;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += (entities[_i].walkingframe*2);
                break;
            case 5:
                //Simpler Loop (just two frames) (slower)
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 6;
                    entities[_i].walkingframe++;
                    if (entities[_i].walkingframe == 2)
                    {
                        entities[_i].walkingframe = 0;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += entities[_i].walkingframe;
                break;
            case 6:
                //Normal Loop (four frames, double sized)
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 4;
                    entities[_i].walkingframe++;
                    if (entities[_i].walkingframe == 4)
                    {
                        entities[_i].walkingframe = 0;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += (entities[_i].walkingframe*2);
                break;
            case 7:
                //Simpler Loop (just two frames) (slower) (with directions!)
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 6;
                    entities[_i].walkingframe++;
                    if (entities[_i].walkingframe == 2)
                    {
                        entities[_i].walkingframe = 0;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += entities[_i].walkingframe;

                if (entities[_i].vx > 0.000f ) entities[_i].drawframe += 2;
                break;
            case 10:
                //Threadmill left
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 3;//(6-entities[_i].para);
                    entities[_i].walkingframe--;
                    if (entities[_i].walkingframe == -1)
                    {
                        entities[_i].walkingframe = 3;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += entities[_i].walkingframe;
                break;
            case 11:
                //Threadmill right
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 3;//(6-entities[_i].para);
                    entities[_i].walkingframe++;
                    if (entities[_i].walkingframe == 4)
                    {
                        entities[_i].walkingframe = 0;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += entities[_i].walkingframe;
                break;
            case 100:
                //Simple case for no animation (platforms, etc)
                entities[_i].drawframe = entities[_i].tile;
                break;
            default:
                entities[_i].drawframe = entities[_i].tile;
                break;
            }
            break;
        case EntityType_DISAPPEARING_PLATFORM: //Disappearing platforms
            entities[_i].drawframe = entities[_i].tile + entities[_i].walkingframe;
            break;
        case EntityType_WARP_TOKEN:
            entities[_i].drawframe = entities[_i].tile;
            if(entities[_i].animate==2)
            {
                //Simpler Loop (just two frames)
                entities[_i].framedelay--;
                if(entities[_i].framedelay<=0)
                {
                    entities[_i].framedelay = 10;
                    entities[_i].walkingframe++;
                    if (entities[_i].walkingframe == 2)
                    {
                        entities[_i].walkingframe = 0;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += entities[_i].walkingframe;
            }
            break;
        case EntityType_CREWMATE:
        case EntityType_COLLECTABLE_CREWMATE:
        case EntityType_SUPERCREWMATE: //Crew member! Very similar to hero
            entities[_i].framedelay--;
            if(entities[_i].dir==1)
            {
                entities[_i].drawframe=entities[_i].tile;
            }
            else
            {
                entities[_i].drawframe=entities[_i].tile+3;
            }

            if(entities[_i].visualonground>0 || entities[_i].visualonroof>0)
            {
                if(entities[_i].vx > 0.0000f || entities[_i].vx < -0.000f)
                {
                    //Walking
                    if(entities[_i].framedelay<=0)
                    {
                        entities[_i].framedelay=4;
                        entities[_i].walkingframe++;
                    }
                    if (entities[_i].walkingframe >=2) entities[_i].walkingframe=0;
                    entities[_i].drawframe += entities[_i].walkingframe + 1;
                }

                //if (entities[_i].visualonroof > 0) entities[_i].drawframe += 6;
            }
            else
            {
                entities[_i].drawframe ++;
                //if (game.gravitycontrol == 1) {
                //    entities[_i].drawframe += 6;
                //}
            }

            if (game.deathseq > -1)
            {
                entities[_i].drawframe=13;
                if (entities[_i].dir == 1) entities[_i].drawframe = 12;
                if (entities[_i].rule == 7) entities[_i].drawframe += 2;
                //if (game.gravitycontrol == 1) entities[_i].drawframe += 2;
            }
            break;
        case EntityType_TELEPORTER: //the teleporter!
            if (entities[_i].tile == 1 || game.noflashingmode)
            {
                //it's inactive
                entities[_i].drawframe = entities[_i].tile;
            }
            else if (entities[_i].tile == 2)
            {
                entities[_i].drawframe = entities[_i].tile;

                entities[_i].framedelay--;
                if (entities[_i].framedelay <= 0)
                {
                    entities[_i].framedelay = 1;
                    entities[_i].walkingframe = (int) (fRandom() * 6);
                    if (entities[_i].walkingframe >= 4)
                    {
                        entities[_i].walkingframe = -1;
                        entities[_i].framedelay = 4;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += entities[_i].walkingframe;
            }
            else if (entities[_i].tile == 6)
            {
                //faster!
                entities[_i].drawframe = entities[_i].tile;

                entities[_i].framedelay--;
                if (entities[_i].framedelay <= 0)
                {
                    entities[_i].framedelay = 2;
                    entities[_i].walkingframe = (int) (fRandom() * 6);
                    if (entities[_i].walkingframe >= 4)
                    {
                        entities[_i].walkingframe = -5;
                        entities[_i].framedelay = 4;
                    }
                }

                entities[_i].drawframe = entities[_i].tile;
                entities[_i].drawframe += entities[_i].walkingframe;
            }
            break;
        default:
            entities[_i].drawframe = entities[_i].tile;
            break;
        }
    }
    else
    {
        //entities[_i].statedelay--;
        if (entities[_i].statedelay < 0) entities[_i].statedelay = 0;
    }
}

void entityclass::animatehumanoidcollision(const int i)
{
    /* For some awful reason, drawframe is used for actual collision.
     * And removing the input delay changes collision drawframe
     * because vx is checked in animateentities().
     * So we need to separate the collision drawframe from the visual drawframe
     * and update it separately in gamelogic.
     * Hence this function.
     */
    entclass* entity;

    if (!INBOUNDS_VEC(i, entities))
    {
        vlog_error("animatehumanoidcollision() out-of-bounds!");
        return;
    }

    entity = &entities[i];

    if (!entity->ishumanoid())
    {
        return;
    }

    if (entity->statedelay > 0)
    {
        return;
    }

    --entity->collisionframedelay;

    if (entity->dir == 1)
    {
        entity->collisiondrawframe = entity->tile;
    }
    else
    {
        entity->collisiondrawframe = entity->tile + 3;
    }

    if (entity->visualonground > 0 || entity->visualonroof > 0)
    {
        if (entity->vx > 0.0f || entity->vx < -0.0f)
        {
            /* Walking */
            if (entity->collisionframedelay <= 1)
            {
                entity->collisionframedelay = 4;
                ++entity->collisionwalkingframe;
            }

            if (entity->collisionwalkingframe >= 2)
            {
                entity->collisionwalkingframe = 0;
            }

            entity->collisiondrawframe += entity->collisionwalkingframe + 1;
        }

        if (entity->visualonroof > 0)
        {
            entity->collisiondrawframe += 6;
        }
    }
    else
    {
        ++entity->collisiondrawframe;

        if (entity->type == EntityType_PLAYER && game.gravitycontrol == 1)
        {
            entity->collisiondrawframe += 6;
        }
    }

    /* deathseq shouldn't matter, but handling it anyway just in case */
    if (game.deathseq > -1)
    {
        entity->collisiondrawframe = 13;

        if (entity->dir == 1)
        {
            entity->collisiondrawframe = 12;
        }

        if ((entity->type == EntityType_PLAYER && game.gravitycontrol == 1)
        || (entity->type != EntityType_PLAYER && entity->rule == 7))
        {
            entity->collisiondrawframe += 2;
        }
    }

    entity->framedelay = entity->collisionframedelay;
    entity->drawframe = entity->collisiondrawframe;
    entity->walkingframe = entity->collisionwalkingframe;
}

int entityclass::getcompanion(void)
{
    //Returns the index of the companion with rule t
    for (size_t i = 0; i < entities.size(); i++)
    {
        if(entities[i].rule==6 || entities[i].rule==7)
        {
            return i;
        }
    }

    return -1;
}

int entityclass::getplayer(void)
{
    //Returns the index of the first player entity
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == EntityType_PLAYER)
        {
            return i;
        }
    }

    return -1;
}

int entityclass::getscm(void)
{
    //Returns the supercrewmate
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == EntityType_SUPERCREWMATE)
        {
            return i;
        }
    }

    return 0;
}

int entityclass::getlineat( int t )
{
    //Get the entity which is a horizontal line at height t (for SWN game)
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i].size == 5)
        {
            if (entities[i].yp == t)
            {
                return i;
            }
        }
    }

    return 0;
}

int entityclass::getcrewman(int t)
{
    // Returns the index of the crewman with colour index given by t.
    // Note: this takes an int, not an EntityColour, as invalid colours are allowed in scripting

    for (size_t i = 0; i < entities.size(); i++)
    {
        if ((entities[i].type == EntityType_CREWMATE || entities[i].type == EntityType_SUPERCREWMATE)
            && (entities[i].rule == 6 || entities[i].rule == 7))
        {
            if (entities[i].colour == t)
            {
                return i;
            }
        }
    }

    // Return entity 0 as a fallback
    // Unfortunately some levels rely on this, where targeting a non-existent crewman returns the first entity...
    // Which, most of the time, is the player.

    return 0;
}

int entityclass::getcustomcrewman(int t)
{
    // like getcrewman, this returns the index of the CUSTOM crewman with colour index given by t

    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == EntityType_COLLECTABLE_CREWMATE)
        {
            if (entities[i].colour == t)
            {
                return i;
            }
        }
    }

    return 0;
}

int entityclass::getteleporter(void)
{
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i].type == EntityType_TELEPORTER)
        {
            return i;
        }
    }

    return -1;
}

bool entityclass::entitycollide( int a, int b )
{
    if (!INBOUNDS_VEC(a, entities) || !INBOUNDS_VEC(b, entities))
    {
        vlog_error("entitycollide() out-of-bounds!");
        return false;
    }

    //Do entities a and b collide?
    SDL_Rect temprect;
    temprect.x = entities[a].xp + entities[a].cx;
    temprect.y = entities[a].yp + entities[a].cy;
    temprect.w = entities[a].w;
    temprect.h = entities[a].h;

    SDL_Rect temprect2;
    temprect2.x = entities[b].xp + entities[b].cx;
    temprect2.y = entities[b].yp + entities[b].cy;
    temprect2.w = entities[b].w;
    temprect2.h = entities[b].h;

    if (help.intersects(temprect, temprect2)) return true;
    return false;
}

bool entityclass::checkdamage(bool scm /*= false*/)
{
    //Returns true if player (or supercrewmate) collides with a damagepoint
    for(size_t i=0; i < entities.size(); i++)
    {
        if((scm && entities[i].type == EntityType_SUPERCREWMATE) || (!scm && entities[i].rule == 0))
        {
            SDL_Rect temprect;
            temprect.x = entities[i].xp + entities[i].cx;
            temprect.y = entities[i].yp + entities[i].cy;
            temprect.w = entities[i].w;
            temprect.h = entities[i].h;

            for (size_t j=0; j<blocks.size(); j++)
            {
                if (blocks[j].type == DAMAGE && help.intersects(blocks[j].rect, temprect))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

int entityclass::checktrigger(int* block_idx)
{
    //Returns an int player entity (rule 0) collides with a trigger
    //Also returns the index of the block
    *block_idx = -1;
    for(size_t i=0; i < entities.size(); i++)
    {
        if(entities[i].rule==0)
        {
            SDL_Rect temprect;
            temprect.x = entities[i].xp + entities[i].cx;
            temprect.y = entities[i].yp + entities[i].cy;
            temprect.w = entities[i].w;
            temprect.h = entities[i].h;

            for (size_t j=0; j<blocks.size(); j++)
            {
                if (blocks[j].type == TRIGGER && help.intersects(blocks[j].rect, temprect))
                {
                    *block_idx = j;
                    return blocks[j].trigger;
                }
            }
        }
    }
    return -1;
}

int entityclass::checkactivity(void)
{
    //Returns an int player entity (rule 0) collides with an activity
    for(size_t i=0; i < entities.size(); i++)
    {
        if(entities[i].rule==0)
        {
            SDL_Rect temprect;
            temprect.x = entities[i].xp + entities[i].cx;
            temprect.y = entities[i].yp + entities[i].cy;
            temprect.w = entities[i].w;
            temprect.h = entities[i].h;

            for (size_t j=0; j<blocks.size(); j++)
            {
                if (blocks[j].type == ACTIVITY && help.intersects(blocks[j].rect, temprect))
                {
                    return j;
                }
            }
        }
    }
    return -1;
}

bool entityclass::checkplatform(const SDL_Rect& temprect, int* px, int* py)
{
    //Return true if rectset intersects a moving platform, setups px & py to the platform x & y
    for (size_t i = 0; i < blocks.size(); i++)
    {
        if (blocks[i].type == BLOCK && help.intersects(blocks[i].rect, temprect))
        {
            *px = blocks[i].xp;
            *py = blocks[i].yp;
            return true;
        }
    }
    return false;
}

bool entityclass::checkblocks(const SDL_Rect& temprect, const float dx, const float dy, const int dr, const bool skipdirblocks)
{
    for (size_t i = 0; i < blocks.size(); i++)
    {
        if(!skipdirblocks && blocks[i].type == DIRECTIONAL)
        {
            if (dy > 0 && blocks[i].trigger == 0) if (help.intersects(blocks[i].rect, temprect)) return true;
            if (dy <= 0 && blocks[i].trigger == 1) if (help.intersects(blocks[i].rect, temprect)) return true;
            if (dx > 0 && blocks[i].trigger == 2) if (help.intersects(blocks[i].rect, temprect)) return true;
            if (dx <= 0 && blocks[i].trigger == 3) if (help.intersects(blocks[i].rect, temprect)) return true;
        }
        if (blocks[i].type == BLOCK && help.intersects(blocks[i].rect, temprect))
        {
            return true;
        }
        if (blocks[i].type == SAFE && (dr)==1 && help.intersects(blocks[i].rect, temprect))
        {
            return true;
        }
    }
    return false;
}

bool entityclass::checkwall(const bool invincible, const SDL_Rect& temprect, const float dx, const float dy, const int dr, const bool skipblocks, const bool skipdirblocks)
{
    //Returns true if entity setup in temprect collides with a wall
    if(skipblocks)
    {
        if (checkblocks(temprect, dx, dy, dr, skipdirblocks)) return true;
    }

    int tempx = getgridpoint(temprect.x);
    int tempy = getgridpoint(temprect.y);
    int tempw = getgridpoint(temprect.x + temprect.w - 1);
    int temph = getgridpoint(temprect.y + temprect.h - 1);
    if (map.collide(tempx, tempy, invincible)) return true;
    if (map.collide(tempw, tempy, invincible)) return true;
    if (map.collide(tempx, temph, invincible)) return true;
    if (map.collide(tempw, temph, invincible)) return true;
    if (temprect.h >= 12)
    {
        int tpy1 = getgridpoint(temprect.y + 6);
        if (map.collide(tempx, tpy1, invincible)) return true;
        if (map.collide(tempw, tpy1, invincible)) return true;
        if (temprect.h >= 18)
        {
            tpy1 = getgridpoint(temprect.y + 12);
            if (map.collide(tempx, tpy1, invincible)) return true;
            if (map.collide(tempw, tpy1, invincible)) return true;
            if (temprect.h >= 24)
            {
                tpy1 = getgridpoint(temprect.y + 18);
                if (map.collide(tempx, tpy1, invincible)) return true;
                if (map.collide(tempw, tpy1, invincible)) return true;
            }
        }
    }
    if (temprect.w >= 12)
    {
        int tpx1 = getgridpoint(temprect.x + 6);
        if (map.collide(tpx1, tempy, invincible)) return true;
        if (map.collide(tpx1, temph, invincible)) return true;
    }
    return false;
}

bool entityclass::checkwall(const bool invincible, const SDL_Rect& temprect)
{
    // Same as above but use default arguments for blocks
    return checkwall(invincible, temprect, 0, 0, 0, true, false);
}

float entityclass::hplatformat(const int px, const int py)
{
    //Returns first entity of horizontal platform at (px, py), -1000 otherwise.
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i].rule == 2 && entities[i].behave >= 2
        && entities[i].xp == px && entities[i].yp == py)
        {
            if (entities[i].behave == 8)   //threadmill!
            {
                return entities[i].para;
            }
            else if(entities[i].behave == 9)    //threadmill!
            {
                return -entities[i].para;
            }
            else
            {
                return entities[i].vx;
            }
        }
    }
    return -1000;
}

static int yline( int a, int b )
{
    if (a < b) return -1;
    return 1;
}

bool entityclass::entityhlinecollide( int t, int l )
{
    if (!INBOUNDS_VEC(t, entities) || !INBOUNDS_VEC(l, entities))
    {
        vlog_error("entityhlinecollide() out-of-bounds!");
        return false;
    }

    //Returns true is entity t collided with the horizontal line l.
    if(entities[t].xp + entities[t].cx+entities[t].w>=entities[l].xp)
    {
        if(entities[t].xp + entities[t].cx<=entities[l].xp+entities[l].w)
        {
            int linetemp = 0;

            linetemp += yline(entities[t].yp, entities[l].yp);
            linetemp += yline(entities[t].yp + entities[t].h, entities[l].yp);
            linetemp += yline(entities[t].oldyp, entities[l].yp);
            linetemp += yline(entities[t].oldyp + entities[t].h, entities[l].yp);

            if (linetemp > -4 && linetemp < 4) return true;
            return false;
        }
    }
    return false;
}

bool entityclass::entityvlinecollide( int t, int l )
{
    if (!INBOUNDS_VEC(t, entities) || !INBOUNDS_VEC(l, entities))
    {
        vlog_error("entityvlinecollide() out-of-bounds!");
        return false;
    }

    //Returns true is entity t collided with the vertical line l.
    if(entities[t].yp + entities[t].cy+entities[t].h>=entities[l].yp
    && entities[t].yp + entities[t].cy<=entities[l].yp+entities[l].h)
    {
        int linetemp = 0;

        linetemp += yline(entities[t].xp + entities[t].cx+1, entities[l].xp);
        linetemp += yline(entities[t].xp + entities[t].cx+1 + entities[t].w, entities[l].xp);
        linetemp += yline(entities[t].oldxp + entities[t].cx+1, entities[l].xp);
        linetemp += yline(entities[t].oldxp + entities[t].cx+1 + entities[t].w, entities[l].xp);

        if (linetemp > -4 && linetemp < 4) return true;
        return false;
    }
    return false;
}

bool entityclass::entitywarphlinecollide(int t, int l) {
    if (!INBOUNDS_VEC(t, entities) || !INBOUNDS_VEC(l, entities))
    {
        vlog_error("entitywarphlinecollide() out-of-bounds!");
        return false;
    }

    //Returns true is entity t collided with the horizontal line l.
    if(entities[t].xp + entities[t].cx+entities[t].w>=entities[l].xp
    &&entities[t].xp + entities[t].cx<=entities[l].xp+entities[l].w){
        int linetemp = 0;
        if (entities[l].yp < 120) {
            //Top line
            if (entities[t].vy < 0) {
                if (entities[t].yp < entities[l].yp + 10) linetemp++;
                if (entities[t].yp + entities[t].h < entities[l].yp + 10) linetemp++;
                if (entities[t].oldyp < entities[l].yp + 10) linetemp++;
                if (entities[t].oldyp + entities[t].h < entities[l].yp + 10) linetemp++;
            }

            if (linetemp > 0) return true;
            return false;
        }else {
            //Bottom line
            if (entities[t].vy > 0) {
                if (entities[t].yp > entities[l].yp - 10) linetemp++;
                if (entities[t].yp + entities[t].h > entities[l].yp - 10) linetemp++;
                if (entities[t].oldyp > entities[l].yp - 10) linetemp++;
                if (entities[t].oldyp + entities[t].h > entities[l].yp - 10) linetemp++;
            }

            if (linetemp > 0) return true;
            return false;
        }
    }
    return false;
}

bool entityclass::entitywarpvlinecollide(int t, int l) {
    if (!INBOUNDS_VEC(t, entities) || !INBOUNDS_VEC(l, entities))
    {
        vlog_error("entitywarpvlinecollide() out-of-bounds!");
        return false;
    }

    //Returns true is entity t collided with the vertical warp line l.
    if(entities[t].yp + entities[t].cy+entities[t].h>=entities[l].yp
    && entities[t].yp + entities[t].cy <= entities[l].yp + entities[l].h) {
        int linetemp = 0;
        if (entities[l].xp < 160) {
            //Left hand line
            if (entities[t].xp + entities[t].cx + 1 < entities[l].xp + 10) linetemp++;
            if (entities[t].xp + entities[t].cx+1 + entities[t].w < entities[l].xp + 10) linetemp++;
            if (entities[t].oldxp + entities[t].cx + 1 < entities[l].xp + 10) linetemp++;
            if (entities[t].oldxp + entities[t].cx + 1 + entities[t].w < entities[l].xp + 10) linetemp++;

            if (linetemp > 0) return true;
            return false;
        }else {
            //Right hand line
            if (entities[t].xp + entities[t].cx + 1 > entities[l].xp - 10) linetemp++;
            if (entities[t].xp + entities[t].cx+1 + entities[t].w > entities[l].xp - 10) linetemp++;
            if (entities[t].oldxp + entities[t].cx + 1 > entities[l].xp - 10) linetemp++;
            if (entities[t].oldxp + entities[t].cx + 1 + entities[t].w > entities[l].xp - 10) linetemp++;

            if (linetemp > 0) return true;
            return false;
        }
    }
    return false;
}

float entityclass::entitycollideplatformroof( int t )
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("entitycollideplatformroof() out-of-bounds!");
        return -1000;
    }

    SDL_Rect temprect;
    temprect.x = entities[t].xp + entities[t].cx;
    temprect.y = entities[t].yp + entities[t].cy -1;
    temprect.w = entities[t].w;
    temprect.h = entities[t].h;

    int px = 0, py = 0;
    if (checkplatform(temprect, &px, &py))
    {
        //px and py now contain an x y coordinate for a platform, find it
        return hplatformat(px, py);
    }
    return -1000;
}

float entityclass::entitycollideplatformfloor( int t )
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("entitycollideplatformfloor() out-of-bounds!");
        return -1000;
    }

    SDL_Rect temprect;
    temprect.x = entities[t].xp + entities[t].cx;
    temprect.y = entities[t].yp + entities[t].cy + 1;
    temprect.w = entities[t].w;
    temprect.h = entities[t].h;

    int px = 0, py = 0;
    if (checkplatform(temprect, &px, &py))
    {
        //px and py now contain an x y coordinate for a platform, find it
        return hplatformat(px, py);
    }
    return -1000;
}

bool entityclass::entitycollidefloor( int t )
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("entitycollidefloor() out-of-bounds!");
        return false;
    }

    SDL_Rect temprect;
    temprect.x = entities[t].xp + entities[t].cx;
    temprect.y = entities[t].yp + entities[t].cy + 1;
    temprect.w = entities[t].w;
    temprect.h = entities[t].h;

    const bool invincible = map.invincibility && entities[t].ishumanoid();

    if (checkwall(invincible, temprect)) return true;
    return false;
}

bool entityclass::entitycollideroof( int t )
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("entitycollideroof() out-of-bounds!");
        return false;
    }

    SDL_Rect temprect;
    temprect.x = entities[t].xp + entities[t].cx;
    temprect.y = entities[t].yp + entities[t].cy - 1;
    temprect.w = entities[t].w;
    temprect.h = entities[t].h;

    const bool invincible = map.invincibility && entities[t].ishumanoid();

    if (checkwall(invincible, temprect)) return true;
    return false;
}

bool entityclass::testwallsx( int t, int tx, int ty, const bool skipdirblocks )
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("testwallsx() out-of-bounds!");
        return false;
    }

    SDL_Rect temprect;
    temprect.x = tx + entities[t].cx;
    temprect.y = ty + entities[t].cy;
    temprect.w = entities[t].w;
    temprect.h = entities[t].h;

    bool skipblocks = entities[t].rule < 2 || entities[t].type == EntityType_SUPERCREWMATE;
    float dx = 0;
    float dy = 0;
    if (entities[t].rule == 0) dx = entities[t].vx;
    int dr = entities[t].rule;

    const bool invincible = map.invincibility && entities[t].ishumanoid();

    //Ok, now we check walls
    if (checkwall(invincible, temprect, dx, dy, dr, skipblocks, skipdirblocks))
    {
        if (entities[t].vx > 1.0f)
        {
            entities[t].vx--;
            entities[t].newxp = entities[t].xp + entities[t].vx;
            return testwallsx(t, entities[t].newxp, entities[t].yp, skipdirblocks);
        }
        else if (entities[t].vx < -1.0f)
        {
            entities[t].vx++;
            entities[t].newxp = entities[t].xp + entities[t].vx;
            return testwallsx(t, entities[t].newxp, entities[t].yp, skipdirblocks);
        }
        else
        {
            entities[t].vx=0;
            return false;
        }
    }
    return true;
}

bool entityclass::testwallsy( int t, int tx, int ty )
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("testwallsy() out-of-bounds!");
        return false;
    }

    SDL_Rect temprect;
    temprect.x = tx + entities[t].cx;
    temprect.y = ty + entities[t].cy;
    temprect.w = entities[t].w;
    temprect.h = entities[t].h;

    bool skipblocks = entities[t].rule < 2 || entities[t].type == EntityType_SUPERCREWMATE;

    float dx = 0;
    float dy = 0;
    if (entities[t].rule == 0) dy = entities[t].vy;
    int dr = entities[t].rule;

    const bool invincible = map.invincibility && entities[t].ishumanoid();

    //Ok, now we check walls
    if (checkwall(invincible, temprect, dx, dy, dr, skipblocks, false))
    {
        if (entities[t].vy > 1)
        {
            entities[t].vy--;
            entities[t].newyp = int(entities[t].yp + entities[t].vy);
            return testwallsy(t, entities[t].xp, entities[t].newyp);
        }
        else if (entities[t].vy < -1)
        {
            entities[t].vy++;
            entities[t].newyp = int(entities[t].yp + entities[t].vy);
            return testwallsy(t, entities[t].xp, entities[t].newyp);
        }
        else
        {
            entities[t].vy=0;
            return false;
        }
    }
    return true;
}

void entityclass::applyfriction( int t, float xrate, float yrate )
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("applyfriction() out-of-bounds!");
        return;
    }

    if (entities[t].vx > 0.00f) entities[t].vx -= xrate;
    if (entities[t].vx < 0.00f) entities[t].vx += xrate;
    if (entities[t].vy > 0.00f) entities[t].vy -= yrate;
    if (entities[t].vy < 0.00f) entities[t].vy += yrate;
    if (entities[t].vy > 10.00f) entities[t].vy = 10.0f;
    if (entities[t].vy < -10.00f) entities[t].vy = -10.0f;
    if (entities[t].vx > 6.00f) entities[t].vx = 6.0f;
    if (entities[t].vx < -6.00f) entities[t].vx = -6.0f;

    if (SDL_fabsf(entities[t].vx) < xrate) entities[t].vx = 0.0f;
    if (SDL_fabsf(entities[t].vy) < yrate) entities[t].vy = 0.0f;
}

void entityclass::updateentitylogic( int t )
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("updateentitylogic() out-of-bounds!");
        return;
    }

    entities[t].oldxp = entities[t].xp;
    entities[t].oldyp = entities[t].yp;

    entities[t].vx = entities[t].vx + entities[t].ax;
    entities[t].vy = entities[t].vy + entities[t].ay;
    entities[t].ax = 0;

    if (entities[t].gravity)
    {
        if (entities[t].rule == 0)
        {
            if(game.gravitycontrol==0)
            {
                entities[t].ay = 3;
            }
            else
            {
                entities[t].ay = -3;
            }
        }
        else if (entities[t].rule == 7)
        {
            entities[t].ay = -3;
        }
        else
        {
            entities[t].ay = 3;
        }
        applyfriction(t, game.inertia, 0.25f);
    }

    entities[t].newxp = entities[t].xp + entities[t].vx;
    entities[t].newyp = entities[t].yp + entities[t].vy;
}

void entityclass::entitymapcollision( int t )
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("entitymapcollision() out-of-bounds!");
        return;
    }

    if (testwallsx(t, entities[t].newxp, entities[t].yp, false))
    {
        entities[t].xp = entities[t].newxp;
    }
    else
    {
        if (entities[t].onwall > 0) entities[t].state = entities[t].onwall;
        if (entities[t].onxwall > 0) entities[t].state = entities[t].onxwall;
    }
    if (testwallsy(t, entities[t].xp, entities[t].newyp))
    {
        entities[t].yp = entities[t].newyp;
    }
    else
    {
        if (entities[t].onwall > 0) entities[t].state = entities[t].onwall;
        if (entities[t].onywall > 0) entities[t].state = entities[t].onywall;
    }
}

void entityclass::movingplatformfix( int t, int j )
{
    if (!INBOUNDS_VEC(t, entities) || !INBOUNDS_VEC(j, entities))
    {
        vlog_error("movingplatformfix() out-of-bounds!");
        return;
    }

    //If this intersects the entity, then we move them along it
    if (entitycollide(t, j))
    {
        //ok, bollox, let's make sure
        entities[j].yp = entities[j].yp + int(entities[j].vy);
        if (entitycollide(t, j))
        {
            entities[j].yp = entities[j].yp - int(entities[j].vy);
            entities[j].vy = entities[t].vy;
            entities[j].newyp = entities[j].yp + int(entities[j].vy);
            if (testwallsy(j, entities[j].xp, entities[j].newyp))
            {
                if (entities[t].vy > 0)
                {
                    entities[j].yp = entities[t].yp + entities[t].h;
                    entities[j].vy = 0;
                    entities[j].onroof = 2;
                    entities[j].visualonroof = 1;
                }
                else
                {
                    entities[j].yp = entities[t].yp - entities[j].h-entities[j].cy;
                    entities[j].vy = 0;
                    entities[j].onground = 2;
                    entities[j].visualonground = 1;
                }
            }
            else
            {
                entities[t].state = entities[t].onwall;
            }
        }
    }
}

void entityclass::customwarplinecheck(int i) {
    if (!INBOUNDS_VEC(i, entities))
    {
        vlog_error("customwarplinecheck() out-of-bounds!");
        return;
    }

    //Turns on obj.customwarpmodevon and obj.customwarpmodehon if player collides
    //with warp lines

    //We test entity to entity
    for (int j = 0; j < (int) entities.size(); j++) {
        if (i != j) {
            if (entities[i].rule == 0 && entities[j].rule == 5 //Player vs vertical line!
            && (entities[j].type == EntityType_WARP_LINE_LEFT || entities[j].type == EntityType_WARP_LINE_RIGHT)
            && entitywarpvlinecollide(i, j)) {
                customwarpmodevon = true;
            }

            if (entities[i].rule == 0 && entities[j].rule == 7   //Player vs horizontal WARP line
            && (entities[j].type == EntityType_WARP_LINE_TOP || entities[j].type == EntityType_WARP_LINE_BOTTOM)
            && entitywarphlinecollide(i, j)) {
                customwarpmodehon = true;
            }
        }
    }
}

void entityclass::entitycollisioncheck(void)
{
    for (size_t i = 0; i < entities.size(); i++)
    {
        bool player = entities[i].rule == 0;
        bool scm = game.supercrewmate && entities[i].type == EntityType_SUPERCREWMATE;
        if (!player && !scm)
        {
            continue;
        }

        //We test entity to entity
        for (size_t j = 0; j < entities.size(); j++)
        {
            if (i == j)
            {
                continue;
            }

            collisioncheck(i, j, scm);
        }
    }

    //can't have the player being stuck...
    stuckprevention(getplayer());

    //Can't have the supercrewmate getting stuck either!
    if (game.supercrewmate)
    {
        stuckprevention(getscm());
    }

    //Is the player colliding with any damageblocks?
    if (checkdamage() && !map.invincibility)
    {
        //usual player dead stuff
        game.deathseq = 30;
    }

    //how about the supercrewmate?
    if (game.supercrewmate)
    {
        if (checkdamage(true) && !map.invincibility)
        {
            //usual player dead stuff
            game.scmhurt = true;
            game.deathseq = 30;
        }
    }

    // WARNING: If updating this code, don't forget to update Map.cpp mapclass::twoframedelayfix()
    int block_idx = -1;
    int activetrigger = checktrigger(&block_idx);
    if (activetrigger > -1 && INBOUNDS_VEC(block_idx, blocks))
    {
        // Load the block's script if its gamestate is out of range
        if (blocks[block_idx].script != "" && (activetrigger < 300 || activetrigger > 336))
        {
            game.startscript = true;
            game.newscript = blocks[block_idx].script;
            removetrigger(activetrigger);
            game.setstate(0);
        }
        else
        {
            game.setstate(activetrigger);
        }
        game.setstatedelay(0);
    }
}

void entityclass::collisioncheck(int i, int j, bool scm /*= false*/)
{
    if (!INBOUNDS_VEC(i, entities) || !INBOUNDS_VEC(j, entities))
    {
        vlog_error("collisioncheck() out-of-bounds!");
        return;
    }

    switch (entities[j].rule)
    {
    case 1:
        if (!entities[j].harmful)
        {
            break;
        }

        //person i hits enemy or enemy bullet j
        if (entitycollide(i, j) && !map.invincibility)
        {
            if (entities[i].size == 0 && (entities[j].size == 0 || entities[j].size == 12))
            {
                //They're both sprites, so do a per pixel collision
                SDL_Point colpoint1;
                colpoint1.x = entities[i].xp;
                colpoint1.y = entities[i].yp;
                SDL_Point colpoint2;
                colpoint2.x = entities[j].xp;
                colpoint2.y = entities[j].yp;
                int drawframe1 = entities[i].collisiondrawframe;
                int drawframe2 = entities[j].drawframe;

                std::vector<SDL_Surface*>& spritesvec = graphics.flipmode ? graphics.flipsprites_surf : graphics.sprites_surf;
                if (INBOUNDS_VEC(drawframe1, spritesvec) && INBOUNDS_VEC(drawframe2, spritesvec)
                && graphics.Hitest(spritesvec[drawframe1],
                                 colpoint1, spritesvec[drawframe2], colpoint2))
                {
                    //Do the collision stuff
                    game.deathseq = 30;
                    game.scmhurt = scm;
                }
            }
            else
            {
                //Ok, then we just assume a normal bounding box collision
                game.deathseq = 30;
                game.scmhurt = scm;
            }
        }
        break;
    case 2:   //Moving platforms
        if (entities[j].behave >= 8 && entities[j].behave < 10)
        {
            //We don't want conveyors, moving platforms only
            break;
        }
        if (entitycollide(i, j))
        {
            //Disable collision temporarily so we don't push the person out!
            //Collision will be restored at end of platform update loop in gamelogic
            disableblockat(entities[j].xp, entities[j].yp);
        }
        break;
    case 3:   //Entity to entity
        if(entities[j].onentity>0)
        {
            if (entitycollide(i, j)) entities[j].state = entities[j].onentity;
        }
        break;
    case 4:   //Person vs horizontal line!
        if(game.deathseq==-1)
        {
            //Here we compare the person's old position versus his new one versus the line.
            //All points either be above or below it. Otherwise, there was a collision this frame.
            if (entities[j].onentity > 0)
            {
                if (entityhlinecollide(i, j))
                {
                    music.playef(Sound_GRAVITYLINE);
                    game.gravitycontrol = (game.gravitycontrol + 1) % 2;
                    game.totalflips++;
                    if (game.gravitycontrol == 0)
                    {
                        if (entities[i].vy < 1) entities[i].vy = 1;
                    }
                    else
                    {
                        if (entities[i].vy > -1) entities[i].vy = -1;
                    }

                    entities[j].state = entities[j].onentity;
                    entities[j].life = 6;
                }
            }
        }
        break;
    case 5:   //Person vs vertical gravity/warp line!
        if(game.deathseq==-1)
        {
            if(entities[j].onentity>0)
            {
                if (entityvlinecollide(i, j))
                {
                    entities[j].state = entities[j].onentity;
                    entities[j].life = 4;
                }
            }
        }
        break;
    case 6:   //Person versus crumbly blocks! Special case
        if (entities[j].onentity > 0)
        {
            //ok; only check the actual collision if they're in a close proximity
            int temp = entities[i].yp - entities[j].yp;
            if (temp > -30 && temp < 30)
            {
                temp = entities[i].xp - entities[j].xp;
                if (temp > -30 && temp < 30)
                {
                    if (entitycollide(i, j)) entities[j].state = entities[j].onentity;
                }
            }
        }
        break;
    case 7: // Person versus horizontal warp line, pre-2.1
        if (GlitchrunnerMode_less_than_or_equal(Glitchrunner2_0)
        && game.deathseq == -1
        && entities[j].onentity > 0
        && entityhlinecollide(i, j))
        {
            entities[j].state = entities[j].onentity;
        }
        break;
    }
}

void entityclass::stuckprevention(int t)
{
    if (!INBOUNDS_VEC(t, entities))
    {
        vlog_error("stuckprevention() out-of-bounds!");
        return;
    }

    // Can't have this entity (player or supercrewmate) being stuck...
    if (!testwallsx(t, entities[t].xp, entities[t].yp, true))
    {
        // Let's try to get out...
        if (game.gravitycontrol == 0)
        {
            entities[t].yp -= 3;
        }
        else
        {
            entities[t].yp += 3;
        }
    }
}
