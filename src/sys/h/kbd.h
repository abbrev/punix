#ifndef _KBD_H_
#define _KBD_H_

#ifdef TI89
#define KEY_NBR_ROW		 7	/* 10 for 92+/v200, 7 for 89 */
#elif defined(TI92P)
#define KEY_NBR_ROW		 10	/* 10 for 92+/v200, 7 for 89 */
#else
#error TI89 and TI92P are undefined!
#endif

#define KEY_LEFT	 338
#define KEY_UP		 337
#define KEY_RIGHT	 344
#define KEY_DOWN	 340
#define KEY_CATALOG	 278

#define KEY_VOID	 0
#define KEY_ENTER	 13
#define KEY_THETA	 136
#define KEY_SIGN	 173
#define KEY_BACK	 257
#define KEY_STO		 22	/* Original Key code is 258 */
#define KEY_COS		 260
#define KEY_TAN		 261
#define KEY_LN		 262
#define KEY_CLEAR	 263
#define KEY_ESC		 264
#define KEY_APPS	 265
#define KEY_MODE	 266
#define KEY_F1		 268
#define KEY_F2		 269
#define KEY_F3		 270
#define KEY_F4		 271
#define KEY_F5		 272
#define KEY_F6		 273
#define KEY_F7		 274
#define KEY_F8		 275
#define KEY_SIN		 259
#define KEY_INS		 4353
#define KEY_ON		 267
#define KEY_OFF		 4363
#define KEY_OFF2	 16651
#define KEY_QUIT	 4360
#define KEY_SWITCH	 4361
#define KEY_VARLINK	 4141
#define KEY_CHAR	 4139
#define KEY_ENTRY	 4109
#define KEY_RCL		 4354
#define KEY_MATH	 4149
#define KEY_MEM		 4150
#define KEY_ANS		 4372
#define KEY_CUSTOM	 4147
#define KEY_UNITS	 4400
#define KEY_2ND		 0x1000
#define KEY_DIAMOND	 0x2000
#define KEY_SHIFT	 0x4000
#define KEY_HAND	 0x8000
#define KEY_ALPHA	 0x8000
#define KEY_COMPOSE	 KEY_MODE

#define KEY_INFEQUAL	 156
#define KEY_DIFERENT	 157
#define KEY_SUPEQUAL	 158

#define KEY_EE		 149
#define KEY_HOME	 277

#ifdef TI89
#define HELPKEYS_KEY (KEY_DIAMOND|KEY_EE)
#elif defined(TI92P)
#define HELPKEYS_KEY (KEY_DIAMOND|'k')
#endif

#define LINE_FEED 13

#endif
