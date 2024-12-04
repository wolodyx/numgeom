/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_VTK_HOME_BOB_PROJECTS_NUMGEOM_BLD_SRC_VTK_PARSER_H_INCLUDED
# define YY_VTK_HOME_BOB_PROJECTS_NUMGEOM_BLD_SRC_VTK_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef VTKDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define VTKDEBUG 1
#  else
#   define VTKDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define VTKDEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined VTKDEBUG */
#if VTKDEBUG
extern int vtkdebug;
#endif

/* Token kinds.  */
#ifndef VTKTOKENTYPE
# define VTKTOKENTYPE
  enum vtktokentype
  {
    VTKEMPTY = -2,
    VTKEOF = 0,                    /* "end of file"  */
    VTKerror = 256,                /* error  */
    VTKUNDEF = 257,                /* "invalid token"  */
    VTK_DATAFILE_VERSION_3_0 = 258, /* VTK_DATAFILE_VERSION_3_0  */
    ASCII = 259,                   /* ASCII  */
    BINARY = 260,                  /* BINARY  */
    CELL_TYPES = 261,              /* CELL_TYPES  */
    CELLS = 262,                   /* CELLS  */
    DATASET = 263,                 /* DATASET  */
    DIMENSIONS = 264,              /* DIMENSIONS  */
    DOUBLE = 265,                  /* DOUBLE  */
    FLOAT = 266,                   /* FLOAT  */
    POINTS = 267,                  /* POINTS  */
    POLYDATA = 268,                /* POLYDATA  */
    POLYGONS = 269,                /* POLYGONS  */
    STRUCTURED_GRID = 270,         /* STRUCTURED_GRID  */
    TITLE = 271,                   /* TITLE  */
    UNSTRUCTURED_GRID = 272,       /* UNSTRUCTURED_GRID  */
    real = 273,                    /* real  */
    integer = 274                  /* integer  */
  };
  typedef enum vtktokentype vtktoken_kind_t;
#endif

/* Value type.  */
#if ! defined VTKSTYPE && ! defined VTKSTYPE_IS_DECLARED
union VTKSTYPE
{
#line 16 "/home/bob/projects/numgeom/src/vtk.y"

  double real;
  int integer;
  char* string;

#line 97 "/home/bob/projects/numgeom/bld/src/vtk_parser.h"

};
typedef union VTKSTYPE VTKSTYPE;
# define VTKSTYPE_IS_TRIVIAL 1
# define VTKSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined VTKLTYPE && ! defined VTKLTYPE_IS_DECLARED
typedef struct VTKLTYPE VTKLTYPE;
struct VTKLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define VTKLTYPE_IS_DECLARED 1
# define VTKLTYPE_IS_TRIVIAL 1
#endif


extern VTKSTYPE vtklval;
extern VTKLTYPE vtklloc;

int vtkparse (void);


#endif /* !YY_VTK_HOME_BOB_PROJECTS_NUMGEOM_BLD_SRC_VTK_PARSER_H_INCLUDED  */
