
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     OPTIONS = 258,
     LISTEN_ON = 259,
     NAMESERVER = 260,
     PORT = 261,
     ADDRESS = 262,
     LLQ = 263,
     PUBLIC = 264,
     PRIVATE = 265,
     ALLOWUPDATE = 266,
     ALLOWQUERY = 267,
     KEY = 268,
     ALGORITHM = 269,
     SECRET = 270,
     ISSUER = 271,
     SERIAL = 272,
     ZONE = 273,
     TYPE = 274,
     ALLOW = 275,
     OBRACE = 276,
     EBRACE = 277,
     SEMICOLON = 278,
     IN = 279,
     DOTTED_DECIMAL_ADDRESS = 280,
     WILDCARD = 281,
     DOMAINNAME = 282,
     HOSTNAME = 283,
     QUOTEDSTRING = 284,
     NUMBER = 285
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 96 "../mDNSShared/dnsextd_parser.y"

	int			number;
	char	*	string;



/* Line 1676 of yacc.c  */
#line 89 "objects/prod/dnsextd_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


