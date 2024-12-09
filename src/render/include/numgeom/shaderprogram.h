#ifndef numgeom_app_shaderprogram_h
#define numgeom_app_shaderprogram_h

#include <filesystem>
#include <memory>

#include "numgeom/render_export.h"

class Outcome;


class RENDER_EXPORT ShaderProgram
{
public:

    typedef std::shared_ptr<ShaderProgram> Ptr;

    static Ptr BuildFromSources(
        const std::filesystem::path& vertexShaderSource,
        const std::filesystem::path& fragmentShaderSource,
        Outcome* = nullptr
    );

    static Ptr BuildFromSources(
        const char* vertexShaderSource,
        const char* fragmentShaderSource,
        Outcome* = nullptr
    );

public:

    ~ShaderProgram();

private:
    ShaderProgram(unsigned int);
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

private:
    unsigned int myProgram;
};
#endif // !numgeom_app_shaderprogram_h
