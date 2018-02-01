#ifndef NODESURFACEFLOWOUTPUT_H
#define NODESURFACEFLOWOUTPUT_H

#include "swmmcomponent_global.h"
#include "spatiotemporal/timegeometryoutput.h"

class SWMMComponent;

class SWMMCOMPONENT_EXPORT NodeSurfaceFlowOutput:
    public TimeGeometryOutputDouble
{
    Q_OBJECT

  public:
    NodeSurfaceFlowOutput(const QString &id,
                          Dimension *timeDimension,
                          Dimension *geometryDimension,
                          ValueDefinition *valueDefinition,
                          SWMMComponent *modelComponent);

    virtual ~NodeSurfaceFlowOutput();

    void updateValues(HydroCouple::IInput *querySpecifier) override;

    void updateValues() override;

  private:
    SWMMComponent *m_SWMMComponent;
    int m_printTracker;
};


#endif // NODESURFACEFLOWOUTPUT_H
