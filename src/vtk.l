%{
#include <stdio.h>
#include "vtk_parser.h"
%}

alpha       [A-Za-z]
dig         [0-9]
string      \"(\\.|[^"\\])*\"
name        {alpha}({alpha}|{dig})*
integer     {dig}+
real        [-]?{dig}+"."{dig}*([eE][-+]?{dig}+)?


%option yylineno
%option never-interactive
%option nounistd
%option prefix="vtk"
%x SC_TITLE
%%

[ \t\n]+        ; /* ignore white spaces */
^"#"[ ]*vtk[ ]*DataFile[ ]*Version[ ]*[123]\.0\n {
                  BEGIN(SC_TITLE);
                  return VTK_DATAFILE_VERSION_3_0;
                }

<SC_TITLE>.*    { return TITLE; }
<SC_TITLE>\n    { BEGIN(INITIAL); }

{real}          {
                  vtklval.real = atof(yytext);
                  return real;
                }

{integer}       {
                  vtklval.integer = atoi(yytext);
                  return integer;
                }
"ASCII"         { return ASCII; }
"BINARY"        { return BINARY; }
"CELL_TYPES"    { return CELL_TYPES; }
"CELLS"         { return CELLS; }
"DATASET"       { return DATASET; }
"DIMENSIONS"    { return DIMENSIONS; }
"double"        { return DOUBLE; }
"float"         { return FLOAT; }
"POINTS"        { return POINTS; }
"POLYDATA"      { return POLYDATA; }
"POLYGONS"      { return POLYGONS; }
"STRUCTURED_GRID" { return STRUCTURED_GRID; }
"UNSTRUCTURED_GRID" { return UNSTRUCTURED_GRID; }
%%
