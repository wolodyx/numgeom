#include "utilities_occ.h"

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <ShapeFix_Shape.hxx>
#include <STEPControl_Reader.hxx>


TopoDS_Shape LoadFromFile(const std::filesystem::path& fileName)
{
    if(!std::filesystem::exists(fileName))
        return TopoDS_Shape();

    std::string ext = fileName.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c){ return std::tolower(c); });
    TopoDS_Shape shape;
    if(ext == ".step" || ext == ".stp")
    {
        STEPControl_Reader reader;
        std::string str = fileName.string();
        if(reader.ReadFile(str.c_str()) != IFSelect_RetDone)
            return TopoDS_Shape();
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
        BRep_Builder builder;
        BRepTools::Read(shape, fileName.string().c_str(), builder);
    }

    return shape;
}
