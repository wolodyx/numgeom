#include "numgeom/loadfromvtk.h"

#include <cassert>

#include "numgeom/staticjaggedarray.h"


static std::string s_vtkerror;


extern "C" {;

#include "vtk_parser.h"

extern FILE* vtkin;
extern int vtklineno;

void vtkerror(const char* str)
{
    std::stringstream ss;
    ss << "Error: (line "
       << vtklineno
       << ") "
       << str;
    s_vtkerror = ss.str();
}


struct VtkGridData
{
    std::vector<gp_Pnt> points;
    StaticJaggedArray cells;
    size_t index, index2;
    size_t num;
    bool loadPointArray;
    bool loadPolydataArray;
    bool loadCellArray;
    bool loadCellTypeArray;

    void ClearFlags()
    {
        loadPointArray = false;
        loadPolydataArray = false;
        loadCellArray = false;
        loadCellTypeArray = false;
    }
};


static VtkGridData s_fileData;


void PrepareVtkLoading()
{
    s_fileData.ClearFlags();
    s_fileData.points.clear();
    s_fileData.cells.Clear();
}


void PreparePointArray(int count)
{
    s_fileData.ClearFlags();
    s_fileData.loadPointArray = true;
    s_fileData.points.resize(count);
    s_fileData.index = 0;
}


void PreparePolygonArray(int rows, int n)
{
    s_fileData.ClearFlags();
    s_fileData.loadPolydataArray = true;
    s_fileData.cells.Initialize(rows, n - rows);
    s_fileData.index = 0;
    s_fileData.index2 = 0;
    s_fileData.num = 0;
}


void InsertNextValue(double val)
{
    if(s_fileData.loadPointArray)
    {
        size_t idx = s_fileData.index / 3;
        size_t comp = s_fileData.index % 3 + 1;
        s_fileData.points[idx].SetCoord(comp, val);
        ++s_fileData.index;
    }
    else if(s_fileData.loadPolydataArray)
    {
        if(s_fileData.num == 0)
        {
            s_fileData.num = static_cast<Standard_Integer>(val);
            s_fileData.cells.Offsets()[s_fileData.index2 + 1] =
                   s_fileData.cells.Offsets()[s_fileData.index2]
                 + s_fileData.num;
            ++s_fileData.index2;
        }
        else
        {
            s_fileData.cells.Data()[s_fileData.index] = static_cast<Standard_Integer>(val);
            std::cout << s_fileData.index << ' ';
            ++s_fileData.index;
            --s_fileData.num;
        }
    }
    else
    {
        assert(false);
        abort();
    }
}
}


TriMesh::Ptr LoadTriMeshFromVtk(
    const std::filesystem::path& fileName
)
{
    FILE* file = fopen(fileName.string().c_str(), "r");
    if(!file)
    {
        return TriMesh::Ptr();
    }

    vtkin = file;
    int parseRC = vtkparse();
    fclose(file);
    if(parseRC != 0)
    {
        return TriMesh::Ptr();
    }

    size_t nbCells = s_fileData.cells.Size();
    size_t nbTrias = 0;
    for(size_t i = 0; i < nbCells; ++i)
    {
        size_t n = s_fileData.cells.Size(i);
        if(n == 3)
            ++nbTrias;
    }

    std::vector<TriMesh::Cell> trias;
    trias.reserve(nbTrias);
    for(size_t i = 0; i < nbCells; ++i)
    {
        size_t n = s_fileData.cells.Size(i);
        if(n == 3)
        {
            const size_t* nodes = s_fileData.cells[i];
            trias.push_back(TriMesh::Cell(nodes[0], nodes[1], nodes[2]));
        }
    }

    return TriMesh::Create(s_fileData.points, trias);
}
