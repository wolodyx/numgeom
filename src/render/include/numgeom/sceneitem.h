#ifndef numgeom_render_sceneitem_h
#define numgeom_render_sceneitem_h

#include <vector>

#include <gp_Pnt.h>

#include "numgeom/iterator.h"
#include "numgeom/linmath.h"
#include "numgeom/render_export.h"


class RENDER_EXPORT SceneItem
{
public:

    SceneItem();

    virtual ~SceneItem() {}

    virtual size_t pointsNumber() const = 0;

    virtual size_t elementsNumber() const = 0;

    virtual Iterator<gp_Pnt> points() const = 0;

    Vec3f color() const { return myColor; }
    void setColor(const Vec3f& color) { myColor = color; }

    bool isVisible() const { return myVisibility; }
    void setVisible() { myVisibility = true; }
    void setInvisible() { myVisibility = false; }

private:
    Vec3f myColor;
    bool myVisibility;
};


class RENDER_EXPORT SceneItem0 : public SceneItem
{
public:
    virtual ~SceneItem0() {}
};


class RENDER_EXPORT SceneItem1 : public SceneItem
{
public:
    virtual ~SceneItem1() {}
    virtual Iterator<Tuple2s> lines() const = 0;
};


class RENDER_EXPORT SceneItem2 : public SceneItem
{
public:
    virtual ~SceneItem2() {}
    virtual Iterator<Tuple3s> trias() const = 0;
};


class RENDER_EXPORT SceneItem_CachedPoints : public SceneItem
{
public:

    virtual ~SceneItem_CachedPoints() {}

    size_t piontsNumber() const override;

    Iterator<gp_Pnt> points() const override;

    void addPoint(const gp_Pnt&);
    void addPoints(std::initializer_list<gp_Pnt>);

    void clearPoints();

private:
    std::vector<gp_Pnt> myPoints;
};


class RENDER_EXPORT SceneItem1_Cached : public virtual SceneItem_CachedPoints
                                      , public virtual SceneItem1
{
public:

    virtual ~SceneItem1_Cached() {}

    size_t elementsNumber() const override;
    Iterator<Tuple2s> lines() const override;

    void addLine(const Tuple2s&);
    void clearElements();
    void clear();

private:
    std::vector<Tuple2s> myLines;
};


class RENDER_EXPORT SceneItem2_Cached : public virtual SceneItem_CachedPoints
                                      , public virtual SceneItem2
{
public:

    virtual ~SceneItem2_Cached() {}

    size_t elementsNumber() const override;
    Iterator<Tuple3s> trias() const override;

    void addTria(const Tuple3s&);
    void clearElements();
    void clear();

private:
    std::vector<Tuple3s> myTrias;
};
#endif // !numgeom_render_sceneitem_h
