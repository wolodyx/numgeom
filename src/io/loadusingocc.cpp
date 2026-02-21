#include "numgeom/loadusingocc.h"

#include <algorithm>

#include "BRepTools.hxx"
#include "ShapeFix_Shape.hxx"
#include "STEPControl_Reader.hxx"

#include "numgeom/utilities.h"


TriMesh::Ptr LoadUsingOCC(const std::filesystem::path& filename)
{
    if(!std::filesystem::exists(filename))
        return TriMesh::Ptr();

    std::string ext = filename.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c){ return std::tolower(c); });

    TopoDS_Shape shape;
    if(ext == ".step" || ext == ".stp")
    {
        STEPControl_Reader reader;
        std::string str = filename.string();
        if(reader.ReadFile(str.c_str()) != IFSelect_RetDone)
            return TriMesh::Ptr();
        //reader.PrintCheckLoad(false, IFSelect_ListByItem);
        Standard_Integer NbRoots = reader.NbRootsForTransfer();
        Standard_Integer NbTrans = reader.TransferRoots();
        shape = reader.OneShape();

        Standard_Boolean doHealing = false;
        if(doHealing)
        {
            ShapeFix_Shape shapeHealer(shape);
            if(shapeHealer.Perform())
                shape = shapeHealer.Shape();
        }
    } else if(ext == ".brep")
    {
        // BRep_Builder builder;
        // BRepTools::Read(shape, filename.string().c_str(), builder);
    }

    return ConvertToTriMesh(shape);
}
