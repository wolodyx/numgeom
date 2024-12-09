#include "numgeom/shaderprogram.h"

#include <fstream>

#include "GL/glew.h"

#include "numgeom/outcome.h"


ShaderProgram::ShaderProgram(unsigned int program)
{
    myProgram = program;
}


ShaderProgram::~ShaderProgram()
{
    if(myProgram != 0)
        glDeleteProgram(myProgram);
}


namespace {;
bool ReadFromFile(
    const std::filesystem::path& path,
    std::string& data
)
{
    data.clear();
    std::ifstream file(path);
    if(!file.is_open())
        return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    data = buffer.str();
    return true;
}
}


ShaderProgram::Ptr ShaderProgram::BuildFromSources(
    const std::filesystem::path& vertexShaderSource,
    const std::filesystem::path& fragmentShaderSource,
    Outcome* outcome
)
{
    std::string vSource;
    if(!ReadFromFile(vertexShaderSource,vSource))
    {
        if(outcome)
            (*outcome) = "Не получилось открыть файл.";
        return Ptr();
    }

    std::string fSource;
    if(!ReadFromFile(fragmentShaderSource,fSource))
    {
        if(outcome)
            (*outcome) = "Не получилось открыть файл.";
        return Ptr();
    }

    return BuildFromSources(vSource.c_str(), fSource.c_str());
}


ShaderProgram::Ptr ShaderProgram::BuildFromSources(
    const char* vertexShaderSource,
    const char* fragmentShaderSource,
    Outcome* outcome
)
{
    if(!vertexShaderSource || !fragmentShaderSource)
    {
        if(outcome)
            (*outcome) = "Не заданы исходные коды шейдеров";
        return Ptr();
    }

    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fShader);

    const GLuint program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);

    if(outcome)
        (*outcome) = Outcome::Success();
    return Ptr(new ShaderProgram(program));
}
