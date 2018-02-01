#ifndef PONDEDAREAINPUT_H
#define PONDEDAREAINPUT_H

#include "swmmcomponent_global.h"
#include "spatial/geometryexchangeitems.h"

#include <unordered_map>

class SWMMComponent;

class SWMMCOMPONENT_EXPORT PondedAreaInput :
    public GeometryInputDouble
{
    Q_OBJECT

  public:

    PondedAreaInput(const QString& id,
                    Dimension *geometryDimesion,
                    ValueDefinition* valueDefinition,
                    SWMMComponent *modelComponent);

    virtual ~PondedAreaInput();

    bool canConsume(HydroCouple::IOutput *provider, QString &message) const override;

    void retrieveValuesFromProvider() override;

    void applyData() override;

  private:

    SWMMComponent *m_SWMMComponent;
    std::unordered_map<int,int> m_geometryMapping;
    bool m_retrieved, m_applied;

};

#endif // PONDEDAREAINPUT_H
