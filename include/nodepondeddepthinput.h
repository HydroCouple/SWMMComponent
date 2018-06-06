#ifndef NODEPONDEDDEPTHINPUT_H
#define NODEPONDEDDEPTHINPUT_H


#include "swmmcomponent_global.h"
#include "spatiotemporal/timegeometryinput.h"
#include <unordered_map>

class SWMMComponent;


class SWMMCOMPONENT_EXPORT NodePondedDepthInput: public TimeGeometryInputDouble
{
    Q_OBJECT

  public:
    NodePondedDepthInput(const QString &id,
                         Dimension *timeDimension,
                         Dimension *geometryDimension,
                         ValueDefinition *valueDefinition,
                         SWMMComponent *modelComponent);

    bool setProvider(HydroCouple::IOutput *provider) override;

    bool canConsume(HydroCouple::IOutput *provider, QString &message) const override;

    void retrieveValuesFromProvider() override;

    void applyData() override;

  private:

    SWMMComponent *m_SWMMComponent;
    std::unordered_map<int,int> m_geometryMapping;
    int m_printTracker;

};


#endif // NODEPONDEDDEPTHINPUT_H
