#ifndef NODEWSEINPUT_H
#define NODEWSEINPUT_H

#include "swmmcomponent_global.h"
#include "spatiotemporal/timegeometryinput.h"
#include <unordered_map>

class SWMMComponent;


class SWMMCOMPONENT_EXPORT NodeWSEInput :
    public TimeGeometryInputDouble
{
    Q_OBJECT

  public:
    NodeWSEInput(const QString &id,
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

};

#endif // NODEWSEINPUT_H
