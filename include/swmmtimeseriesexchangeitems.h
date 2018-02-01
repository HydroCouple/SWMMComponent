#ifndef SWMMNODETIMESERIESEXCHANGEITEM
#define SWMMNODETIMESERIESEXCHANGEITEM

#include "swmmcomponent_global.h"
#include "swmmobjectitems.h"
#include "headers.h"
#include "temporal/timeseriesexchangeitem.h"

class SWMMComponent ;
class Dimension;

class SWMMCOMPONENT_EXPORT SWMMNodeWSETimeSeriesOutput : public TimeSeriesOutputDouble
{
    Q_OBJECT

  public:

    SWMMNodeWSETimeSeriesOutput(TNode* node,
                                Dimension* dimension,
                                const std::list<SDKTemporal::DateTime*>& times,
                                ValueDefinition* valueDefinition,
                                SWMMComponent* component);

    virtual ~SWMMNodeWSETimeSeriesOutput();

    void updateValues(HydroCouple::IInput *querySpecifier) override;

    void updateValues() override;

  private:

    TNode *m_node;
    SWMMComponent *m_component;

};

class SWMMCOMPONENT_EXPORT SWMMNodeWSETimeSeriesInput : public TimeSeriesInputDouble
{
    Q_OBJECT

  public:

    SWMMNodeWSETimeSeriesInput(TNode* node,
                               Dimension *dimension,
                               const std::list<SDKTemporal::DateTime *> &times,
                               ValueDefinition* valueDefinition,
                               SWMMComponent *component);

    virtual ~SWMMNodeWSETimeSeriesInput();

    bool canConsume(HydroCouple::IOutput* provider, QString &message) const override;

    void retrieveValuesFromProvider() override;

    void applyData() override;

  private:
    TNode *m_node;
    SWMMComponent *m_component;

};

class SWMMCOMPONENT_EXPORT SWMMNodeLatInflowTimeSeriesInput : public TimeSeriesMultiInputDouble
{
    Q_OBJECT

  public:

    SWMMNodeLatInflowTimeSeriesInput(TNode* node,
                                     Dimension *dimension,
                                     const std::list<SDKTemporal::DateTime*>& times,
                                     ValueDefinition *valueDefinition,
                                     SWMMComponent *component);

    virtual ~SWMMNodeLatInflowTimeSeriesInput();

    bool canConsume(HydroCouple::IOutput* provider, QString &message) const override;

    void retrieveValuesFromProvider() override;

    void applyData() override;

  private:
    TNode *m_node;
    SWMMComponent *m_component;
};


class SWMMCOMPONENT_EXPORT SWMMLinkDischargeTimeSeriesOutput : public TimeSeriesOutputDouble
{
    Q_OBJECT

  public:

    SWMMLinkDischargeTimeSeriesOutput(TLink* link,
                                      Dimension *dimension,
                                      const std::list<SDKTemporal::DateTime*>& times,
                                      ValueDefinition *valueDefinition,
                                      SWMMComponent *component);

    virtual ~SWMMLinkDischargeTimeSeriesOutput();

    void updateValues(HydroCouple::IInput *querySpecifier) override;

    void updateValues() override;

  private:
    TLink *m_link;
    SWMMComponent *m_component;
};



#endif // SWMMNODETIMESERIESEXCHANGEITEM

