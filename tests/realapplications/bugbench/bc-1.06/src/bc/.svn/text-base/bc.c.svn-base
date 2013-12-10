/* A Bison parser, made from bc.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	ENDOFLINE	257
# define	AND	258
# define	OR	259
# define	NOT	260
# define	STRING	261
# define	NAME	262
# define	NUMBER	263
# define	ASSIGN_OP	264
# define	REL_OP	265
# define	INCR_DECR	266
# define	Define	267
# define	Break	268
# define	Quit	269
# define	Length	270
# define	Return	271
# define	For	272
# define	If	273
# define	While	274
# define	Sqrt	275
# define	Else	276
# define	Scale	277
# define	Ibase	278
# define	Obase	279
# define	Auto	280
# define	Read	281
# define	Warranty	282
# define	Halt	283
# define	Last	284
# define	Continue	285
# define	Print	286
# define	Limits	287
# define	UNARY_MINUS	288
# define	HistoryVar	289

#line 1 "bc.y"

/* bc.y: The grammar for a POSIX compatable bc processor with some
         extensions to the language. */

/*  This file is part of GNU bc.
    Copyright (C) 1991, 1992, 1993, 1994, 1997 Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License , or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; see the file COPYING.  If not, write to:
      The Free Software Foundation, Inc.
      59 Temple Place, Suite 330
      Boston, MA 02111 USA

    You may contact the author by:
       e-mail:  philnelson@acm.org
      us-mail:  Philip A. Nelson
                Computer Science Department, 9062
                Western Washington University
                Bellingham, WA 98226-9062
       
*************************************************************************/

#include "bcdefs.h"
#include "global.h"
#include "proto.h"

#line 40 "bc.y"
#ifndef YYSTYPE
typedef union {
	char	 *s_value;
	char	  c_value;
	int	  i_value;
	arg_list *a_value;
       } yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		185
#define	YYFLAG		-32768
#define	YYNTBASE	50

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 289 ? yytranslate[x] : 84)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    40,     2,     2,
      43,    44,    38,    36,    47,    37,     2,    39,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    42,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    48,     2,    49,    41,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    45,     2,    46,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     4,     7,     9,    12,    13,    15,    16,
      18,    22,    25,    26,    28,    31,    35,    38,    42,    44,
      47,    49,    51,    53,    55,    57,    59,    61,    63,    66,
      67,    68,    69,    70,    85,    86,    95,    96,    97,   106,
     110,   111,   115,   117,   121,   123,   125,   126,   127,   132,
     133,   146,   147,   149,   150,   154,   158,   160,   164,   169,
     173,   179,   186,   187,   189,   191,   195,   199,   205,   206,
     208,   209,   211,   212,   217,   218,   223,   224,   229,   232,
     236,   240,   244,   248,   252,   256,   260,   263,   265,   267,
     271,   276,   279,   282,   287,   292,   297,   301,   303,   308,
     310,   312,   314,   316,   318,   319,   321
};
static const short yyrhs[] =
{
      -1,    50,    51,     0,    53,     3,     0,    69,     0,     1,
       3,     0,     0,     3,     0,     0,    55,     0,    53,    42,
      55,     0,    53,    42,     0,     0,    55,     0,    54,     3,
       0,    54,     3,    55,     0,    54,    42,     0,    54,    42,
      56,     0,    56,     0,     1,    56,     0,    28,     0,    33,
       0,    78,     0,     7,     0,    14,     0,    31,     0,    15,
       0,    29,     0,    17,    77,     0,     0,     0,     0,     0,
      18,    57,    43,    76,    42,    58,    76,    42,    59,    76,
      44,    60,    52,    56,     0,     0,    19,    43,    78,    44,
      61,    52,    56,    67,     0,     0,     0,    20,    62,    43,
      78,    63,    44,    52,    56,     0,    45,    54,    46,     0,
       0,    32,    64,    65,     0,    66,     0,    66,    47,    65,
       0,     7,     0,    78,     0,     0,     0,    22,    68,    52,
      56,     0,     0,    13,     8,    43,    71,    44,    52,    45,
      83,    72,    70,    54,    46,     0,     0,    73,     0,     0,
      26,    73,     3,     0,    26,    73,    42,     0,     8,     0,
       8,    48,    49,     0,    38,     8,    48,    49,     0,    73,
      47,     8,     0,    73,    47,     8,    48,    49,     0,    73,
      47,    38,     8,    48,    49,     0,     0,    75,     0,    78,
       0,     8,    48,    49,     0,    75,    47,    78,     0,    75,
      47,     8,    48,    49,     0,     0,    78,     0,     0,    78,
       0,     0,    82,    10,    79,    78,     0,     0,    78,     4,
      80,    78,     0,     0,    78,     5,    81,    78,     0,     6,
      78,     0,    78,    11,    78,     0,    78,    36,    78,     0,
      78,    37,    78,     0,    78,    38,    78,     0,    78,    39,
      78,     0,    78,    40,    78,     0,    78,    41,    78,     0,
      37,    78,     0,    82,     0,     9,     0,    43,    78,    44,
       0,     8,    43,    74,    44,     0,    12,    82,     0,    82,
      12,     0,    16,    43,    78,    44,     0,    21,    43,    78,
      44,     0,    23,    43,    78,    44,     0,    27,    43,    44,
       0,     8,     0,     8,    48,    78,    49,     0,    24,     0,
      25,     0,    23,     0,    35,     0,    30,     0,     0,     3,
       0,    83,     3,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   107,   116,   118,   120,   122,   128,   129,   132,   134,
     135,   136,   138,   140,   141,   142,   143,   144,   146,   147,
     150,   152,   154,   163,   170,   180,   191,   193,   195,   197,
     197,   197,   197,   197,   241,   241,   254,   254,   254,   273,
     275,   275,   279,   280,   282,   288,   291,   292,   292,   301,
     301,   321,   323,   325,   327,   329,   332,   334,   336,   338,
     340,   342,   345,   347,   349,   354,   360,   365,   381,   386,
     388,   393,   401,   401,   428,   428,   441,   441,   457,   463,
     491,   496,   501,   506,   511,   516,   521,   526,   535,   551,
     553,   569,   588,   611,   613,   615,   617,   623,   625,   630,
     632,   634,   636,   640,   647,   648,   649
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "ENDOFLINE", "AND", "OR", "NOT", "STRING", 
  "NAME", "NUMBER", "ASSIGN_OP", "REL_OP", "INCR_DECR", "Define", "Break", 
  "Quit", "Length", "Return", "For", "If", "While", "Sqrt", "Else", 
  "Scale", "Ibase", "Obase", "Auto", "Read", "Warranty", "Halt", "Last", 
  "Continue", "Print", "Limits", "UNARY_MINUS", "HistoryVar", "'+'", 
  "'-'", "'*'", "'/'", "'%'", "'^'", "';'", "'('", "')'", "'{'", "'}'", 
  "','", "'['", "']'", "program", "input_item", "opt_newline", 
  "semicolon_list", "statement_list", "statement_or_error", "statement", 
  "@1", "@2", "@3", "@4", "@5", "@6", "@7", "@8", "print_list", 
  "print_element", "opt_else", "@9", "function", "@10", 
  "opt_parameter_list", "opt_auto_define_list", "define_list", 
  "opt_argument_list", "argument_list", "opt_expression", 
  "return_expression", "expression", "@11", "@12", "@13", 
  "named_expression", "required_eol", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    50,    50,    51,    51,    51,    52,    52,    53,    53,
      53,    53,    54,    54,    54,    54,    54,    54,    55,    55,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    57,
      58,    59,    60,    56,    61,    56,    62,    63,    56,    56,
      64,    56,    65,    65,    66,    66,    67,    68,    67,    70,
      69,    71,    71,    72,    72,    72,    73,    73,    73,    73,
      73,    73,    74,    74,    75,    75,    75,    75,    76,    76,
      77,    77,    79,    78,    80,    78,    81,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    82,    82,    82,
      82,    82,    82,    82,    83,    83,    83
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     0,     2,     2,     1,     2,     0,     1,     0,     1,
       3,     2,     0,     1,     2,     3,     2,     3,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     0,
       0,     0,     0,    14,     0,     8,     0,     0,     8,     3,
       0,     3,     1,     3,     1,     1,     0,     0,     4,     0,
      12,     0,     1,     0,     3,     3,     1,     3,     4,     3,
       5,     6,     0,     1,     1,     3,     3,     5,     0,     1,
       0,     1,     0,     4,     0,     4,     0,     4,     2,     3,
       3,     3,     3,     3,     3,     3,     2,     1,     1,     3,
       4,     2,     2,     4,     4,     4,     3,     1,     4,     1,
       1,     1,     1,     1,     0,     1,     2
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       1,     0,     0,     0,    23,    97,    88,     0,     0,    24,
      26,     0,    70,    29,     0,    36,     0,   101,    99,   100,
       0,    20,    27,   103,    25,    40,    21,   102,     0,     0,
       0,     2,     0,     9,    18,     4,    22,    87,     5,    19,
      78,    62,     0,    97,   101,    91,     0,     0,    28,    71,
       0,     0,     0,     0,     0,     0,     0,    86,     0,     0,
       0,    13,     3,     0,    74,    76,     0,     0,     0,     0,
       0,     0,     0,    72,    92,    97,     0,    63,    64,     0,
      51,     0,    68,     0,     0,     0,     0,    96,    44,    41,
      42,    45,    89,     0,    16,    39,    10,     0,     0,    79,
      80,    81,    82,    83,    84,    85,     0,     0,    90,     0,
      98,    56,     0,     0,    52,    93,     0,    69,    34,    37,
      94,    95,     0,    15,    17,    75,    77,    73,    65,    97,
      66,     0,     0,     6,     0,    30,     6,     0,    43,     0,
      57,     0,     7,     0,    59,     0,    68,     0,     6,    67,
      58,   104,     0,     0,     0,    46,     0,   105,    53,    60,
       0,    31,    47,    35,    38,   106,     0,    49,    61,    68,
       6,     0,     0,     0,     0,    54,    55,     0,    32,    48,
      50,     6,     0,    33,     0,     0
};

static const short yydefgoto[] =
{
       1,    31,   143,    32,    60,    61,    34,    50,   146,   169,
     181,   136,    52,   137,    56,    89,    90,   163,   170,    35,
     172,   113,   167,   114,    76,    77,   116,    48,    36,   106,
      97,    98,    37,   158
};

static const short yypact[] =
{
  -32768,   170,   375,   567,-32768,   -25,-32768,    -3,     7,-32768,
  -32768,   -32,   567,-32768,   -29,-32768,   -26,    -7,-32768,-32768,
      -5,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   567,   567,
     213,-32768,    16,-32768,-32768,-32768,   642,    14,-32768,-32768,
      63,   597,   567,    -9,-32768,-32768,    18,   567,-32768,   642,
      19,   567,    20,   567,   567,    15,   537,-32768,   122,   505,
       3,-32768,-32768,   305,-32768,-32768,   567,   567,   567,   567,
     567,   567,   567,-32768,-32768,   -18,    21,    26,   642,    39,
      -4,   410,   567,   419,   567,   428,   466,-32768,-32768,-32768,
      36,   642,-32768,   259,   505,-32768,-32768,   567,   567,   404,
     316,   316,    44,    44,    44,    44,   567,   107,-32768,   627,
  -32768,    -8,    79,    45,    43,-32768,    49,   642,-32768,   642,
  -32768,-32768,   537,-32768,-32768,    63,   652,   404,-32768,    38,
     642,    46,    48,    90,    -1,-32768,    90,    61,-32768,   337,
  -32768,    59,-32768,    65,    64,   103,   567,   505,    90,-32768,
  -32768,   111,    68,    70,    78,    99,   505,-32768,     5,-32768,
      75,-32768,-32768,-32768,-32768,-32768,    -4,-32768,-32768,   567,
      90,    13,   213,    81,   505,-32768,-32768,     6,-32768,-32768,
  -32768,    90,   505,-32768,   129,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,  -135,-32768,   -37,     1,    -2,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,    25,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,   -30,-32768,-32768,  -136,-32768,     0,-32768,
  -32768,-32768,   131,-32768
};


#define	YYLAST		693


static const short yytable[] =
{
      39,   147,    33,    40,   111,    43,    93,   144,   165,    93,
     154,    47,    49,   156,    51,    46,   175,    53,    41,    62,
      44,    18,    19,    42,    73,    41,    74,    23,    57,    58,
     107,   166,    27,   173,   112,   174,    54,   145,    55,    42,
     131,    78,    79,    64,    65,    94,   182,    81,    94,    95,
      66,    83,   180,    85,    86,   176,    91,    39,    63,    87,
     134,    80,    82,    84,    96,   108,    99,   100,   101,   102,
     103,   104,   105,   109,    66,    67,    68,    69,    70,    71,
      72,    41,   117,   122,   119,    72,   139,   132,   110,   133,
     134,   135,   124,   142,   123,   140,   141,   125,   126,    67,
      68,    69,    70,    71,    72,   148,   127,    79,   150,   130,
     151,   153,   152,     3,   157,     5,     6,   159,   160,     7,
     161,   162,    91,    11,   168,   178,    64,    65,    16,   185,
      17,    18,    19,    66,    20,   177,   171,    23,    45,    79,
       0,     0,    27,     0,    28,   155,   117,   138,     0,     0,
      29,     0,     0,     0,   164,     0,   128,     0,    67,    68,
      69,    70,    71,    72,     0,     0,    92,     0,     0,   117,
     184,     2,   179,    -8,     0,     0,     3,     4,     5,     6,
     183,     0,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,     0,    17,    18,    19,     0,    20,    21,    22,
      23,    24,    25,    26,     0,    27,     0,    28,     0,     0,
       0,     0,    -8,    29,    59,    30,   -12,     0,     0,     3,
       4,     5,     6,     0,     0,     7,     0,     9,    10,    11,
      12,    13,    14,    15,    16,     0,    17,    18,    19,     0,
      20,    21,    22,    23,    24,    25,    26,     0,    27,     0,
      28,     0,     0,     0,     0,   -12,    29,     0,    30,   -12,
      59,     0,   -14,     0,     0,     3,     4,     5,     6,     0,
       0,     7,     0,     9,    10,    11,    12,    13,    14,    15,
      16,     0,    17,    18,    19,     0,    20,    21,    22,    23,
      24,    25,    26,     0,    27,     0,    28,     0,     0,     0,
       0,   -14,    29,     0,    30,   -14,    59,     0,   -11,     0,
       0,     3,     4,     5,     6,     0,     0,     7,     0,     9,
      10,    11,    12,    13,    14,    15,    16,     0,    17,    18,
      19,     0,    20,    21,    22,    23,    24,    25,    26,     0,
      27,     0,    28,     3,     0,     5,     6,   -11,    29,     7,
      30,     0,     0,    11,    69,    70,    71,    72,    16,     0,
      17,    18,    19,     0,    20,     0,     0,    23,     0,     0,
       0,     0,    27,     0,    28,     0,     0,     0,    38,     0,
      29,     3,     4,     5,     6,     0,   149,     7,     0,     9,
      10,    11,    12,    13,    14,    15,    16,     0,    17,    18,
      19,     0,    20,    21,    22,    23,    24,    25,    26,     0,
      27,     0,    28,     0,    64,    65,     0,     0,    29,     0,
      30,    66,     0,    64,    65,     0,     0,     0,     0,     0,
      66,     0,    64,    65,     0,     0,     0,     0,     0,    66,
      67,    68,    69,    70,    71,    72,    67,    68,    69,    70,
      71,    72,     0,     0,   115,    67,    68,    69,    70,    71,
      72,     0,     0,   118,    67,    68,    69,    70,    71,    72,
      64,    65,   120,     0,     0,     0,     0,    66,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    67,    68,    69,    70,    71,    72,     0,     0,
     121,     3,     4,     5,     6,     0,     0,     7,     0,     9,
      10,    11,    12,    13,    14,    15,    16,     0,    17,    18,
      19,     0,    20,    21,    22,    23,    24,    25,    26,     0,
      27,     0,    28,     3,    88,     5,     6,     0,    29,     7,
      30,     0,     0,    11,     0,     0,     0,     0,    16,     0,
      17,    18,    19,     0,    20,     0,     0,    23,     0,     0,
       0,     0,    27,     3,    28,     5,     6,     0,     0,     7,
      29,     0,     0,    11,     0,     0,     0,     0,    16,     0,
      17,    18,    19,     0,    20,     0,     0,    23,     0,     0,
       0,     0,    27,     3,    28,    75,     6,     0,     0,     7,
      29,     0,     0,    11,     0,     0,     0,     0,    16,     0,
      17,    18,    19,     0,    20,     0,     0,    23,     0,     0,
       0,     0,    27,     3,    28,   129,     6,     0,     0,     7,
      29,     0,     0,    11,     0,     0,    64,    65,    16,     0,
      17,    18,    19,    66,    20,     0,    64,    23,     0,     0,
       0,     0,    27,    66,    28,     0,     0,     0,     0,     0,
      29,     0,     0,     0,     0,     0,     0,     0,    67,    68,
      69,    70,    71,    72,     0,     0,     0,     0,    67,    68,
      69,    70,    71,    72
};

static const short yycheck[] =
{
       2,   136,     1,     3,     8,     8,     3,     8,     3,     3,
     146,    43,    12,   148,    43,     8,     3,    43,    43,     3,
      23,    24,    25,    48,    10,    43,    12,    30,    28,    29,
      48,    26,    35,   169,    38,   170,    43,    38,    43,    48,
      48,    41,    42,     4,     5,    42,   181,    47,    42,    46,
      11,    51,    46,    53,    54,    42,    56,    59,    42,    44,
      47,    43,    43,    43,    63,    44,    66,    67,    68,    69,
      70,    71,    72,    47,    11,    36,    37,    38,    39,    40,
      41,    43,    82,    47,    84,    41,    48,     8,    49,    44,
      47,    42,    94,     3,    93,    49,    48,    97,    98,    36,
      37,    38,    39,    40,    41,    44,   106,   107,    49,   109,
      45,     8,    48,     6,     3,     8,     9,    49,    48,    12,
      42,    22,   122,    16,    49,    44,     4,     5,    21,     0,
      23,    24,    25,    11,    27,   172,   166,    30,     7,   139,
      -1,    -1,    35,    -1,    37,   147,   146,   122,    -1,    -1,
      43,    -1,    -1,    -1,   156,    -1,    49,    -1,    36,    37,
      38,    39,    40,    41,    -1,    -1,    44,    -1,    -1,   169,
       0,     1,   174,     3,    -1,    -1,     6,     7,     8,     9,
     182,    -1,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    -1,    23,    24,    25,    -1,    27,    28,    29,
      30,    31,    32,    33,    -1,    35,    -1,    37,    -1,    -1,
      -1,    -1,    42,    43,     1,    45,     3,    -1,    -1,     6,
       7,     8,     9,    -1,    -1,    12,    -1,    14,    15,    16,
      17,    18,    19,    20,    21,    -1,    23,    24,    25,    -1,
      27,    28,    29,    30,    31,    32,    33,    -1,    35,    -1,
      37,    -1,    -1,    -1,    -1,    42,    43,    -1,    45,    46,
       1,    -1,     3,    -1,    -1,     6,     7,     8,     9,    -1,
      -1,    12,    -1,    14,    15,    16,    17,    18,    19,    20,
      21,    -1,    23,    24,    25,    -1,    27,    28,    29,    30,
      31,    32,    33,    -1,    35,    -1,    37,    -1,    -1,    -1,
      -1,    42,    43,    -1,    45,    46,     1,    -1,     3,    -1,
      -1,     6,     7,     8,     9,    -1,    -1,    12,    -1,    14,
      15,    16,    17,    18,    19,    20,    21,    -1,    23,    24,
      25,    -1,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    -1,    37,     6,    -1,     8,     9,    42,    43,    12,
      45,    -1,    -1,    16,    38,    39,    40,    41,    21,    -1,
      23,    24,    25,    -1,    27,    -1,    -1,    30,    -1,    -1,
      -1,    -1,    35,    -1,    37,    -1,    -1,    -1,     3,    -1,
      43,     6,     7,     8,     9,    -1,    49,    12,    -1,    14,
      15,    16,    17,    18,    19,    20,    21,    -1,    23,    24,
      25,    -1,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    -1,    37,    -1,     4,     5,    -1,    -1,    43,    -1,
      45,    11,    -1,     4,     5,    -1,    -1,    -1,    -1,    -1,
      11,    -1,     4,     5,    -1,    -1,    -1,    -1,    -1,    11,
      36,    37,    38,    39,    40,    41,    36,    37,    38,    39,
      40,    41,    -1,    -1,    44,    36,    37,    38,    39,    40,
      41,    -1,    -1,    44,    36,    37,    38,    39,    40,    41,
       4,     5,    44,    -1,    -1,    -1,    -1,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    36,    37,    38,    39,    40,    41,    -1,    -1,
      44,     6,     7,     8,     9,    -1,    -1,    12,    -1,    14,
      15,    16,    17,    18,    19,    20,    21,    -1,    23,    24,
      25,    -1,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    -1,    37,     6,     7,     8,     9,    -1,    43,    12,
      45,    -1,    -1,    16,    -1,    -1,    -1,    -1,    21,    -1,
      23,    24,    25,    -1,    27,    -1,    -1,    30,    -1,    -1,
      -1,    -1,    35,     6,    37,     8,     9,    -1,    -1,    12,
      43,    -1,    -1,    16,    -1,    -1,    -1,    -1,    21,    -1,
      23,    24,    25,    -1,    27,    -1,    -1,    30,    -1,    -1,
      -1,    -1,    35,     6,    37,     8,     9,    -1,    -1,    12,
      43,    -1,    -1,    16,    -1,    -1,    -1,    -1,    21,    -1,
      23,    24,    25,    -1,    27,    -1,    -1,    30,    -1,    -1,
      -1,    -1,    35,     6,    37,     8,     9,    -1,    -1,    12,
      43,    -1,    -1,    16,    -1,    -1,     4,     5,    21,    -1,
      23,    24,    25,    11,    27,    -1,     4,    30,    -1,    -1,
      -1,    -1,    35,    11,    37,    -1,    -1,    -1,    -1,    -1,
      43,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,
      38,    39,    40,    41,    -1,    -1,    -1,    -1,    36,    37,
      38,    39,    40,    41
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif
#endif

#line 315 "/usr/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 1:
#line 108 "bc.y"
{
			      yyval.i_value = 0;
			      if (interactive && !quiet)
				{
				  show_bc_version ();
				  welcome ();
				}
			    }
    break;
case 3:
#line 119 "bc.y"
{ run_code (); }
    break;
case 4:
#line 121 "bc.y"
{ run_code (); }
    break;
case 5:
#line 123 "bc.y"
{
			      yyerrok;
			      init_gen ();
			    }
    break;
case 7:
#line 130 "bc.y"
{ warn ("newline not allowed"); }
    break;
case 8:
#line 133 "bc.y"
{ yyval.i_value = 0; }
    break;
case 12:
#line 139 "bc.y"
{ yyval.i_value = 0; }
    break;
case 19:
#line 148 "bc.y"
{ yyval.i_value = yyvsp[0].i_value; }
    break;
case 20:
#line 151 "bc.y"
{ warranty (""); }
    break;
case 21:
#line 153 "bc.y"
{ limits (); }
    break;
case 22:
#line 155 "bc.y"
{
			      if (yyvsp[0].i_value & 2)
				warn ("comparison in expression");
			      if (yyvsp[0].i_value & 1)
				generate ("W");
			      else 
				generate ("p");
			    }
    break;
case 23:
#line 164 "bc.y"
{
			      yyval.i_value = 0;
			      generate ("w");
			      generate (yyvsp[0].s_value);
			      free (yyvsp[0].s_value);
			    }
    break;
case 24:
#line 171 "bc.y"
{
			      if (break_label == 0)
				yyerror ("Break outside a for/while");
			      else
				{
				  sprintf (genstr, "J%1d:", break_label);
				  generate (genstr);
				}
			    }
    break;
case 25:
#line 181 "bc.y"
{
			      warn ("Continue statement");
			      if (continue_label == 0)
				yyerror ("Continue outside a for");
			      else
				{
				  sprintf (genstr, "J%1d:", continue_label);
				  generate (genstr);
				}
			    }
    break;
case 26:
#line 192 "bc.y"
{ exit (0); }
    break;
case 27:
#line 194 "bc.y"
{ generate ("h"); }
    break;
case 28:
#line 196 "bc.y"
{ generate ("R"); }
    break;
case 29:
#line 198 "bc.y"
{
			      yyvsp[0].i_value = break_label; 
			      break_label = next_label++;
			    }
    break;
case 30:
#line 203 "bc.y"
{
			      if (yyvsp[-1].i_value & 2)
				warn ("Comparison in first for expression");
			      if (yyvsp[-1].i_value >= 0)
				generate ("p");
			      yyvsp[-1].i_value = next_label++;
			      sprintf (genstr, "N%1d:", yyvsp[-1].i_value);
			      generate (genstr);
			    }
    break;
case 31:
#line 213 "bc.y"
{
			      if (yyvsp[-1].i_value < 0) generate ("1");
			      yyvsp[-1].i_value = next_label++;
			      sprintf (genstr, "B%1d:J%1d:", yyvsp[-1].i_value, break_label);
			      generate (genstr);
			      yyval.i_value = continue_label;
			      continue_label = next_label++;
			      sprintf (genstr, "N%1d:", continue_label);
			      generate (genstr);
			    }
    break;
case 32:
#line 224 "bc.y"
{
			      if (yyvsp[-1].i_value & 2 )
				warn ("Comparison in third for expression");
			      if (yyvsp[-1].i_value & 16)
				sprintf (genstr, "J%1d:N%1d:", yyvsp[-7].i_value, yyvsp[-4].i_value);
			      else
				sprintf (genstr, "pJ%1d:N%1d:", yyvsp[-7].i_value, yyvsp[-4].i_value);
			      generate (genstr);
			    }
    break;
case 33:
#line 234 "bc.y"
{
			      sprintf (genstr, "J%1d:N%1d:",
				       continue_label, break_label);
			      generate (genstr);
			      break_label = yyvsp[-13].i_value;
			      continue_label = yyvsp[-5].i_value;
			    }
    break;
case 34:
#line 242 "bc.y"
{
			      yyvsp[-1].i_value = if_label;
			      if_label = next_label++;
			      sprintf (genstr, "Z%1d:", if_label);
			      generate (genstr);
			    }
    break;
case 35:
#line 249 "bc.y"
{
			      sprintf (genstr, "N%1d:", if_label); 
			      generate (genstr);
			      if_label = yyvsp[-5].i_value;
			    }
    break;
case 36:
#line 255 "bc.y"
{
			      yyvsp[0].i_value = next_label++;
			      sprintf (genstr, "N%1d:", yyvsp[0].i_value);
			      generate (genstr);
			    }
    break;
case 37:
#line 261 "bc.y"
{
			      yyvsp[0].i_value = break_label; 
			      break_label = next_label++;
			      sprintf (genstr, "Z%1d:", break_label);
			      generate (genstr);
			    }
    break;
case 38:
#line 268 "bc.y"
{
			      sprintf (genstr, "J%1d:N%1d:", yyvsp[-7].i_value, break_label);
			      generate (genstr);
			      break_label = yyvsp[-4].i_value;
			    }
    break;
case 39:
#line 274 "bc.y"
{ yyval.i_value = 0; }
    break;
case 40:
#line 276 "bc.y"
{  warn ("print statement"); }
    break;
case 44:
#line 283 "bc.y"
{
			      generate ("O");
			      generate (yyvsp[0].s_value);
			      free (yyvsp[0].s_value);
			    }
    break;
case 45:
#line 289 "bc.y"
{ generate ("P"); }
    break;
case 47:
#line 293 "bc.y"
{
			      warn ("else clause in if statement");
			      yyvsp[0].i_value = next_label++;
			      sprintf (genstr, "J%d:N%1d:", yyvsp[0].i_value, if_label); 
			      generate (genstr);
			      if_label = yyvsp[0].i_value;
			    }
    break;
case 49:
#line 303 "bc.y"
{
			      /* Check auto list against parameter list? */
			      check_params (yyvsp[-5].a_value,yyvsp[0].a_value);
			      sprintf (genstr, "F%d,%s.%s[",
				       lookup(yyvsp[-7].s_value,FUNCTDEF), 
				       arg_str (yyvsp[-5].a_value), arg_str (yyvsp[0].a_value));
			      generate (genstr);
			      free_args (yyvsp[-5].a_value);
			      free_args (yyvsp[0].a_value);
			      yyvsp[-8].i_value = next_label;
			      next_label = 1;
			    }
    break;
case 50:
#line 316 "bc.y"
{
			      generate ("0R]");
			      next_label = yyvsp[-11].i_value;
			    }
    break;
case 51:
#line 322 "bc.y"
{ yyval.a_value = NULL; }
    break;
case 53:
#line 326 "bc.y"
{ yyval.a_value = NULL; }
    break;
case 54:
#line 328 "bc.y"
{ yyval.a_value = yyvsp[-1].a_value; }
    break;
case 55:
#line 330 "bc.y"
{ yyval.a_value = yyvsp[-1].a_value; }
    break;
case 56:
#line 333 "bc.y"
{ yyval.a_value = nextarg (NULL, lookup (yyvsp[0].s_value,SIMPLE), FALSE);}
    break;
case 57:
#line 335 "bc.y"
{ yyval.a_value = nextarg (NULL, lookup (yyvsp[-2].s_value,ARRAY), FALSE); }
    break;
case 58:
#line 337 "bc.y"
{ yyval.a_value = nextarg (NULL, lookup (yyvsp[-2].s_value,ARRAY), TRUE); }
    break;
case 59:
#line 339 "bc.y"
{ yyval.a_value = nextarg (yyvsp[-2].a_value, lookup (yyvsp[0].s_value,SIMPLE), FALSE); }
    break;
case 60:
#line 341 "bc.y"
{ yyval.a_value = nextarg (yyvsp[-4].a_value, lookup (yyvsp[-2].s_value,ARRAY), FALSE); }
    break;
case 61:
#line 343 "bc.y"
{ yyval.a_value = nextarg (yyvsp[-5].a_value, lookup (yyvsp[-2].s_value,ARRAY), TRUE); }
    break;
case 62:
#line 346 "bc.y"
{ yyval.a_value = NULL; }
    break;
case 64:
#line 350 "bc.y"
{
			      if (yyvsp[0].i_value & 2) warn ("comparison in argument");
			      yyval.a_value = nextarg (NULL,0,FALSE);
			    }
    break;
case 65:
#line 355 "bc.y"
{
			      sprintf (genstr, "K%d:", -lookup (yyvsp[-2].s_value,ARRAY));
			      generate (genstr);
			      yyval.a_value = nextarg (NULL,1,FALSE);
			    }
    break;
case 66:
#line 361 "bc.y"
{
			      if (yyvsp[0].i_value & 2) warn ("comparison in argument");
			      yyval.a_value = nextarg (yyvsp[-2].a_value,0,FALSE);
			    }
    break;
case 67:
#line 366 "bc.y"
{
			      sprintf (genstr, "K%d:", -lookup (yyvsp[-2].s_value,ARRAY));
			      generate (genstr);
			      yyval.a_value = nextarg (yyvsp[-4].a_value,1,FALSE);
			    }
    break;
case 68:
#line 382 "bc.y"
{
			      yyval.i_value = 16;
			      warn ("Missing expression in for statement");
			    }
    break;
case 70:
#line 389 "bc.y"
{
			      yyval.i_value = 0;
			      generate ("0");
			    }
    break;
case 71:
#line 394 "bc.y"
{
			      if (yyvsp[0].i_value & 2)
				warn ("comparison in return expresion");
			      if (!(yyvsp[0].i_value & 4))
				warn ("return expression requires parenthesis");
			    }
    break;
case 72:
#line 402 "bc.y"
{
			      if (yyvsp[0].c_value != '=')
				{
				  if (yyvsp[-1].i_value < 0)
				    sprintf (genstr, "DL%d:", -yyvsp[-1].i_value);
				  else
				    sprintf (genstr, "l%d:", yyvsp[-1].i_value);
				  generate (genstr);
				}
			    }
    break;
case 73:
#line 413 "bc.y"
{
			      if (yyvsp[0].i_value & 2) warn("comparison in assignment");
			      if (yyvsp[-2].c_value != '=')
				{
				  sprintf (genstr, "%c", yyvsp[-2].c_value);
				  generate (genstr);
				}
			      if (yyvsp[-3].i_value < 0)
				sprintf (genstr, "S%d:", -yyvsp[-3].i_value);
			      else
				sprintf (genstr, "s%d:", yyvsp[-3].i_value);
			      generate (genstr);
			      yyval.i_value = 0;
			    }
    break;
case 74:
#line 429 "bc.y"
{
			      warn("&& operator");
			      yyvsp[0].i_value = next_label++;
			      sprintf (genstr, "DZ%d:p", yyvsp[0].i_value);
			      generate (genstr);
			    }
    break;
case 75:
#line 436 "bc.y"
{
			      sprintf (genstr, "DZ%d:p1N%d:", yyvsp[-2].i_value, yyvsp[-2].i_value);
			      generate (genstr);
			      yyval.i_value = (yyvsp[-3].i_value | yyvsp[0].i_value) & ~4;
			    }
    break;
case 76:
#line 442 "bc.y"
{
			      warn("|| operator");
			      yyvsp[0].i_value = next_label++;
			      sprintf (genstr, "B%d:", yyvsp[0].i_value);
			      generate (genstr);
			    }
    break;
case 77:
#line 449 "bc.y"
{
			      int tmplab;
			      tmplab = next_label++;
			      sprintf (genstr, "B%d:0J%d:N%d:1N%d:",
				       yyvsp[-2].i_value, tmplab, yyvsp[-2].i_value, tmplab);
			      generate (genstr);
			      yyval.i_value = (yyvsp[-3].i_value | yyvsp[0].i_value) & ~4;
			    }
    break;
case 78:
#line 458 "bc.y"
{
			      yyval.i_value = yyvsp[0].i_value & ~4;
			      warn("! operator");
			      generate ("!");
			    }
    break;
case 79:
#line 464 "bc.y"
{
			      yyval.i_value = 3;
			      switch (*(yyvsp[-1].s_value))
				{
				case '=':
				  generate ("=");
				  break;

				case '!':
				  generate ("#");
				  break;

				case '<':
				  if (yyvsp[-1].s_value[1] == '=')
				    generate ("{");
				  else
				    generate ("<");
				  break;

				case '>':
				  if (yyvsp[-1].s_value[1] == '=')
				    generate ("}");
				  else
				    generate (">");
				  break;
				}
			    }
    break;
case 80:
#line 492 "bc.y"
{
			      generate ("+");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    }
    break;
case 81:
#line 497 "bc.y"
{
			      generate ("-");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    }
    break;
case 82:
#line 502 "bc.y"
{
			      generate ("*");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    }
    break;
case 83:
#line 507 "bc.y"
{
			      generate ("/");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    }
    break;
case 84:
#line 512 "bc.y"
{
			      generate ("%");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    }
    break;
case 85:
#line 517 "bc.y"
{
			      generate ("^");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    }
    break;
case 86:
#line 522 "bc.y"
{
			      generate ("n");
			      yyval.i_value = yyvsp[0].i_value & ~4;
			    }
    break;
case 87:
#line 527 "bc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[0].i_value < 0)
				sprintf (genstr, "L%d:", -yyvsp[0].i_value);
			      else
				sprintf (genstr, "l%d:", yyvsp[0].i_value);
			      generate (genstr);
			    }
    break;
case 88:
#line 536 "bc.y"
{
			      int len = strlen(yyvsp[0].s_value);
			      yyval.i_value = 1;
			      if (len == 1 && *yyvsp[0].s_value == '0')
				generate ("0");
			      else if (len == 1 && *yyvsp[0].s_value == '1')
				generate ("1");
			      else
				{
				  generate ("K");
				  generate (yyvsp[0].s_value);
				  generate (":");
				}
			      free (yyvsp[0].s_value);
			    }
    break;
case 89:
#line 552 "bc.y"
{ yyval.i_value = yyvsp[-1].i_value | 5; }
    break;
case 90:
#line 554 "bc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[-1].a_value != NULL)
				{ 
				  sprintf (genstr, "C%d,%s:",
					   lookup (yyvsp[-3].s_value,FUNCT),
					   call_str (yyvsp[-1].a_value));
				  free_args (yyvsp[-1].a_value);
				}
			      else
				{
				  sprintf (genstr, "C%d:", lookup (yyvsp[-3].s_value,FUNCT));
				}
			      generate (genstr);
			    }
    break;
case 91:
#line 570 "bc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[0].i_value < 0)
				{
				  if (yyvsp[-1].c_value == '+')
				    sprintf (genstr, "DA%d:L%d:", -yyvsp[0].i_value, -yyvsp[0].i_value);
				  else
				    sprintf (genstr, "DM%d:L%d:", -yyvsp[0].i_value, -yyvsp[0].i_value);
				}
			      else
				{
				  if (yyvsp[-1].c_value == '+')
				    sprintf (genstr, "i%d:l%d:", yyvsp[0].i_value, yyvsp[0].i_value);
				  else
				    sprintf (genstr, "d%d:l%d:", yyvsp[0].i_value, yyvsp[0].i_value);
				}
			      generate (genstr);
			    }
    break;
case 92:
#line 589 "bc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[-1].i_value < 0)
				{
				  sprintf (genstr, "DL%d:x", -yyvsp[-1].i_value);
				  generate (genstr); 
				  if (yyvsp[0].c_value == '+')
				    sprintf (genstr, "A%d:", -yyvsp[-1].i_value);
				  else
				      sprintf (genstr, "M%d:", -yyvsp[-1].i_value);
				}
			      else
				{
				  sprintf (genstr, "l%d:", yyvsp[-1].i_value);
				  generate (genstr);
				  if (yyvsp[0].c_value == '+')
				    sprintf (genstr, "i%d:", yyvsp[-1].i_value);
				  else
				    sprintf (genstr, "d%d:", yyvsp[-1].i_value);
				}
			      generate (genstr);
			    }
    break;
case 93:
#line 612 "bc.y"
{ generate ("cL"); yyval.i_value = 1;}
    break;
case 94:
#line 614 "bc.y"
{ generate ("cR"); yyval.i_value = 1;}
    break;
case 95:
#line 616 "bc.y"
{ generate ("cS"); yyval.i_value = 1;}
    break;
case 96:
#line 618 "bc.y"
{
			      warn ("read function");
			      generate ("cI"); yyval.i_value = 1;
			    }
    break;
case 97:
#line 624 "bc.y"
{ yyval.i_value = lookup(yyvsp[0].s_value,SIMPLE); }
    break;
case 98:
#line 626 "bc.y"
{
			      if (yyvsp[-1].i_value > 1) warn("comparison in subscript");
			      yyval.i_value = lookup(yyvsp[-3].s_value,ARRAY);
			    }
    break;
case 99:
#line 631 "bc.y"
{ yyval.i_value = 0; }
    break;
case 100:
#line 633 "bc.y"
{ yyval.i_value = 1; }
    break;
case 101:
#line 635 "bc.y"
{ yyval.i_value = 2; }
    break;
case 102:
#line 637 "bc.y"
{ yyval.i_value = 3;
			      warn ("History variable");
			    }
    break;
case 103:
#line 641 "bc.y"
{ yyval.i_value = 4;
			      warn ("Last variable");
			    }
    break;
case 104:
#line 647 "bc.y"
{ warn ("End of line required"); }
    break;
case 106:
#line 650 "bc.y"
{ warn ("Too many end of lines"); }
    break;
}

#line 705 "/usr/share/bison/bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
yyerrhandle:
  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
#line 653 "bc.y"


