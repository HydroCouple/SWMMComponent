#ifndef SWMMNODETIMESERIESEXCHANGEITEM
#define SWMMNODETIMESERIESEXCHANGEITEM

#include "swmmcomponent_global.h"
#include "swmmobjectitems.h"
#include "core/headers.h"
#include "temporal/timeseriesexchangeitem.h"

class SWMMComponent ;
class Dimension;

class SWMMCOMPONENT_EXPORT SWMMNodeWSETimeSeriesOutput : public TimeSeriesOutputDouble,
    public virtual SWMMOutputObjectItem
{
    Q_OBJECT

  public:

    SWMMNodeWSETimeSeriesOutput(TNode* node,
                                Dimension* dimension,
                                const QList<SDKTemporal::Time*>& times,
                                ValueDefinition* valueDefinition,
                                SWMMComponent* component);

    virtual ~SWMMNodeWSETimeSeriesOutput();

    void update(HydroCouple::IInput *querySpecifier) override;

    void retrieveDataFromModel() override;

  private:
    TNode *m_node;
    SWMMComponent *m_component;

};

class SWMMCOMPONENT_EXPORT SWMMNodeWSETimeSeriesInput : public TimeSeriesInputDouble,
    public virtual SWMMInputObjectItem
{
    Q_OBJECT

  public:

    SWMMNodeWSETimeSeriesInput(TNode* node,
                               Dimension *dimension,
                               const QList<SDKTemporal::Time*>& times,
                               ValueDefinition* valueDefinition,
                               SWMMComponent *component);

    virtual ~SWMMNodeWSETimeSeriesInput();

    bool canConsume(HydroCouple::IOutput* provider, QString &message) const override;

    void retrieveOuputItemData() override;

  private:
    TNode *m_node;
    SWMMComponent *m_component;

};

class SWMMCOMPONENT_EXPORT SWMMNodeLatInflowTimeSeriesInput : public TimeSeriesMultiInputDouble,
    public virtual SWMMInputObjectItem
{
    Q_OBJECT

  public:

    SWMMNodeLatInflowTimeSeriesInput(TNode* node,
                                     Dimension *dimension,
                                     const QList<SDKTemporal::Time*>& times,
                                     ValueDefinition *valueDefinition,
                                     SWMMComponent *component);

    virtual ~SWMMNodeLatInflowTimeSeriesInput();

    bool canConsume(HydroCouple::IOutput* provider, QString &message) const override;

    void retrieveOuputItemData() override;

  private:
    TNode *m_node;
    SWMMComponent *m_component;
};


class SWMMCOMPONENT_EXPORT SWMMLinkDischargeTimeSeriesOutput : public TimeSeriesOutputDouble,
    public virtual SWMMOutputObjectItem
{
    Q_OBJECT

  public:

    SWMMLinkDischargeTimeSeriesOutput(TLink* link,
                                      Dimension *dimension,
                                      const QList<SDKTemporal::Time*>& times,
                                      ValueDefinition *valueDefinition,
                                      SWMMComponent *component);

    virtual ~SWMMLinkDischargeTimeSeriesOutput();

    void update(HydroCouple::IInput *querySpecifier) override;

    void retrieveDataFromModel() override;

  private:
    TLink *m_link;
    SWMMComponent *m_component;
};



#endif // SWMMNODETIMESERIESEXCHANGEITEM

