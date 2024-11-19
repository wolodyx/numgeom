%{
#include <stdio.h>
#include "vtk_parser.h"

int vtklex();
void vtkerror(const char* msg);

int vtkwrap() { return 1; }
void InsertNextValue(double);
void PreparePointArray(int);
void PrepareVtkLoading();
void PreparePolygonArray(int, int);
%}


%union {
  double real;
  int integer;
  char* string;
};

%define parse.error verbose
%define api.prefix {vtk}
%locations
%start Input

%token VTK_DATAFILE_VERSION_3_0
       ASCII
       BINARY
       CELL_TYPES
       CELLS
       DATASET
       DIMENSIONS
       DOUBLE
       FLOAT
       POINTS
       POLYDATA
       POLYGONS
       STRUCTURED_GRID
       TITLE
       UNSTRUCTURED_GRID
%token<real> real
%token<integer> integer
%%

Input:
    VTK_DATAFILE_VERSION_3_0 { PrepareVtkLoading(); }
    TITLE
    FileFormat
    DataSet
    Fields
;

FileFormat: ASCII | BINARY;

DataSet: StructuredGrid | UnstructuredGrid | Polydata

StructuredGrid:
    DATASET STRUCTURED_GRID
    DIMENSIONS integer integer integer
    POINTS integer ValueType
    NumericValues
;

UnstructuredGrid:
    DATASET UNSTRUCTURED_GRID
    POINTS integer ValueType
    NumericValues
    CELLS integer integer
    IntValues
    CELL_TYPES integer
    IntValues
;

Polydata:
    DATASET POLYDATA
    POINTS integer ValueType { PreparePointArray($4); }
    NumericValues
    POLYGONS integer integer { PreparePolygonArray($9, $10); }
    IntValues
;

ValueType : FLOAT | DOUBLE

NumericValues:
    real                  { InsertNextValue($1); }
  | integer               { InsertNextValue($1); }
  | NumericValues real    { InsertNextValue($2); }
  | NumericValues integer { InsertNextValue($2); }

IntValues:
    integer               { InsertNextValue($1); }
  | IntValues integer     { InsertNextValue($2); }
;

Fields: ;

%%
