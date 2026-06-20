#ifndef NUMGEOM_FRAMEWORK_FGOBJECT_H
#define NUMGEOM_FRAMEWORK_FGOBJECT_H

#include "numgeom/pixelbuffer.h"
#include "numgeom/trackedobject.h"

class FgObject : public TrackedObject {
public:
  virtual ~FgObject();
  virtual const PixelBuffer& GetPixelBuffer() const = 0;
  virtual uint32_t GetWidth() const = 0;
  virtual uint32_t GetHeight() const = 0;

 protected:
  FgObject();

private:
  FgObject(const FgObject&) = delete;
  FgObject operator=(const FgObject&) = delete;
};
#endif // !NUMGEOM_FRAMEWORK_FGOBJECT_H
