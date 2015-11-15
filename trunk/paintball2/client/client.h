/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// client.h -- primary header for client

//define	PARANOID			// speed sapping error checking
#ifndef CLIENT_H
#define CLIENT_H

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ref.h"

#include "vid.h"
#include "screen.h"
#include "sound.h"
#include "input.h"
#include "keys.h"
#include "console.h"
#include "cdaudio.h"

// ===
// jitmultithreading
#ifdef WIN32 
#include "../win32/pthread.h"
#include "../win32/semaphore.h"
#include "../win32/sched.h"
#else
#include <pthread.h>
#include <sched.h>
#endif
// ===


// Remove OGG on linux for bug testing. You can enable it in the makefile.
#ifdef WIN32
#define OGG_SUPPORT
#endif

//=============================================================================

typedef struct
{
	qboolean		valid;			// cleared if delta parsing was invalid
	int				serverframe;
	int				servertime;		// server time the message is valid for (in msec)
	int				deltaframe;
	byte			areabits[MAX_MAP_AREAS/8];		// portalarea visibility bits
	player_state_t	playerstate;
	int				num_entities;
	int				parse_entities;	// non-masked index into cl_parse_entities array
} frame_t;

typedef struct
{
	entity_state_t	baseline;		// delta from this if not from a previous frame
	entity_state_t	current;
	entity_state_t	prev;			// will always be valid, but might just be a copy of current

	int			serverframe;		// if not current, this ent isn't in the frame

	int			trailcount;			// for diminishing grenade trails
	vec3_t		lerp_origin;		// for trails (variable hz)

	int			fly_stoptime;
} centity_t;

#define MAX_CLIENTWEAPONMODELS		20		// PGM -- upped from 16 to fit the chainfist vwep

typedef struct
{
	char	name[MAX_QPATH];
	char	cinfo[MAX_QPATH];
	//struct image_s	*skin;
	struct image_s	*skins[MAX_MESHSKINS]; // jitskm
	struct image_s	*icon;
	char	iconname[MAX_QPATH];
	struct model_s	*model;
	struct model_s	*weaponmodel[MAX_CLIENTWEAPONMODELS];
} clientinfo_t;

extern char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
extern int num_cl_weaponmodels;

#define	CMD_BACKUP		64	// allow a lot of command backups for very fast systems

//
// the client_state_t structure is wiped completely at every
// server map change
//
typedef struct
{
	int			timeoutcount;

	int			timedemo_frames;
	int			timedemo_start;

	qboolean	refresh_prepped;		// false if on new level or new ref dll
	qboolean	sound_prepped;			// ambient sounds can start
	qboolean	force_refdef;			// vid has changed, so we can't use a paused refdef

	int			parse_entities;			// index (not anded off) into cl_parse_entities[]

	usercmd_t	cmd;
	usercmd_t	cmds[CMD_BACKUP];		// each mesage will send several old cmds
	int			cmd_time[CMD_BACKUP];	// time sent, for calculating pings
	short		predicted_origins[CMD_BACKUP][3];	// for debug comparing against server

	float		predicted_step;			// for stair up smoothing
	float		predicted_step_unsent;	// jitmove / jitnetfps - predicted step for movement between cmd packets.

	vec3_t		predicted_origin;		// generated by CL_PredictMovement
	vec3_t		predicted_angles;
	vec3_t		prediction_error;

	frame_t		frame;					// received from server
	int			surpressCount;			// number of messages rate supressed
	frame_t		frames[UPDATE_BACKUP];

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t		viewangles;

	int			time;			// this is the time value that the client
								// is rendering at.  always <= cls.realtime
	float		frametime;		// jitnetfps -- time it took the client to render a frame
	float		lerpfrac;		// between oldframe and frame

	refdef_t	refdef;

	vec3_t		v_forward, v_right, v_up;	// set when refdef.angles is set

	//
	// transient data from server
	//
	char		layout[1024];		// general 2D overlay
	int			inventory[MAX_ITEMS];

	//
	// non-gameserver infornamtion
	// FIXME: move this cinematic stuff into the cin_t structure
	FILE		*cinematic_file;
	int			cinematictime;		// cls.realtime for first cinematic frame
	int			cinematicframe;
	char		cinematicpalette[768];
	qboolean	cinematicpalette_active;

	//
	// server state information
	//
	qboolean	attractloop;		// running the attract loop, any key will menu
	int			servercount;	// server identification for prespawns
	char		gamedir[MAX_QPATH];
	int			playernum;
	qboolean	playernum_demooverride; // jitdemo

	char		configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];

	//
	// locally derived information from server state
	//
	struct model_s	*model_draw[MAX_MODELS];
	struct cmodel_s	*model_clip[MAX_MODELS];

	struct sfx_s	*sound_precache[MAX_SOUNDS];
	struct image_s	*image_precache[MAX_IMAGES];

	clientinfo_t	clientinfo[MAX_CLIENTS];
	clientinfo_t	baseclientinfo;
} client_state_t;

extern	client_state_t	cl;

/*
==================================================================

the client_static_t structure is persistant through an arbitrary number
of server connections

==================================================================
*/

typedef enum {
	ca_uninitialized,
	ca_disconnected, 	// not talking to a server
	ca_connecting,		// sending request packets to the server
	ca_connected,		// netchan_t established, waiting for svc_serverdata
	ca_active			// game views should be displayed
} connstate_t;

#if 0 // jitdownload -- don't think this is used.
typedef enum {
	dl_none,
	dl_model,
	dl_sound,
	dl_skin,
	dl_single
} dltype_t;		// download type
#endif

typedef enum {
	key_game,
	key_console,
	key_message,
	key_menu
} keydest_t;

typedef struct
{
	connstate_t	state;
	keydest_t	key_dest;

	int			framecount;
	int			realtime;			// always increasing, no clamping, etc
	float		frametime;			// seconds since last frame

// screen rendering information
	float		disable_screen;		// showing loading plaque between levels
									// or changing rendering dlls
	qboolean	loading_screen;		// viciouz - loading screen as opposed to video change
									// if time gets > 30 seconds ahead, break it
	int			disable_servercount;	// when we receive a frame and cl.servercount
									// > cls.disable_servercount, clear disable_screen

// connection information
	char		servername[MAX_OSPATH];	// name of server from original connect
	float		connect_time;		// for connection retransmits

	int			quakePort;			// a 16 bit value that allows quake servers
									// to work around address translating routers
	netchan_t	netchan;
	int			serverProtocol;		// in case we are doing some kind of version hack

	int			challenge;			// from the server to use for connecting

	FILE		*download;			// file transfer from server
	char		downloadtempname[MAX_OSPATH];
	char		downloadname[MAX_OSPATH];
	int			downloadnumber;
#if 0 // I don't think this is used.
	dltype_t	downloadtype;
#endif
	int			downloadpercent;

// demo recording info must be here, so it isn't cleared on level change
	qboolean	demorecording;
	qboolean	demowaiting;	// don't record until a non-delta message is received
	FILE		*demofile;
#ifdef USE_DOWNLOAD3 // jitdownload
	byte		*download3chunks;
	byte		*download3data;
	unsigned	download3size;
	int			download3compression;
	int			download3lastchunkwritten;
	int			download3completechunks;
	byte		download3fileid;
	byte		download3lastfileid;
	float		download3rate; // transfer speed in bytes/second
	int			download3starttime;
	int			download3lastratecheck;
	int			download3bytessincelastratecheck;
	int			download3bytesreceived;
	qboolean	download3requested; // did we actually request the download?
	int			download3backacks[DOWNLOAD3_NUMBACKUPACKS];
	byte		download3currentbackack;
	unsigned	download3md5sum;
	qboolean	download3supported; // does the server support it?
	byte		download3startcmd;
#endif
	unsigned	last_transmit_time; // jitnetfps
	unsigned	server_gamebuild; // jitversion
	unsigned	server_enginebuild; // jitversion
	int			gametype; // jitscores
} client_static_t;

extern client_static_t	cls;

//=============================================================================

//
// cvars
//
extern	cvar_t	*cl_stereo_separation;
extern	cvar_t	*cl_stereo;

extern	cvar_t	*cl_gun;
extern	cvar_t	*cl_add_lights;
extern	cvar_t	*cl_add_particles;
extern	cvar_t	*cl_add_entities;
extern	cvar_t	*cl_predict;
extern	cvar_t	*cl_footsteps;
extern	cvar_t	*cl_locknetfps; // jitnetfps
extern	cvar_t	*cl_cmdrate; // jitnetfps
extern	cvar_t	*cl_noskins;
extern	cvar_t	*cl_autoskins;

extern	cvar_t	*cl_upspeed;
extern	cvar_t	*cl_forwardspeed;
extern	cvar_t	*cl_sidespeed;

extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;

extern	cvar_t	*cl_run;

extern	cvar_t	*cl_anglespeedkey;

extern	cvar_t	*cl_drawfps; // jit
extern	cvar_t	*cl_drawpps; // jitnetfps
extern	cvar_t	*cl_drawtexinfo; // jit
extern	cvar_t	*cl_centerprintkills; // jit
extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_showmiss;
extern	cvar_t	*cl_shownamechange; // jit
extern	cvar_t	*cl_showclamp;
extern  cvar_t  *cl_drawclock; // viciouz - computer time clock
extern  cvar_t  *cl_drawclockx; // T3RR0R15T: clock position
extern  cvar_t  *cl_drawclocky; // T3RR0R15T: clock position
extern  cvar_t  *cl_maptime; // T3RR0R15T: elapsed maptime (from AprQ2)
extern  cvar_t  *cl_maptimex; // T3RR0R15T: maptime position
extern  cvar_t  *cl_maptimey; // T3RR0R15T: maptime position
extern  cvar_t  *cl_drawping; // T3RR0R15T: display ping on HUD
extern  cvar_t  *cl_autorecord; // T3RR0R15T: client side autodemo
extern  cvar_t  *cl_scoreboard_sorting; // T3RR0R15T: scoreboard sorting

extern	cvar_t	*lookspring;
extern	cvar_t	*lookstrafe;
extern	cvar_t	*sensitivity;
extern	cvar_t	*fov; // jit
extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*m_forward;
extern	cvar_t	*m_side;
extern	cvar_t	*m_invert; // jitmouse
extern	cvar_t	*m_doubleclickspeed; // jitmenu
extern	cvar_t	*m_fovscale; // jit

extern	cvar_t	*freelook;

extern	cvar_t	*cl_lightlevel;	// FIXME HACK

extern	cvar_t	*cl_paused;
extern	cvar_t	*cl_timedemo;

extern	cvar_t	*cl_vwep;
extern	cvar_t	*r_oldmodels;
extern	cvar_t	*gl_highres_textures;

#ifdef USE_DOWNLOAD3
extern	cvar_t	*cl_fast_download; // jitdownload
#endif

// Xile/NiceAss LOC
extern cvar_t *cl_drawlocs;
extern cvar_t *loc_here;
extern cvar_t *loc_there;

typedef struct
{
	int		key;				// so entities can reuse same entry
	vec3_t	color;
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
} cdlight_t;

extern	centity_t	cl_entities[MAX_EDICTS];
extern	cdlight_t	cl_dlights[MAX_DLIGHTS];

// the cl_parse_entities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original 
#define	MAX_PARSE_ENTITIES	1024
extern	entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

//=============================================================================

extern	netadr_t	net_from;
extern	sizebuf_t	net_message;

void DrawAltString (int x, int y, char *s);	// toggle high bit
qboolean CL_CheckOrDownloadFile (const char *filename);

void CL_AddNetgraph (void);

//ROGUE
typedef struct cl_sustain
{
	int			id;
	int			type;
	int			endtime;
	int			nextthink;
	int			thinkinterval;
	vec3_t		org;
	vec3_t		dir;
	int			color;
	int			count;
	int			magnitude;
	void		(*think)(struct cl_sustain *self);
} cl_sustain_t;

#define MAX_SUSTAINS		32
void CL_ParticleSteamEffect2(cl_sustain_t *self);

void CL_TeleporterParticles (entity_state_t *ent);
void CL_ParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void CL_ParticleEffect2 (vec3_t org, vec3_t dir, int color, int count);

// RAFAEL
void CL_ParticleEffect3 (vec3_t org, vec3_t dir, int color, int count);


//=================================================

// ========
// PGM
typedef struct particle_s
{
	struct particle_s	*next;

	float		time;

	vec3_t		org;
	vec3_t		vel;
	vec3_t		accel;
	float		color;
	float		colorvel;
	float		alpha;
	float		alphavel;
} cparticle_t;

#define	PARTICLE_GRAVITY	80 // jit, was 40
#define BLASTER_PARTICLE_COLOR		0xe0
// PMM
#define INSTANT_PARTICLE	-10000.0
// PGM
// ========

void CL_ClearEffects (void);
void CL_ClearTEnts (void);
void CL_BlasterTrail (vec3_t start, vec3_t end);
void CL_QuadTrail (vec3_t start, vec3_t end);
void CL_RailTrail (vec3_t start, vec3_t end);
void CL_BubbleTrail (vec3_t start, vec3_t end);
void CL_FlagTrail (vec3_t start, vec3_t end, float color);

// RAFAEL
void CL_IonripperTrail (vec3_t start, vec3_t end);

// ========
// PGM
void CL_BlasterParticles2 (vec3_t org, vec3_t dir, unsigned int color);
void CL_BlasterTrail2 (vec3_t start, vec3_t end);
void CL_DebugTrail (vec3_t start, vec3_t end);
void CL_SmokeTrail (vec3_t start, vec3_t end, int colorStart, int colorRun, int spacing);
void CL_Flashlight (int ent, vec3_t pos);
void CL_ForceWall (vec3_t start, vec3_t end, int color);
void CL_FlameEffects (centity_t *ent, vec3_t origin);
//void CL_GenericParticleEffect (vec3_t org, vec3_t dir, int color, int count, int numcolors, int dirspread, float alphavel);
void CL_BubbleTrail2 (vec3_t start, vec3_t end, int dist);
void CL_Heatbeam (vec3_t start, vec3_t end);
void CL_ParticleSteamEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude);
void CL_TrackerTrail (vec3_t start, vec3_t end, int particleColor);
void CL_Tracker_Explode(vec3_t origin);
void CL_TagTrail (vec3_t start, vec3_t end, float color);
void CL_ColorFlash (vec3_t pos, int ent, int intensity, float r, float g, float b);
void CL_Tracker_Shell(vec3_t origin);
void CL_MonsterPlasma_Shell(vec3_t origin);
void CL_ColorExplosionParticles (vec3_t org, int color, int run);
void CL_ParticleSmokeEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude);
void CL_Widowbeamout (cl_sustain_t *self);
void CL_Nukeblast (cl_sustain_t *self);
void CL_WidowSplash (vec3_t org);
// PGM
// ========

int CL_ParseEntityBits (unsigned *bits);
void CL_ParseDelta (entity_state_t *from, entity_state_t *to, int number, int bits);
void CL_ParseFrame (void);

void CL_ParseTEnt (void);
void CL_ParseConfigString (void);
void CL_ParseMuzzleFlash (void);
void CL_ParseMuzzleFlash2 (void);
void SmokeAndFlash(vec3_t origin);

void CL_SetLightstyle (int i);

void CL_RunParticles (void);
void CL_RunDLights (void);
void CL_RunLightStyles (void);

void CL_AddEntities (void);
void CL_AddDLights (void);
void CL_AddTEnts (void);
void CL_AddLightStyles (void);

//=================================================

void CL_PrepRefresh (void);
void CL_RegisterSounds (void);

void CL_Quit_f (void);

void IN_Accumulate (void);

void CL_ParseLayout (void);


//
// cl_main
//
extern	refexport_t	re;		// interface to refresh .dll

void CL_Init (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_GetChallengePacket (void);
void CL_PingServers_f (void);
void CL_Snd_Restart_f (void);
void CL_RequestNextDownload (void);
void CL_ServerlistPacket (netadr_t net_from, const char *sRandStr, sizebuf_t *net_message); // jit (unused)
void CL_Serverlist2Packet (netadr_t net_from, sizebuf_t *net_message); // jitserverlist

#ifdef USE_DOWNLOAD3
void CL_DownloadFileName (char *dest, int destlen, char *fn);
void CL_StopCurrentDownload (void);
extern cvar_t *qport;
#endif

//
// cl_input
//
typedef struct
{
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame
	int			state;
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (usercmd_t *cmd);

void CL_ClearState (void);

void CL_ReadPackets (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd);

void IN_CenterView (void);

float CL_KeyState (kbutton_t *key);
char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void CL_WriteDemoMessage (void);
void CL_Stop_f (void);
void CL_Record_f (void);

//
// cl_parse.c
//
extern	char *svc_strings[256];

void CL_ParseServerMessage (void);
void CL_LoadClientinfo (clientinfo_t *ci, char *s);
void SHOWNET(char *s);
void CL_ParseClientinfo (int player);
void CL_Download_f (void);
#ifdef USE_DOWNLOAD3
void CL_Download3_f (void); // jitdownload
#endif
void CL_WriteConfig_f (void); // jitconfig
void translate_string (char *out, size_t size, const char *in); // jit

//
// cl_scores.c
//
// ===
// jitscores / jitevents
void CL_Score_f (void);
void CL_Scoreboard_f (void);
void CL_ScoreboardShow_f (void);
void CL_ScoreboardHide_f (void);
void CL_ParseScoreData (const unsigned char *data); 
void CL_ParsePingData (const unsigned char *data);
void cl_scores_refresh (void);
void cl_scores_setping (int client, int ping);
void cl_scores_setstarttime (int client, int time);
void cl_scores_setkills (int client, int kills);
void cl_scores_setdeaths (int client, int deaths);
void cl_scores_setgrabs (int client, int grabs);
void cl_scores_setcaps (int client, int caps);
void cl_scores_setteam (int client, char team);
void cl_scores_setisalive (int client, qboolean alive);
void cl_scores_setisalive_all (qboolean alive);
void cl_scores_sethasflag_all (qboolean hasflag);
void cl_scores_sethasflag (int client, qboolean hasflag);
void cl_scores_setinuse (int client, qboolean inuse);
void cl_scores_setinuse_all (qboolean inuse);
void cl_scores_clear (int client);
int  cl_scores_get_team (int client);
int  cl_scores_get_team_splat (int client);
int  cl_scores_get_isalive (int client);
int  CL_ScoresDemoData (int startindex, unsigned char **sptr);
unsigned char cl_scores_get_team_textcolor (int client);
void init_cl_scores (void);
void shutdown_cl_scores (void);
#define GAMETYPE_NONE	-1
#define GAMETYPE_DM		0
#define GAMETYPE_1FLAG	1
#define GAMETYPE_2FLAG	2
#define GAMETYPE_SIEGE	3
#define GAMETYPE_KOTH	4
#define GAMETYPE_ELIM	5
#define GAMETYPE_PONG	6
#define GAMETYPE_TDM	7
#define MAX_SCOREBOARD_STRING 128

// console.c client only
void Con_DrawDownloadBar (qboolean inConsole);

//
// cl_stats.c
//
void Stats_Init();
void Stats_Shutdown();
void Stats_AddEvent(int type);
void Stats_Query(void);
void Stats_LoadFromFile();
void Stats_WriteToFile();
void Stats_UpdateTime();
void Stats_Clear(void);

#define STATS_KILL 0
#define STATS_DEATH 1
#define STATS_GRAB 2
#define STATS_CAP 3

//
// cl_decode.c
//
int decode_unsigned (const unsigned char *in, unsigned int *out, int max);
void encode_unsigned (unsigned int count, unsigned int *in, unsigned char *out);
void CL_ParsePrintEvent (const char *str);
void CL_DrawEventStrings (void);
int format_string (char *out_str, int max_len, const char *in_str);
#define name_from_index(a) cl.clientinfo[(a)].name
//
// cl_profile.c
//
void CL_ProfileEdit_f (void);
void CL_ProfileEdit2_f (void);
void CL_ProfileAdd_f (void);
void CL_ProfileSelect_f (void);
void CL_WebLoad_f (void);
void CL_InitProfile (void);
void CL_VNInitResponse (netadr_t adr_from, sizebuf_t *ptData);
void CL_VNResponse (netadr_t adr_from, sizebuf_t *ptData);
// jit
// ===

//
// cl_view.c
//
extern	int			gun_frame;
extern	struct model_s	*gun_model;

void V_Init (void);
void V_RenderView( float stereo_separation );
void V_AddEntity (entity_t *ent);
void V_AddParticle (vec3_t org, int color, float alpha);
void V_AddLight (vec3_t org, float intensity, float r, float g, float b);
void V_AddStain (vec3_t org, vec3_t color, float size);
void V_AddLightStyle (int style, float r, float g, float b);

//
// cl_tent.c
//
void CL_RegisterTEntSounds (void);
void CL_RegisterTEntModels (void);
void CL_SmokeAndFlash(vec3_t origin);
#define MAX_STEP_VARIATIONS 4

//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictMove (void);
void CL_CheckPredictionError (void);


//
// cl_fx.c
//
cdlight_t *CL_AllocDlight (int key);
void CL_BigTeleportParticles (vec3_t org);
void CL_RocketTrail (vec3_t start, vec3_t end, centity_t *old);
void CL_DiminishingTrail (vec3_t start, vec3_t end, centity_t *old, int flags);
void CL_FlyEffect (centity_t *ent, vec3_t origin);
void CL_BfgParticles (entity_t *ent);
void CL_AddParticles (void);
void CL_EntityEvent (entity_state_t *ent);
// RAFAEL
void CL_TrapParticles (entity_t *ent);

//
// menu.c
//
void M_Init (void);
qboolean M_Keydown (int key);
qboolean M_Keyup (int key); // jitmenu
void M_CreateTemporaryBackground (void);
void M_Draw (void);
void M_Menu_Main_f (void);
void M_ForceMenuOff (void);
void M_AddToServerList (netadr_t adr, char *info, qboolean pinging);
void M_MouseSet (int mx, int my); // jitmenu
void M_MouseMove (int mx, int my); // jitmenu
void M_RefreshMenu (void); // jitmenu
void M_ReloadMenu (void); // jitmenu
void M_RefreshActiveMenu (void); // jitmenu
void M_RefreshWidget (const char *name, qboolean lock); // jitmenu
qboolean M_MenuActive (void); // jitmenu
void M_PlayMenuSounds (); // jitmenu
void M_PrintDialog (const char *text);
void M_PopMenu (const char *menu_name); // jitmenu

// cl_serverlist.c
void Serverlist_Init (void);
void Serverlist_Shutdown (void);

int strlen_noformat (const char *s); // jitmenu
int strpos_noformat (const char *in_str, int pos); // jitmenu

// cl_scrn.c
void SCR_PrintPopup (const char *s, qboolean behindmenu); // jit
int SCR_WordWrapText (const char *text_in, float width, char *text_out, size_t size_out); // jit

//
// cl_inv.c
//
void CL_ParseInventory (void);
void CL_KeyInventory (int key);
void CL_DrawInventory (void);
void CL_ParsePrintItem (char *s); // jit
void CL_DrawItemPickups (void); // jit

//
// cl_vote.c -- jitvote
//
void CL_ParseMaplistData (const unsigned char *data);
void CL_UpdateMaplistModes (void);
void init_cl_vote (void);
extern char **cl_maplist_info;
extern char **cl_maplist_names;
extern char **cl_maplist_modes;
extern int cl_maplist_count;
extern int cl_maplist_modes_count;


// pb2_xfire.c -- jitxfire
void CL_Xfire (void);


// snd_ogg.c
#ifdef OGG_SUPPORT
extern qboolean ogg_started;
void OGG_Init (void);
void OGG_Shutdown (void);
void OGG_Stream (void);
char **FS_ListFiles (const char *findname, int *numfiles, unsigned musthave, unsigned canthave, qboolean sort);
void FS_FreeList (char **list, int nfiles);
#endif

//
// cl_pred.c
//
void CL_PredictMovement (void);


//
// cl_images.c
//
void CL_ShutdownImages (void);
bordered_pic_data_t *CL_FindBPic (const char *name);


#if id386
void x86_TimerStart( void );
void x86_TimerStop( void );
void x86_TimerInit( unsigned long smallest, unsigned longest );
unsigned long *x86_TimerGetHistogram( void );
#endif

// Xile/NiceAss LOC
void CL_LoadLoc (void);
void CL_LocPlace (void);
void CL_AddViewLocs (void);
void CL_LocDelete (void);
void CL_LocAdd (char *name);
void CL_LocWrite (char *filename);
void CL_LocHelp_f (void);

extern image_t		*i_conback;
extern image_t		*i_net;
extern image_t		*i_pause;
extern image_t		*i_loading;
extern image_t		*i_backtile;
// jitmenu:
extern image_t		*i_slider1l;
extern image_t		*i_slider1r;
extern image_t		*i_slider1lh;
extern image_t		*i_slider1rh;
extern image_t		*i_slider1ls;
extern image_t		*i_slider1rs;
extern image_t		*i_slider1t;
extern image_t		*i_slider1th;
extern image_t		*i_slider1b;
extern image_t		*i_slider1bh;
extern image_t		*i_slider1bs;
extern image_t		*i_checkbox1u;
extern image_t		*i_checkbox1uh;
extern image_t		*i_checkbox1us;
extern image_t		*i_checkbox1c;
extern image_t		*i_checkbox1ch;
extern image_t		*i_checkbox1cs;
extern image_t		*i_field1l;
extern image_t		*i_field1lh;
extern image_t		*i_field1ls;
extern image_t		*i_field1m;
extern image_t		*i_field1mh;
extern image_t		*i_field1ms;
extern image_t		*i_field1r;
extern image_t		*i_field1rh;
extern image_t		*i_field1rs;

extern image_t		*i_cursor;
extern image_t		*i_cursor_text;

extern bordered_pic_data_t	bpdata_popup1;
extern bordered_pic_data_t	bpdata_button1;
extern bordered_pic_data_t	bpdata_button1_hover;
extern bordered_pic_data_t	bpdata_button1_select;

extern float hudscale; // jithudscale
extern float consoleheight; // T3RR0R15T: console height

#endif // CLIENT_H

