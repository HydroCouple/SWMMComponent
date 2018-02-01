#ifndef LINKFLOWOUTPUT_H
#define LINKFLOWOUTPUT_H

#include "swmmcomponent_global.h"
#include "spatiotemporal/timegeometryoutput.h"

class SWMMComponent;

class SWMMCOMPONENT_EXPORT LinkFlowOutput:
    public TimeGeometryOutputDouble
{
    Q_OBJECT

  public:
    LinkFlowOutput(const QString &id,
                   Dimension *timeDimension,
                   Dimension *geometryDimension,
                   ValueDefinition *valueDefinition,
                   SWMMComponent *modelComponent);

    virtual ~LinkFlowOutput();

    void updateValues(HydroCouple::IInput *querySpecifier) override;

    void updateValues() override;

  private:
    SWMMComponent *m_SWMMComponent;
};


#endif // LINKFLOWOUTPUT_H
