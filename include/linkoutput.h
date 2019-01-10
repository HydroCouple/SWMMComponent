#ifndef LINKOUTPUT_H
#define LINKOUTPUT_H

#include "swmmcomponent_global.h"
#include "spatiotemporal/timegeometryoutput.h"

class SWMMComponent;

class SWMMCOMPONENT_EXPORT LinkOutput: public TimeGeometryOutputDouble
{
    Q_OBJECT

  public:

    enum LinkVariable
    {
      Depth,
      WaterSurfaceElevation,
      XsectionArea,
      TopWidth,
      Perimeter,
      Flow,
      DVolumeDTime,
    };

    LinkOutput(const QString &id,
                   Dimension *timeDimension,
                   Dimension *geometryDimension,
                   ValueDefinition *valueDefinition,
                   LinkVariable linkOutputVariable,
                   SWMMComponent *modelComponent);

    virtual ~LinkOutput();

    void updateValues(HydroCouple::IInput *querySpecifier) override;

    void updateValues() override;

  private:
    LinkVariable m_linkVariable;
    SWMMComponent *m_SWMMComponent;
};

#endif // LINKOUTPUT_H
